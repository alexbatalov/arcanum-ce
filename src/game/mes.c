#include "game/mes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tig/tig.h>

typedef struct MesFile {
    /* 0000 */ char path[TIG_MAX_PATH];
    /* 0104 */ int refcount;
    /* 0108 */ int num_entries;
    /* 010C */ int max_entries;
    /* 0110 */ MesFileEntry* entries;
    /* 0114 */ char* data;
    /* 0118 */ size_t size;
} MesFile;

static bool find_mes_file(const char* path, int* index_ptr);
static void mes_load_internal(MesFile* mes_file);
static bool parse_entry(TigFile* stream, MesFileEntry* mes_file_entry);
static bool parse_field(TigFile* stream, char* buffer);
static TigFile* mes_file_open(const char* path, const char* mode);
static void mes_file_close(TigFile* stream);
static int consume_next_char(TigFile* stream);
static int compare_mes_file_entry(const void* a, const void* b);
static void copy_mes_file_entry(MesFileEntry* dst, MesFileEntry* src, char* str);
static void check_duplicates(MesFile* mes_file);

/**
 * Current line number during parsing, used for error reporting.
 *
 * 0x6018DC
 */
static int mes_file_parse_line;

/**
 * Buffer for storing a single line during parsing.
 *
 * 0x6018E0
 */
static char mes_file_parse_buffer[MAX_STRING];

/**
 * Path of the currently parsed message file, used for error reporting.
 *
 * 0x6020B0
 */
static const char* current_mes_file_path;

/**
 * The length of string in the parsing buffer (`mes_file_parse_buffer`).
 *
 * 0x6020B4
 */
static int mes_file_parse_len;

/**
 * Buffer for storing error message when a requested message number of not
 * found.
 *
 * See `mes_get_msg`.
 *
 * 0x6020B8
 */
static char mes_tmp_str[MAX_STRING];

/**
 * Current position in the parsing buffer.
 *
 * 0x602888
 */
static int mes_file_parse_pos;

/**
 * The number of loaded message files in `mes_files`.
 *
 * 0x60288C
 */
static int mes_files_length;

/**
 * The capacity of the `mes_files` array.
 *
 * 0x602890
 */
static int mes_files_capacity;

/**
 * Array of loaded message files.
 *
 * 0x602894
 */
static MesFile* mes_files;

/**
 * Loads a message file and assigns a handle for accessing it.
 *
 * 0x4D4290
 */
bool mes_load(const char* path, mes_file_handle_t* mes_file_handle_ptr)
{
    int index;
    MesFile mes_file;

    // Check if the file is already loaded.
    if (find_mes_file(path, &index)) {
        // File has already been loaded, increment refcount.
        mes_files[index].refcount++;
        *mes_file_handle_ptr = (mes_file_handle_t)index;
        return true;
    }

    // Initialize new mes file object.
    strcpy(mes_file.path, path);
    mes_file.refcount = 1;
    mes_file.num_entries = 0;
    mes_file.max_entries = 0;
    mes_file.entries = NULL;

    // Keep current path for error reporting.
    current_mes_file_path = path;
    mes_load_internal(&mes_file);

    // Check if any entries were loaded.
    if (mes_file.num_entries == 0) {
        if (mes_file_handle_ptr != NULL) {
            *mes_file_handle_ptr = MES_FILE_HANDLE_INVALID;
        }
        return false;
    }

    mes_files[index] = mes_file;
    mes_files_length++;

    *mes_file_handle_ptr = (mes_file_handle_t)index;

    return true;
}

/**
 * Retrieves a message entry by its number from a mes file.
 *
 * 0x4D43A0
 */
void mes_get_msg(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    MesFileEntry* found;

    // Search for the entry using binary search.
    found = bsearch(mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry);

    // Set an error message if not found.
    if (found == NULL) {
        sprintf(mes_tmp_str, "Error! Missing line %d in %s",
            mes_file_entry->num,
            mes_files[mes_file_handle].path);
        mes_file_entry->str = mes_tmp_str;
        return;
    }

    *mes_file_entry = *found;
}

/**
 * Retrieves the first message entry from a mes file.
 *
 * 0x4D4420
 */
bool mes_find_first(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    if (mes_files[mes_file_handle].num_entries == 0) {
        return false;
    }

    *mes_file_entry = mes_files[mes_file_handle].entries[0];

    return true;
}

/**
 * Retrieves the next message entry from a mes file.
 *
 * 0x4D4460
 */
bool mes_find_next(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    MesFileEntry* found;
    int exists;
    int index;

    // Search for the current entry to find its index.
    found = tig_bsearch(mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry,
        &exists);

    index = (int)(found - mes_files[mes_file_handle].entries);
    if (exists) {
        // Move to the next entry if the current one exists.
        index++;
    }

    // Check range.
    if (index >= mes_files[mes_file_handle].num_entries) {
        return false;
    }

    *mes_file_entry = mes_files[mes_file_handle].entries[index];

    return true;
}

/**
 * Returns the total number of entries in a mes file.
 *
 * 0x4D44E0
 */
int mes_entries_count(mes_file_handle_t mes_file_handle)
{
    return mes_files[mes_file_handle].num_entries;
}

/**
 * Retrieves a message entry by its index in a mes file.
 *
 * 0x4D4500
 */
bool mes_get_entry(mes_file_handle_t mes_file_handle, int index, MesFileEntry* mes_file_entry)
{
    if (index < 0 || index >= mes_files[mes_file_handle].num_entries) {
        return false;
    }

    *mes_file_entry = mes_files[mes_file_handle].entries[index];

    return true;
}

/**
 * Counts the number of entries within a specified range of entry numbers.
 *
 * 0x4D4550
 */
int mes_entries_count_in_range(mes_file_handle_t mes_file_handle, int start, int end)
{
    MesFileEntry mes_file_entry;
    MesFileEntry* found;
    int index;
    int cnt = 0;

    // Search for the starting entry.
    mes_file_entry.num = start;
    found = bsearch(&mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry);

    if (found != NULL) {
        // Count entries from the starting point until the end number is exceeded.
        index = (int)(found - mes_files[mes_file_handle].entries);
        while (index < mes_files[mes_file_handle].num_entries) {
            if (mes_files[mes_file_handle].entries[index].num > end) {
                break;
            }
            index++;
            cnt++;
        }
    }

    return cnt;
}

/**
 * Searches for a message entry by its number in a mes file.
 *
 * 0x4D45E0
 */
bool mes_search(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    MesFileEntry* found;

    // Search for the entry using binary search.
    found = bsearch(mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry);

    if (found == NULL) {
        return false;
    }

    *mes_file_entry = *found;

    return true;
}

/**
 * Unloads a mes file, freeing resources if no references remain.
 *
 * 0x4D4630
 */
bool mes_unload(mes_file_handle_t mes_file_handle)
{
    if (mes_file_handle != MES_FILE_HANDLE_INVALID) {
        // Decrease the reference count.
        mes_files[mes_file_handle].refcount--;

        // Free resources if no references remain.
        if (mes_files[mes_file_handle].refcount == 0) {
            FREE(mes_files[mes_file_handle].entries);
            FREE(mes_files[mes_file_handle].data);
            mes_files_length--;
        }

        // Free the global array if no open files remain.
        if (mes_files_length == 0) {
            FREE(mes_files);
            mes_files = NULL;
            mes_files_capacity = 0;
        }
    }

    return true;
}

/**
 * Duplicate of `mes_entries_count`.
 *
 * 0x4D46D0
 */
int mes_num_entries(mes_file_handle_t mes_file_handle)
{
    return mes_files[mes_file_handle].num_entries;
}

/**
 * Merges entries from one mes file into another.
 *
 * 0x4D46F0
 */
void mes_merge(mes_file_handle_t dst, mes_file_handle_t src)
{
    int index;
    char* data;
    MesFile* dst_mes_file = &(mes_files[dst]);
    MesFile* src_mes_file = &(mes_files[src]);

    // Keep the original pointer for pointer math (see below).
    data = dst_mes_file->data;

    // Expand the data buffer and entries array to accommodate both files' data.
    dst_mes_file->data = (char*)REALLOC(dst_mes_file->data,
        dst_mes_file->size + src_mes_file->size);
    dst_mes_file->entries = (MesFileEntry*)REALLOC(dst_mes_file->entries,
        sizeof(MesFileEntry) * (dst_mes_file->num_entries + src_mes_file->num_entries));

    // Append data and entries from the source file.
    memcpy(&(dst_mes_file->data[dst_mes_file->size]),
        src_mes_file->data,
        src_mes_file->size);
    memcpy(&(dst_mes_file->entries[dst_mes_file->num_entries]),
        src_mes_file->entries,
        sizeof(MesFileEntry) * src_mes_file->num_entries);

    // Fix string pointers in the destination entries.
    for (index = 0; index < dst_mes_file->num_entries; index++) {
        dst_mes_file->entries[index].str += dst_mes_file->data - data;
    }

    // Adjust string pointers in the copied source entries.
    for (index = 0; index < src_mes_file->num_entries; index++) {
        dst_mes_file->entries[dst_mes_file->num_entries + index].str += dst_mes_file->data
            + dst_mes_file->size
            - src_mes_file->data;
    }

    // Update the destination file's size and entry count.
    dst_mes_file->size += src_mes_file->size;
    dst_mes_file->num_entries += src_mes_file->num_entries;

    // Sort the combined entries (since all lookups use binary search).
    if (dst_mes_file->num_entries != 0) {
        qsort(dst_mes_file->entries,
            dst_mes_file->num_entries,
            sizeof(MesFileEntry),
            compare_mes_file_entry);
    }

    // Check for duplicate entry numbers.
    check_duplicates(dst_mes_file);
}

/**
 * Searches for an existing message file by path and returns its index.
 *
 * Returns `true` if the file is already loaded, `false` otherwise.
 *
 * 0x4D4840
 */
bool find_mes_file(const char* path, int* index_ptr)
{
    int index;
    int candidate = -1;

    // Iterate through the message files array to find a matching path or a
    // first free slot.
    for (index = 0; index < mes_files_capacity; index++) {
        if (mes_files[index].refcount != 0) {
            if (SDL_strcasecmp(path, mes_files[index].path) == 0) {
                *index_ptr = index;
                return true;
            }
        } else {
            if (candidate == -1) {
                candidate = index;
            }
        }
    }

    // Use the free slot if found.
    if (candidate != -1) {
        *index_ptr = candidate;
        return false;
    }

    *index_ptr = index;

    // No free slot is available, expand message files array.
    mes_files_capacity += 10;
    mes_files = (MesFile*)REALLOC(mes_files, sizeof(MesFile) * mes_files_capacity);

    // Initialize new slots with zero refcount.
    while (index < mes_files_capacity) {
        mes_files[index++].refcount = 0;
    }

    return false;
}

/**
 * Internal helper that loads and parses a message file.
 *
 * 0x4D4920
 */
void mes_load_internal(MesFile* mes_file)
{
    TigFile* stream;
    MesFileEntry mes_file_entry;
    char buffer[MAX_STRING];
    int offset;

    stream = mes_file_open(mes_file->path, "rt");
    if (stream == NULL) {
        return;
    }

    // Allocate a buffer for the file's data based on its size. The actual size
    // for string data is smaller because it does not include comments, braces,
    // or new lines (unless they are part of a message entry).
    mes_file->size = tig_file_filelength(stream);
    mes_file->data = (char*)MALLOC(mes_file->size);

    mes_file_entry.str = buffer;

    offset = 0;
    mes_file_parse_line = 1;

    // Parse entries until the end of the file.
    while (parse_entry(stream, &mes_file_entry)) {
        // Expand the entries array if necessary.
        if (mes_file->num_entries == mes_file->max_entries) {
            mes_file->max_entries += 10;
            mes_file->entries = (MesFileEntry*)REALLOC(mes_file->entries,
                sizeof(MesFileEntry) * mes_file->max_entries);
        }

        // Copy the parsed entry to the entries array and update the data buffer.
        copy_mes_file_entry(&(mes_file->entries[mes_file->num_entries]),
            &mes_file_entry,
            &(mes_file->data[offset]));
        offset += (int)strlen(mes_file_entry.str) + 1;
        mes_file->num_entries++;
    }

    // Update the actual size of the data buffer.
    mes_file->size = offset;
    mes_file_close(stream);

    // Sort entries by number (since all lookups use binary search).
    if (mes_file->num_entries != 0) {
        qsort(mes_file->entries,
            mes_file->num_entries,
            sizeof(MesFileEntry),
            compare_mes_file_entry);
    }

    // Check for duplicate entry numbers.
    check_duplicates(mes_file);
}

/**
 * Parses a single entry from the file stream.
 *
 * 0x4D4A60
 */
bool parse_entry(TigFile* stream, MesFileEntry* mes_file_entry)
{
    char buffer[MAX_STRING];

    // Parse the number field.
    if (!parse_field(stream, buffer)) {
        return false;
    }

    // Validate the number field.
    if (!SDL_isdigit(buffer[0])) {
        tig_debug_printf("Warning! Bad number field on line: %d (%s)\n",
            mes_file_parse_line,
            current_mes_file_path);
    }

    mes_file_entry->num = atoi(buffer);

    // Parse the text field.
    if (!parse_field(stream, buffer)) {
        tig_debug_printf("Warning! Missing text on line: %d (%s)\n",
            mes_file_parse_line,
            current_mes_file_path);
        return false;
    }

    strcpy(mes_file_entry->str, buffer);

    return true;
}

/**
 * Parses a single field enclosed in braces from the file stream.
 *
 * 0x4D4B50
 */
bool parse_field(TigFile* stream, char* buffer)
{
    int ch;
    int pos;
    bool too_long;

    // Skip characters until an opening brace is found.
    ch = consume_next_char(stream);
    while (ch != '{') {
        if (ch == EOF) {
            return false;
        }

        if (ch == '\n') {
            // Track line numbers for error reporting.
            mes_file_parse_line++;
        } else if (ch == '}') {
            tig_debug_printf("Warning! Possible missing left brace { on or before line %d (%s)\n",
                mes_file_parse_line,
                current_mes_file_path);
        }

        ch = consume_next_char(stream);
    }

    pos = 0;
    too_long = false;

    // Read characters until a closing brace is found.
    ch = consume_next_char(stream);
    while (ch != '}') {
        if (ch == EOF) {
            tig_debug_printf("Warning! Expected right bracket }, reached end of file (%s).\n",
                current_mes_file_path);
            return false;
        }

        if (ch == '\n') {
            // Track line numbers for error reporting.
            mes_file_parse_line++;
        } else if (ch == '{') {
            tig_debug_printf("Warning! Possible missing right brace } on or before line %d (%s)\n",
                mes_file_parse_line,
                current_mes_file_path);
        }

        // Store character in buffer if there's space.
        if (pos < MAX_STRING) {
            buffer[pos++] = (char)ch;
        } else {
            too_long = true;
        }

        ch = consume_next_char(stream);
    }

    buffer[pos] = '\0';

    // Log a warning if the string was too long.
    if (too_long) {
        tig_debug_printf("Warning! String too long on line: %d (%s)\n",
            mes_file_parse_line,
            current_mes_file_path);
    }

    return true;
}

/**
 * Opens a mes file and initializes parsing state.
 *
 * 0x4D4C70
 */
TigFile* mes_file_open(const char* path, const char* mode)
{
    mes_file_parse_pos = 0;
    mes_file_parse_len = 0;
    return tig_file_fopen(path, mode);
}

/**
 * Closes a mes file stream.
 *
 * 0x4D4C90
 */
void mes_file_close(TigFile* stream)
{
    tig_file_fclose(stream);
}

/**
 * Reads the next character from the file stream, buffering lines as needed.
 *
 * 0x4D4CA0
 */
int consume_next_char(TigFile* stream)
{
    // Refill the buffer if the current position reaches the end.
    if (mes_file_parse_pos == mes_file_parse_len) {
        mes_file_parse_pos = 0;
        mes_file_parse_len = 0;

        if (tig_file_fgets(mes_file_parse_buffer, sizeof(mes_file_parse_buffer), stream) == NULL) {
            return EOF;
        }

        mes_file_parse_len = (int)strlen(mes_file_parse_buffer);
        if (mes_file_parse_len == 0) {
            return EOF;
        }
    }

    return mes_file_parse_buffer[mes_file_parse_pos++];
}

/**
 * Compares two `MesFileEntry` structures by their num field for sorting.
 *
 * 0x4D4D10
 */
int compare_mes_file_entry(const void* a, const void* b)
{
    const MesFileEntry* entry_a = (const MesFileEntry*)a;
    const MesFileEntry* entry_b = (const MesFileEntry*)b;

    if (entry_a->num < entry_b->num) {
        return -1;
    } else if (entry_a->num > entry_b->num) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Copies a `MesFileEntry` and the associated string to a new location.
 *
 * 0x4D4D30
 */
void copy_mes_file_entry(MesFileEntry* dst, MesFileEntry* src, char* str)
{
    *dst = *src;
    dst->str = str;
    strcpy(str, src->str);
}

/**
 * Checks for duplicate entry numbers in a mes file and logs appropriate
 * warnings.
 *
 * 0x4D4D70
 */
void check_duplicates(MesFile* mes_file)
{
    int index1;
    int index2;

    // NOTE: It seems the Troika developers were in a hurry when writing this
    // check. This is a quick-and-dirty O(n^2) algorithm, which may be slow for
    // large files. Since the entries are guaranteed to be sorted, duplicates
    // should be adjacent, which is O(n).
    for (index1 = 0; index1 < mes_file->num_entries; index1++) {
        for (index2 = 0; index2 < index1; index2++) {
            if (mes_file->entries[index1].num == mes_file->entries[index2].num) {
                tig_debug_printf("%s: two lines numbered %d\n",
                    mes_file->path,
                    mes_file->entries[index1].num);
            }
        }
    }
}

/**
 * Prints statistics about currently loaded mes files.
 *
 * 0x4D4DD0
 */
void mes_stats(void)
{
    int index;

    for (index = 0; index < mes_files_capacity; index++) {
        if (mes_files[index].refcount > 0) {
            tig_debug_printf("Open Msg File: \"%s\", ref_count=%d.\n",
                mes_files[index].path,
                mes_files[index].refcount);
        }
    }
}
