#include "tig/database.h"

#include <stdarg.h>
#include <stdio.h>

#include <fpattern/fpattern.h>
#include <zlib.h>

#include "tig/compat.h"
#include "tig/find_file.h"
#include "tig/memory.h"

#define FOURCC_DAT SDL_FOURCC(' ', 'T', 'A', 'D')
#define FOURCC_DAT1 SDL_FOURCC('1', 'T', 'A', 'D')
#define DECOMPRESSION_BUFFER_SIZE 0x4000

typedef struct DecompressionContext {
    /* 0000 */ z_stream zstrm;
    /* 0038 */ unsigned char buffer[DECOMPRESSION_BUFFER_SIZE];
} DecompressionContext;

#define TIG_DATABASE_FILE_UNGOTTEN 0x01
#define TIG_DATABASE_FILE_EOF 0x02
#define TIG_DATABASE_FILE_ERROR 0x04
#define TIG_DATABASE_FILE_TEXT_MODE 0x08

typedef struct TigDatabaseFileHandle {
    unsigned int flags;
    TigDatabase* database;
    TigDatabaseEntry* entry;
    FILE* underlying_stream;
    int pos;
    int compressed_pos;
    int ungotten;
    DecompressionContext* decompression_context;
    TigDatabaseFileHandle* next;
} TigDatabaseFileHandle;

static void tig_database_find_prepare(TigDatabaseFindFileData* ffd);
static void tig_database_load_ignored(TigDatabase* database);
static int num_path_segments(const char* path);
static int tig_database_find_entry_by_path(const void* a1, const void* a2);
static bool tig_database_fclose_internal(TigDatabaseFileHandle* stream);
static bool tig_database_fopen_internal(TigDatabase* database, TigDatabaseEntry* entry, const char* mode, TigDatabaseFileHandle* stream);
static int tig_database_fgetc_internal(TigDatabaseFileHandle* stream);
static bool tig_database_fread_internal(void* buffer, size_t size, TigDatabaseFileHandle* stream);

// 0x638BBC
static unsigned char tig_database_decompression_buffer[DECOMPRESSION_BUFFER_SIZE];

// 0x63CBC0
static TigDatabase* tig_database_open_databases_head;

// 0x53BC50
TigDatabase* tig_database_open(const char* path)
{
    FILE* stream;
    int id;
    int name_table_size;
    TigGuid guid;
    int entry_table_size;
    int entry_table_offset;
    TigDatabase* database;
    TigDatabaseEntry* entry;
    int offset;
    unsigned int index;
    int name_size;
    char* name;
    long pos;
    long size;

    stream = fopen(path, "rb");
    if (stream == NULL) {
        return NULL;
    }

    if (fseek(stream, -12, SEEK_END) != 0) {
        fclose(stream);
        return false;
    }

    if (fread(&id, sizeof(id), 1, stream) != 1) {
        fclose(stream);
        return false;
    }

    if (fread(&name_table_size, sizeof(name_table_size), 1, stream) != 1) {
        fclose(stream);
        return false;
    }

    if (id == FOURCC_DAT) {
        memset(&guid, 0, sizeof(guid));
    } else if (id == FOURCC_DAT1) {
        if (fseek(stream, -24, SEEK_END) != 0) {
            fclose(stream);
            return false;
        }

        if (fread(&guid, sizeof(guid), 1, stream) != 1) {
            fclose(stream);
            return false;
        }

        if (fread(&id, sizeof(id), 1, stream) != 1) {
            fclose(stream);
            return false;
        }
    } else {
        fclose(stream);
        return false;
    }

    if (fread(&entry_table_offset, sizeof(entry_table_offset), 1, stream) != 1) {
        fclose(stream);
        return false;
    }

    if (fseek(stream, -4 - entry_table_offset, SEEK_END) != 0) {
        fclose(stream);
        return false;
    }

    if (fread(&entry_table_size, sizeof(entry_table_size), 1, stream) != 1) {
        fclose(stream);
        return false;
    }

    pos = ftell(stream);
    if (pos == -1
        || fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return false;
    }

    size = ftell(stream);
    if (size == -1
        || fseek(stream, pos, SEEK_SET) != 0) {
        fclose(stream);
        return false;
    }

    offset = (int)size - entry_table_size - entry_table_offset;

    database = (TigDatabase*)MALLOC(sizeof(TigDatabase));

    if (fread(&(database->entries_count), sizeof(database->entries_count), 1, stream) != 1) {
        // FIX: Memory leak.
        FREE(database);
        fclose(stream);
        return NULL;
    }

    database->guid = guid;
    database->path = STRDUP(path);
    database->entries = (TigDatabaseEntry*)MALLOC(sizeof(TigDatabaseEntry) * database->entries_count);
    database->open_file_handles_head = NULL;
    database->name_table = (char*)MALLOC(name_table_size);

    name = database->name_table;
    for (index = 0; index < database->entries_count; index++) {
        if (fread(&name_size, sizeof(name_size), 1, stream) != 1) {
            break;
        }

        if (fread(name, name_size, 1, stream) != 1) {
            break;
        }

        entry = &(database->entries[index]);

        if (fseek(stream, 4, SEEK_CUR) != 0
            || fread(&(entry->flags), sizeof(entry->flags), 1, stream) != 1
            || fread(&(entry->size), sizeof(entry->size), 1, stream) != 1
            || fread(&(entry->compressed_size), sizeof(entry->compressed_size), 1, stream) != 1
            || fread(&(entry->offset), sizeof(entry->offset), 1, stream) != 1) {
            break;
        }

        compat_windows_path_to_native(name);
        SDL_strlwr(name);

        entry->path = name;
        entry->flags &= ~TIG_DATABASE_ENTRY_0x200;
        entry->flags |= TIG_DATABASE_ENTRY_0x100;
        entry->offset += offset;

        name += name_size;
    }

    fclose(stream);

    if (index < database->entries_count) {
        FREE(database->name_table);
        FREE(database->entries);
        FREE(database);
        return false;
    }

    database->next = tig_database_open_databases_head;
    tig_database_open_databases_head = database;

    tig_database_load_ignored(database);

    return database;
}

// 0x53BF40
bool tig_database_close(TigDatabase* database)
{
    TigDatabase* prev_database;
    TigDatabase* curr_database;
    TigDatabaseFileHandle* curr_file_handle;
    TigDatabaseFileHandle* next_file_handle;

    prev_database = NULL;
    curr_database = tig_database_open_databases_head;
    while (curr_database != NULL && curr_database != database) {
        prev_database = curr_database;
        curr_database = curr_database->next;
    }

    if (curr_database == NULL) {
        return false;
    }

    if (prev_database != NULL) {
        prev_database->next = curr_database->next;
    } else {
        tig_database_open_databases_head = curr_database->next;
    }

    curr_file_handle = database->open_file_handles_head;
    while (curr_file_handle != NULL) {
        next_file_handle = curr_file_handle->next;
        tig_database_fclose(curr_file_handle);
        curr_file_handle = next_file_handle;
    }

    FREE(database->name_table);
    FREE(database->entries);
    FREE(database->path);
    FREE(database);

    return true;
}

// 0x53BFD0
bool tig_database_find_first_entry(TigDatabase* database, const char* pattern, TigDatabaseFindFileData* ffd)
{
    memset(ffd, 0, sizeof(*ffd));

    ffd->database = database;
    ffd->pattern = STRDUP(pattern);
    ffd->path_segments_count = num_path_segments(pattern);

    compat_windows_path_to_native(ffd->pattern);
    SDL_strlwr(ffd->pattern);

    if (!fpattern_isvalid(pattern)) {
        ffd->index = database->entries_count;
        return false;
    }

    tig_database_find_prepare(ffd);

    return tig_database_find_next_entry(ffd);
}

// 0x53C030
void tig_database_find_prepare(TigDatabaseFindFileData* ffd)
{
    char* pch = ffd->pattern;
    char ch;
    int l;
    int r;
    int m;
    int cmp;

    while (*pch != '\0' && *pch != '*' && *pch != '?') {
        pch++;
    }

    ch = *pch;
    *pch = '\0';

    l = 0;
    r = ffd->database->entries_count - 1;

    // Use binary search to find index of the first entry that match the left
    // side of the pattern (or exact index if supplied pattern is exact file
    // name).
    while (l <= r) {
        m = (l + r) / 2;
        cmp = SDL_strcmp(ffd->database->entries[m].path, ffd->pattern);
        if (cmp < 0) {
            l = m + 1;
        } else if (cmp > 0) {
            r = m - 1;
        } else {
            // See below.
            l = m;
            break;
        }
    }

    *pch = ch;

    ffd->index = l;
}

// 0x53C0D0
bool tig_database_find_next_entry(TigDatabaseFindFileData* ffd)
{
    unsigned int index;

    index = ffd->index;
    if (index >= ffd->database->entries_count) {
        return false;
    }

    while (index < ffd->database->entries_count) {
        if (fpattern_matchn(ffd->pattern, ffd->database->entries[index].path)
            && num_path_segments(ffd->database->entries[index].path) == ffd->path_segments_count
            && (ffd->database->entries[index].flags & TIG_DATABASE_ENTRY_IGNORED) == 0) {
            // Found.
            ffd->name = ffd->database->entries[index].path;
            ffd->size = ffd->database->entries[index].size;
            ffd->is_directory = (ffd->database->entries[index].flags & TIG_DATABASE_ENTRY_DIRECTORY) != 0;

            break;
        }
        index++;
    }

    ffd->index = index + 1;

    return index < ffd->database->entries_count;
}

// 0x53C190
bool tig_database_find_close(TigDatabaseFindFileData* ffd)
{
    FREE(ffd->pattern);
    memset(ffd, 0, sizeof(*ffd));
    return true;
}

// 0x53C1C0
int tig_database_filelength(TigDatabaseFileHandle* stream)
{
    return stream->entry->size;
}

// 0x53C1D0
bool tig_database_get_entry(TigDatabase* database, const char* path, TigDatabaseEntry** entry_ptr)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    SDL_strlwr(native_path);

    *entry_ptr = (TigDatabaseEntry*)bsearch(native_path, database->entries, database->entries_count, sizeof(*database->entries), tig_database_find_entry_by_path);
    if (*entry_ptr == NULL) {
        return false;
    }

    if (((*entry_ptr)->flags & TIG_DATABASE_ENTRY_IGNORED) != 0) {
        *entry_ptr = NULL;
        return false;
    }

    return true;
}

// 0x53C220
int tig_database_fclose(TigDatabaseFileHandle* stream)
{
    int rc;

    rc = tig_database_fclose_internal(stream) ? 0 : -1;
    FREE(stream);

    return rc;
}

// 0x53C250
int tig_database_fflush(TigDatabaseFileHandle* stream)
{
    // Database does not support writing;
    (void)stream;

    return -1;
}

// 0x53C260
TigDatabaseFileHandle* tig_database_fopen(TigDatabase* database, const char* file_name, const char* mode)
{
    TigDatabaseEntry* entry;

    if (!tig_database_get_entry(database, file_name, &entry)) {
        return false;
    }

    return tig_database_fopen_entry(database, entry, mode);
}

// 0x53C2A0
TigDatabaseFileHandle* tig_database_reopen(TigDatabase* database, const char* path, const char* mode, TigDatabaseFileHandle* stream)
{
    tig_database_fclose_internal(stream);

    TigDatabaseEntry* entry;
    if (!tig_database_get_entry(database, path, &entry)) {
        FREE(stream);
        return NULL;
    }

    if (!tig_database_fopen_internal(database, entry, mode, stream)) {
        FREE(stream);
        return NULL;
    }

    return stream;
}

// 0x53C300
TigDatabaseFileHandle* tig_database_fopen_entry(TigDatabase* database, TigDatabaseEntry* entry, const char* mode)
{
    TigDatabaseFileHandle* stream;

    stream = (TigDatabaseFileHandle*)MALLOC(sizeof(*stream));
    if (!tig_database_fopen_internal(database, entry, mode, stream)) {
        FREE(stream);
        return NULL;
    }

    return stream;
}

// 0x53C340
int tig_database_setbuf(TigDatabaseFileHandle* stream, char* buffer)
{
    if (buffer != NULL) {
        return tig_database_setvbuf(stream, buffer, 0, 512);
    } else {
        return tig_database_setvbuf(stream, NULL, _IONBF, 512);
    }
}

// 0x53C370
int tig_database_setvbuf(TigDatabaseFileHandle* stream, char* buffer, int mode, size_t size)
{
    // Database does not support writing.
    (void)stream;
    (void)buffer;
    (void)mode;
    (void)size;

    return 1;
}

// 0x53C3A0
int tig_database_vfprintf(TigDatabaseFileHandle* stream, const char* fmt, va_list args)
{
    // Database does not support writing.
    (void)stream;
    (void)fmt;
    (void)args;

    return -1;
}

// 0x53C3B0
int tig_database_fgetc(TigDatabaseFileHandle* stream)
{
    int ch;

    ch = tig_database_fgetc_internal(stream);
    if (ch == -1) {
        return -1;
    }

    if ((stream->flags & TIG_DATABASE_FILE_TEXT_MODE) != 0 && ch == '\r') {
        ch = tig_database_fgetc_internal(stream);
        if (ch != '\n') {
            tig_database_ungetc(ch, stream);
        }
    }

    return ch;
}

// 0x53C3F0
char* tig_database_fgets(char* buffer, int max_count, TigDatabaseFileHandle* stream)
{
    int count = 0;
    int ch = '\0';

    while (count < max_count - 1 && ch != '\n') {
        ch = tig_database_fgetc(stream);
        if (ch == -1) {
            if (count == 0) {
                return NULL;
            }
        }

        buffer[count++] = (unsigned char)ch;
    }

    buffer[count] = '\0';

    return buffer;
}

// 0x53C440
int tig_database_fputc(int ch, TigDatabaseFileHandle* stream)
{
    // Database does not support writing.
    (void)ch;
    (void)stream;

    return -1;
}

// 0x53C450
int tig_database_fputs(const char* buffer, TigDatabaseFileHandle* stream)
{
    // Database does not support writing.
    (void)buffer;
    (void)stream;

    return -1;
}

// 0x53C460
int tig_database_ungetc(int ch, TigDatabaseFileHandle* stream)
{
    if (ch == -1) {
        return -1;
    }

    if ((stream->flags & (TIG_DATABASE_FILE_UNGOTTEN | TIG_DATABASE_FILE_ERROR)) != 0) {
        return -1;
    }

    stream->flags &= ~TIG_DATABASE_FILE_EOF;
    stream->flags |= TIG_DATABASE_FILE_UNGOTTEN;
    stream->ungotten = (unsigned char)ch;
    stream->pos--;

    return ch;
}

// 0x53C4A0
size_t tig_database_fread(void* buffer, size_t size, size_t count, TigDatabaseFileHandle* stream)
{
    size_t bytes_to_read;
    size_t bytes_read;
    unsigned char* byte_buffer = (unsigned char*)buffer;

    if (size == 0 || count == 0) {
        return 0;
    }

    bytes_to_read = size * count;
    bytes_read = 0;

    if (bytes_to_read > stream->entry->size - stream->pos) {
        bytes_to_read = stream->entry->size - stream->pos;
        stream->flags |= TIG_DATABASE_FILE_EOF;
    }

    if ((stream->flags & TIG_DATABASE_FILE_UNGOTTEN) != 0) {
        stream->flags &= ~TIG_DATABASE_FILE_UNGOTTEN;
        stream->pos++;
        *byte_buffer++ = (unsigned char)stream->ungotten;
        bytes_to_read--;
        bytes_read++;
    }

    if (tig_database_fread_internal(byte_buffer, bytes_to_read, stream)) {
        bytes_read += bytes_to_read;
    }

    return bytes_read / size;
}

// 0x53C520
size_t tig_database_fwrite(const void* buffer, size_t size, size_t count, TigDatabaseFileHandle* stream)
{
    // Database does not support writing.
    (void)buffer;
    (void)size;
    (void)stream;

    return count - 1;
}

// 0x53C530
int tig_database_fgetpos(TigDatabaseFileHandle* stream, int* off)
{
    *off = tig_database_ftell(stream);

    // FIXME: `fgetpos` should return 0 on success.
    return 1;
}

// 0x53C550
int tig_database_fseek(TigDatabaseFileHandle* stream, int offset, int origin)
{
    int pos;

    if ((stream->flags & TIG_DATABASE_FILE_ERROR) != 0) {
        return 1;
    }

    // Streams opened in text mode can only be rewinded to the beginning.
    if ((stream->flags & TIG_DATABASE_FILE_TEXT_MODE) != 0
        && offset != 0
        && origin != SEEK_SET) {
        return 1;
    }

    switch (origin) {
    case SEEK_SET:
        pos = offset;
        break;
    case SEEK_CUR:
        pos = offset + stream->pos;
        break;
    case SEEK_END:
        pos = offset + stream->entry->size;
        break;
    default:
        return 1;
    }

    // FIX: Original code treats seeking to the end of stream as error.
    if (pos < 0 || (unsigned int)pos > stream->entry->size) {
        stream->flags |= TIG_DATABASE_FILE_ERROR;
        return 1;
    }

    if ((stream->entry->flags & TIG_DATABASE_ENTRY_PLAIN) != 0) {
        if (fseek(stream->underlying_stream, stream->entry->offset + pos, SEEK_SET) != 0) {
            stream->flags |= TIG_DATABASE_FILE_ERROR;
            return 1;
        }
    } else if ((stream->entry->flags & TIG_DATABASE_ENTRY_COMPRESSED) != 0) {
        unsigned int bytes_to_skip;

        if (pos < stream->pos) {
            if (fseek(stream->underlying_stream, stream->entry->offset, SEEK_SET) != 0) {
                stream->flags |= TIG_DATABASE_FILE_ERROR;
                return 1;
            }

            stream->compressed_pos = 0;
            stream->pos = 0;
            inflateEnd(&(stream->decompression_context->zstrm));

            stream->decompression_context->zstrm.next_in = NULL;
            stream->decompression_context->zstrm.avail_in = 0;
            stream->decompression_context->zstrm.next_out = NULL;
            stream->decompression_context->zstrm.avail_out = 0;

            if (inflateInit(&(stream->decompression_context->zstrm)) != Z_OK) {
                stream->flags |= TIG_DATABASE_FILE_ERROR;
                return 1;
            }
        }

        bytes_to_skip = pos - stream->pos;
        while (bytes_to_skip >= DECOMPRESSION_BUFFER_SIZE) {
            if (!tig_database_fread_internal(tig_database_decompression_buffer, DECOMPRESSION_BUFFER_SIZE, stream)) {
                return 1;
            }

            bytes_to_skip -= DECOMPRESSION_BUFFER_SIZE;
        }

        if (bytes_to_skip > 0) {
            if (!tig_database_fread_internal(tig_database_decompression_buffer, bytes_to_skip, stream)) {
                return 1;
            }
        }
    }

    stream->pos = pos;
    stream->flags &= ~(TIG_DATABASE_FILE_UNGOTTEN | TIG_DATABASE_FILE_EOF);
    return 0;
}

// 0x53C6C0
int tig_database_fsetpos(TigDatabaseFileHandle* stream, int off)
{
    return tig_database_fseek(stream, off, SEEK_SET);
}

// 0x53C6E0
int tig_database_ftell(TigDatabaseFileHandle* stream)
{
    return stream->pos;
}

// 0x53C6F0
void tig_database_rewind(TigDatabaseFileHandle* stream)
{
    tig_database_clearerr(stream);
    tig_database_fseek(stream, 0, SEEK_SET);
}

// 0x53C710
void tig_database_clearerr(TigDatabaseFileHandle* stream)
{
    stream->flags &= ~(TIG_DATABASE_FILE_EOF | TIG_DATABASE_FILE_ERROR);
}

// 0x53C720
int tig_database_feof(TigDatabaseFileHandle* stream)
{
    return stream->flags & TIG_DATABASE_FILE_EOF;
}

// 0x53C730
int tig_database_ferror(TigDatabaseFileHandle* stream)
{
    return stream->flags & TIG_DATABASE_FILE_ERROR;
}

// 0x53CCE0
void tig_database_load_ignored(TigDatabase* database)
{
    char path[TIG_MAX_PATH];
    SDL_PathInfo path_info;
    FILE* stream;
    char file_name[TIG_MAX_PATH];
    size_t file_name_length;
    int file_name_segments_count;
    unsigned int index;

    strcpy(path, database->path);
    strcat(path, ".ignore");

    if (!SDL_GetPathInfo(path, &path_info)) {
        return;
    }

    stream = fopen(path, "rt");
    if (stream == NULL) {
        return;
    }

    while (fgets(file_name, sizeof(file_name), stream) != NULL) {
        file_name_length = strlen(file_name);
        if (file_name[file_name_length - 1] == '\n') {
            file_name[file_name_length - 1] = '\0';
        }

        compat_windows_path_to_native(file_name);
        SDL_strlwr(file_name);

        if (fpattern_isvalid(file_name)) {
            file_name_segments_count = num_path_segments(file_name);

            for (index = 0; index < database->entries_count; index++) {
                if (fpattern_matchn(file_name, database->entries[index].path)
                    && num_path_segments(database->entries[index].path) == file_name_segments_count) {
                    database->entries[index].flags |= TIG_DATABASE_ENTRY_IGNORED;
                }
            }
        }
    }

    fclose(stream);
}

// 0x53CE80
int num_path_segments(const char* path)
{
    int count;
    const char* pch;

    count = 0;
    pch = path;
    while (*pch != '\0') {
        if (*pch == '\\' || *pch == '/') {
            count++;
        }
        pch++;
    }

    return count;
}

// 0x53CEB0
int tig_database_find_entry_by_path(const void* a1, const void* a2)
{
    const char* path = (const char*)a1;
    const TigDatabaseEntry* entry = (const TigDatabaseEntry*)a2;
    return SDL_strcmp(path, entry->path);
}

// 0x53CED0
bool tig_database_fclose_internal(TigDatabaseFileHandle* stream)
{
    TigDatabaseFileHandle* prev;
    TigDatabaseFileHandle* curr;

    prev = NULL;
    curr = stream->database->open_file_handles_head;
    while (curr != NULL && curr != stream) {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        return false;
    }

    if (prev != NULL) {
        prev->next = curr->next;
    } else {
        stream->database->open_file_handles_head = curr->next;
    }

    fclose(stream->underlying_stream);

    if ((stream->entry->flags & TIG_DATABASE_ENTRY_COMPRESSED) != 0) {
        inflateEnd(&(stream->decompression_context->zstrm));
        FREE(stream->decompression_context);
    }

    memset(stream, 0, sizeof(*stream));

    return true;
}

// 0x53CF50
bool tig_database_fopen_internal(TigDatabase* database, TigDatabaseEntry* entry, const char* mode, TigDatabaseFileHandle* stream)
{
    if (mode[0] == 'w') {
        return false;
    }

    memset(stream, 0, sizeof(*stream));

    stream->underlying_stream = fopen(database->path, "rb");
    if (stream->underlying_stream == NULL) {
        return false;
    }

    if (fseek(stream->underlying_stream, entry->offset, SEEK_SET) != 0) {
        return false;
    }

    stream->entry = entry;

    if (mode[1] == 't') {
        stream->flags |= TIG_DATABASE_FILE_TEXT_MODE;
    }

    if ((entry->flags & TIG_DATABASE_ENTRY_COMPRESSED) != 0) {
        stream->decompression_context = (DecompressionContext*)MALLOC(sizeof(DecompressionContext));
        stream->decompression_context->zstrm.next_in = stream->decompression_context->buffer;
        stream->decompression_context->zstrm.avail_in = 0;
        stream->decompression_context->zstrm.zalloc = Z_NULL;
        stream->decompression_context->zstrm.zfree = Z_NULL;
        stream->decompression_context->zstrm.opaque = Z_NULL;

        if (inflateInit(&(stream->decompression_context->zstrm)) != Z_OK) {
            FREE(stream->decompression_context);
            fclose(stream->underlying_stream);
            return false;
        }
    }

    stream->database = database;
    stream->next = database->open_file_handles_head;
    database->open_file_handles_head = stream;

    return true;
}

// 0x53D040
int tig_database_fgetc_internal(TigDatabaseFileHandle* stream)
{
    unsigned char ch;

    if ((stream->flags & TIG_DATABASE_FILE_UNGOTTEN) != 0) {
        stream->flags &= ~TIG_DATABASE_FILE_UNGOTTEN;
        stream->pos++;
        return stream->ungotten;
    }

    if ((unsigned int)stream->pos >= stream->entry->size) {
        stream->flags |= TIG_DATABASE_FILE_EOF;
    }

    if ((stream->flags & (TIG_DATABASE_FILE_EOF | TIG_DATABASE_FILE_ERROR)) != 0) {
        return -1;
    }

    if (!tig_database_fread_internal(&ch, sizeof(ch), stream)) {
        return -1;
    }

    return ch;
}

// 0x53D0A0
bool tig_database_fread_internal(void* buffer, size_t size, TigDatabaseFileHandle* stream)
{
    size_t bytes_to_read;
    int rc;

    if ((stream->entry->flags & TIG_DATABASE_ENTRY_PLAIN) != 0) {
        if (fread(buffer, size, 1, stream->underlying_stream) != 1) {
            stream->flags |= TIG_DATABASE_FILE_ERROR;
            return false;
        }
    } else if ((stream->entry->flags & TIG_DATABASE_ENTRY_COMPRESSED) != 0) {
        stream->decompression_context->zstrm.next_out = (Bytef*)buffer;
        stream->decompression_context->zstrm.avail_out = size;

        while (stream->decompression_context->zstrm.avail_out != 0) {
            if (stream->decompression_context->zstrm.avail_in == 0) {
                // No more unprocessed data, request next chunk.
                bytes_to_read = stream->entry->compressed_size - stream->compressed_pos;
                if (bytes_to_read > DECOMPRESSION_BUFFER_SIZE) {
                    bytes_to_read = DECOMPRESSION_BUFFER_SIZE;
                }

                if (fread(stream->decompression_context->buffer, bytes_to_read, 1, stream->underlying_stream) != 1) {
                    stream->flags |= TIG_DATABASE_FILE_ERROR;
                    return false;
                }

                stream->compressed_pos += bytes_to_read;
                stream->decompression_context->zstrm.avail_in = bytes_to_read;
                stream->decompression_context->zstrm.next_in = (Bytef*)stream->decompression_context->buffer;
            }

            rc = inflate(&(stream->decompression_context->zstrm), Z_NO_FLUSH);
            if (rc != Z_OK && rc != Z_STREAM_END) {
                stream->flags |= TIG_DATABASE_FILE_ERROR;
                return false;
            }
        }
    }

    stream->pos += size;

    return true;
}
