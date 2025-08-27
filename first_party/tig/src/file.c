#include "tig/file.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fpattern/fpattern.h>

#include "tig/compat.h"
#include "tig/core.h"
#include "tig/database.h"
#include "tig/debug.h"
#include "tig/find_file.h"
#include "tig/memory.h"

#define CACHE_DIR_NAME "TIGCache"
#define COPY_BUFFER_SIZE 0x8000

#define TIG_FILE_DATABASE 0x01
#define TIG_FILE_PLAIN 0x02
#define TIG_FILE_DELETE_ON_CLOSE 0x04

typedef struct TigFile {
    /* 0000 */ char* path;
    /* 0004 */ unsigned int flags;
    /* 0008 */ union {
        TigDatabaseFileHandle* database_file_stream;
        FILE* plain_file_stream;
    } impl;
} TigFile;

#define TIG_FILE_REPOSITORY_DATABASE 0x1
#define TIG_FILE_REPOSITORY_DIRECTORY 0x2

typedef struct TigFileRepository {
    int type;
    char* path;
    TigDatabase* database;
    struct TigFileRepository* next;
} TigFileRepository;

#define TIG_FILE_IGNORE_DATABASE 0x1
#define TIG_FILE_IGNORE_DIRECTORY 0x2

typedef struct TigFileIgnore {
    /* 0000 */ char* path;
    /* 0004 */ unsigned int flags;
    /* 0008 */ struct TigFileIgnore* next;
} TigFileIgnore;

static bool tig_file_mkdir_native(const char* path);
static bool tig_file_rmdir_native(const char* path);
static bool tig_file_empty_directory_native(const char* path);
static bool tig_file_is_empty_directory_native(const char* path);
static bool tig_file_is_directory_native(const char* path);
static bool tig_file_archive_native(const char* dst, const char* src);
static bool tig_file_unarchive_native(const char* src, const char* dst);
static bool copy_file_path(const char* dst, const char* src);
static bool copy_file_stream(TigFile* dst_stream, TigFile* src_stream);
static bool copy_file_stream_size(TigFile* dst_stream, TigFile* src_stream, size_t size);
static bool tig_file_archive_worker_native(const char* path, TigFile* stream1, TigFile* stream2);
static bool tig_file_repository_add_native(const char* path);
static bool tig_file_repository_remove_native(const char* file_name);
static int tig_file_mkdir_ex_native(const char* path);
static int tig_file_rmdir_ex_native(const char* path);
static bool tig_file_extract_native(const char* filename, char* path);
static void tig_file_list_create_native(TigFileList* list, const char* pattern);
static bool tig_file_exists_native(const char* file_name, TigFileInfo* info);
static bool tig_file_exists_in_path_native(const char* search_path, const char* file_name, TigFileInfo* info);
static int tig_file_remove_native(const char* file_name);
static int tig_file_rename_native(const char* old_file_name, const char* new_file_name);
static TigFile* tig_file_fopen_native(const char* path, const char* mode);
static TigFile* tig_file_reopen_native(const char* path, const char* mode, TigFile* stream);
static TigFile* tig_file_create();
static void tig_file_destroy(TigFile* stream);
static bool tig_file_close_internal(TigFile* stream);
static int tig_file_open_internal_native(const char* path, const char* mode, TigFile* stream);
static void tig_file_process_attribs(SDL_PathType type, unsigned int* flags);
static void tig_file_list_add(TigFileList* list, TigFileInfo* info);
static bool sub_5310C0(const char* filename, const void* owner, size_t size);
static unsigned int tig_file_ignored(const char* path);
static bool tig_file_copy_native(const char* src, const char* dst);
static bool tig_file_copy_internal(TigFile* dst, TigFile* src);
static int tig_file_rmdir_recursively_native(const char* path);

// 0x62B2A8
static TigFileIgnore* off_62B2A8;

// 0x62B2AC
static TigFileRepository* tig_file_repositories_head;

// 0x62B2B0
static TigFileIgnore* tig_file_ignore_head;

// 0x52DFE0
bool tig_file_mkdir_native(const char* path)
{
    return tig_file_mkdir_ex_native(path) == 0;
}

// 0x52E000
bool tig_file_rmdir_native(const char* path)
{
    if (!tig_file_is_directory(path)) {
        return false;
    }

    if (!tig_file_is_empty_directory(path)) {
        return false;
    }

    if (!tig_file_rmdir_ex(path)) {
        return false;
    }

    return true;
}

// 0x52E040
bool tig_file_empty_directory_native(const char* path)
{
    bool success;
    char pattern[TIG_MAX_PATH];
    TigFileList list;
    unsigned int index;

    if (!tig_file_is_directory_native(path)) {
        return false;
    }

    success = true;

    compat_join_path(pattern, sizeof(pattern), path, "*.*");
    tig_file_list_create_native(&list, pattern);

    for (index = 0; index < list.count; index++) {
        compat_join_path(pattern, sizeof(pattern), path, list.entries[index].path);
        if ((list.entries[index].attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0) {
            if (strcmp(list.entries[index].path, ".") != 0
                && strcmp(list.entries[index].path, "..") != 0) {
                if (!tig_file_empty_directory(pattern)) {
                    success = false;
                }

                if (tig_file_rmdir_ex_native(pattern) != 0) {
                    success = false;
                }
            }
        } else {
            if (tig_file_remove_native(pattern) != 0) {
                success = false;
            }
        }
    }

    tig_file_list_destroy(&list);
    return success;
}

// 0x52E1B0
bool tig_file_is_empty_directory_native(const char* path)
{
    char pattern[TIG_MAX_PATH];
    TigFileList list;
    bool is_empty;

    if (!tig_file_is_directory_native(path)) {
        return false;
    }

    compat_join_path(pattern, sizeof(pattern), path, "*.*");

    tig_file_list_create_native(&list, pattern);

    // Only contains trivia (`.` and `..`).
    is_empty = list.count <= 2;

    tig_file_list_destroy(&list);

    return is_empty;
}

// 0x52E220
bool tig_file_is_directory_native(const char* path)
{
    TigFileInfo info;

    return tig_file_exists(path, &info) && (info.attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0;
}

// 0x52E260
bool tig_file_copy_directory(const char* dst, const char* src)
{
    char path1[TIG_MAX_PATH];
    char path2[TIG_MAX_PATH];
    TigFileList list;
    unsigned int index;

    sprintf(path1, "%s\\*.*", src);
    tig_file_list_create(&list, path1);

    if (list.count != 0) {
        if (!tig_file_mkdir(dst)) {
            // FIXME: Leaks `list`.
            return false;
        }

        for (index = 0; index < list.count; index++) {
            sprintf(path1, "%s\\%s", src, list.entries[index].path);
            sprintf(path2, "%s\\%s", dst, list.entries[index].path);

            if ((list.entries[index].attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0) {
                if (strcmp(list.entries[index].path, ".") != 0
                    && strcmp(list.entries[index].path, "..") != 0) {
                    if (!tig_file_copy_directory(path2, path1)) {
                        tig_file_list_destroy(&list);
                        return false;
                    }
                }
            } else {
                if (!copy_file_path(path2, path1)) {
                    tig_file_list_destroy(&list);
                    return false;
                }
            }
        }
    }

    tig_file_list_destroy(&list);
    return true;
}

// 0x52E430
bool tig_file_archive_native(const char* dst, const char* src)
{
    char index_path[TIG_MAX_PATH];
    char data_path[TIG_MAX_PATH];
    TigFile* index_stream;
    TigFile* data_stream;
    bool success;
    int type;

    if (!tig_file_is_directory(src)) {
        return false;
    }

    sprintf(index_path, "%s.tfai", dst);
    index_stream = tig_file_fopen_native(index_path, "wb");
    if (index_stream == NULL) {
        return false;
    }

    sprintf(data_path, "%s.tfaf", dst);
    data_stream = tig_file_fopen_native(data_path, "wb");
    if (data_stream == NULL) {
        tig_file_fclose(index_stream);
        return false;
    }

    success = tig_file_archive_worker_native(src, index_stream, data_stream);
    if (success) {
        type = 3;
        if (tig_file_fwrite(&type, sizeof(type), 1, index_stream) != 1) {
            success = false;
        }
    }

    tig_file_fclose(data_stream);
    tig_file_fclose(index_stream);

    if (!success) {
        tig_file_remove_native(data_path);
        tig_file_remove_native(index_path);
    }

    return success;
}

// 0x52E550
bool tig_file_unarchive_native(const char* src, const char* dst)
{
    char path1[TIG_MAX_PATH];
    char path2[TIG_MAX_PATH];
    char path3[TIG_MAX_PATH];
    TigFile* index_stream;
    TigFile* data_stream;
    int type;
    int size;
    char* pch;
    TigFile* tmp_stream;

    tig_file_mkdir(dst);

    sprintf(path1, "%s.tfai", src);
    index_stream = tig_file_fopen_native(path1, "rb");
    if (index_stream == NULL) {
        return false;
    }

    sprintf(path1, "%s.tfaf", src);
    data_stream = tig_file_fopen_native(path1, "rb");
    if (data_stream == NULL) {
        // FIXME: Leaks `stream1`.
        return false;
    }

    strcpy(path2, dst);

    while (tig_file_fread(&type, sizeof(type), 1, index_stream) == 1) {
        if (type == 0) {
            if (tig_file_fread(&size, sizeof(size), 1, index_stream) != 1) {
                break;
            }

            if (tig_file_fread(path1, size, 1, index_stream) != 1) {
                break;
            }

            path1[size] = '\0';

            if (tig_file_fread(&size, sizeof(size), 1, index_stream) != 1) {
                break;
            }

            compat_join_path(path3, sizeof(path3), path2, path1);
            tmp_stream = tig_file_fopen_native(path3, "wb");
            if (tmp_stream == NULL) {
                break;
            }

            if (!copy_file_stream_size(tmp_stream, data_stream, size)) {
                tig_file_fclose(tmp_stream);
                break;
            }

            tig_file_fclose(tmp_stream);
        } else if (type == 1) {
            if (tig_file_fread(&size, sizeof(size), 1, index_stream) != 1) {
                break;
            }

            if (tig_file_fread(path1, size, 1, index_stream) != 1) {
                break;
            }

            path1[size] = '\0';
            compat_append_path(path2, sizeof(path2), path1);
            tig_file_mkdir_native(path2);
        } else if (type == 2) {
            pch = strrchr(path2, PATH_SEPARATOR);
            if (pch == NULL) {
                break;
            }
            *pch = '\0';

            pch = strrchr(path2, PATH_SEPARATOR);
            if (pch == NULL) {
                break;
            }
            pch[1] = '\0';
        } else if (type == 3) {
            tig_file_fclose(data_stream);
            tig_file_fclose(index_stream);
            return true;
        }
    }

    tig_file_fclose(data_stream);
    tig_file_fclose(index_stream);
    return false;
}

// 0x52E840
bool copy_file_path(const char* dst, const char* src)
{
    TigFile* dst_stream;
    TigFile* src_stream;

    src_stream = tig_file_fopen(src, "rb");
    if (src_stream == NULL) {
        return false;
    }

    dst_stream = tig_file_fopen(dst, "wb");
    if (dst_stream == NULL) {
        tig_file_fclose(src_stream);
        return false;
    }

    if (!copy_file_stream(dst_stream, src_stream)) {
        tig_file_fclose(dst_stream);
        tig_file_fclose(src_stream);
        tig_file_remove(dst);
        return false;
    }

    tig_file_fclose(dst_stream);
    tig_file_fclose(src_stream);
    return true;
}

// 0x52E8D0
bool copy_file_stream(TigFile* dst_stream, TigFile* src_stream)
{
    size_t size = tig_file_filelength(src_stream);
    if (size != 0) {
        return copy_file_stream_size(dst_stream, src_stream, size);
    } else {
        return true;
    }
}

// 0x52E900
bool copy_file_stream_size(TigFile* dst_stream, TigFile* src_stream, size_t size)
{
    unsigned char buffer[COPY_BUFFER_SIZE];

    if (size == 0) {
        return false;
    }

    while (size > COPY_BUFFER_SIZE) {
        if (tig_file_fread(buffer, COPY_BUFFER_SIZE, 1, src_stream) != 1) {
            return false;
        }

        if (tig_file_fwrite(buffer, COPY_BUFFER_SIZE, 1, dst_stream) != 1) {
            return false;
        }

        size -= COPY_BUFFER_SIZE;
    }

    if (tig_file_fread(buffer, size, 1, src_stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(buffer, size, 1, dst_stream) != 1) {
        return false;
    }

    return true;
}

// 0x52E9C0
bool tig_file_archive_worker_native(const char* path, TigFile* index_stream, TigFile* data_stream)
{
    char pattern[TIG_MAX_PATH];
    TigFileList list;
    unsigned int index;
    int type;
    int size;
    TigFile* tmp_stream;

    compat_join_path(pattern, sizeof(pattern), path, "*.*");
    tig_file_list_create_native(&list, pattern);

    for (index = 0; index < list.count; index++) {
        if ((list.entries[index].attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0) {
            if (strcmp(list.entries[index].path, ".") != 0
                && strcmp(list.entries[index].path, "..") != 0) {
                type = 1;
                if (tig_file_fwrite(&type, sizeof(type), 1, index_stream) != 1) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                size = (int)strlen(list.entries[index].path);
                if (tig_file_fwrite(&size, sizeof(size), 1, index_stream) != 1) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                if (tig_file_fputs(list.entries[index].path, index_stream) < 0) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                compat_join_path(pattern, sizeof(pattern), path, list.entries[index].path);

                if (!tig_file_archive_worker_native(pattern, index_stream, data_stream)) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                type = 2;
                if (tig_file_fwrite(&type, sizeof(type), 1, index_stream) != 1) {
                    // FIXME: Leaks `list`.
                    return false;
                }
            }
        } else {
            type = 0;
            if (tig_file_fwrite(&type, sizeof(type), 1, index_stream) != 1) {
                // FIXME: Leaks `list`.
                return false;
            }

            size = (int)strlen(list.entries[index].path);
            if (tig_file_fwrite(&size, sizeof(size), 1, index_stream) != 1) {
                // FIXME: Leaks `list`.
                return false;
            }

            if (tig_file_fputs(list.entries[index].path, index_stream) < 0) {
                // FIXME: Leaks `list`.
                return false;
            }

            size = (int)list.entries[index].size;
            if (tig_file_fwrite(&size, sizeof(size), 1, index_stream) != 1) {
                // FIXME: Leaks `list`.
                return false;
            }

            compat_join_path(pattern, sizeof(pattern), path, list.entries[index].path);

            tmp_stream = tig_file_fopen(pattern, "rb");
            if (tmp_stream == NULL) {
                // FIXME: Leaks `list`.
                return false;
            }

            if (!copy_file_stream(data_stream, tmp_stream)) {
                // FIXME: Leaks `tmp_stream`.
                // FIXME: Leaks `list`.
                return false;
            }

            tig_file_fclose(tmp_stream);
        }
    }

    tig_file_list_destroy(&list);
    return true;
}

// 0x52ECA0
int tig_file_init(TigInitInfo* init_info)
{
    (void)init_info;

    // FIX: Make `tig.dat` mandatory.
    //
    // When `tig.dat` is not present in the current working directory, the
    // original code attempts to add a repository at the path provided by
    // `GetModuleFileNameA`, which is the full path to `Arcanum.exe`. Obviously,
    // this is not a valid asset bundle.
    if (!tig_file_repository_add("tig.dat")) {
        return TIG_ERR_GENERIC;
    }

    // Current working directory.
    tig_file_repository_add(".");

    return TIG_OK;
}

// 0x52ED00
void tig_file_exit()
{
    TigFileIgnore* next;

    while (tig_file_ignore_head != NULL) {
        next = tig_file_ignore_head->next;
        if (tig_file_ignore_head->path != NULL) {
            FREE(tig_file_ignore_head->path);
        }
        FREE(tig_file_ignore_head);
        tig_file_ignore_head = next;
    }

    tig_file_repository_remove_all();
}

// 0x52ED40
bool tig_file_repository_add_native(const char* path)
{
    TigFileRepository* curr;
    TigFileRepository* prev;
    TigFileRepository* next;
    TigFileRepository* repo;
    SDL_PathInfo path_info;
    TigDatabase* database;
    char cache_path[TIG_MAX_PATH];

    prev = NULL;
    curr = tig_file_repositories_head;
    while (curr != NULL && SDL_strcasecmp(curr->path, path) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL) {
        if (prev != NULL) {
            if ((curr->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0
                && curr->next != NULL
                && SDL_strcasecmp(curr->next->path, path) == 0) {
                // Move repo on top of the list.
                next = curr->next;
                curr->next = next->next;
                next->next = tig_file_repositories_head;
                tig_file_repositories_head = next;
            }

            prev->next = curr->next;
            curr->next = tig_file_repositories_head;
            tig_file_repositories_head = curr;
        }
        return true;
    }

    // NOTE: Original code is slightly different. For unknown reason (likely a
    // bug) it treats `path` as a pattern and loops over it using
    // `tig_find_first_file` and `tig_find_next_file`. However, it does not use
    // the resolved file name, only the provided `path`. This means that the
    // same repository can be added multiple times if the pattern given to this
    // function has several matches.
    if (SDL_GetPathInfo(path, &path_info)) {
        if (path_info.type == SDL_PATHTYPE_DIRECTORY) {
            repo = (TigFileRepository*)MALLOC(sizeof(TigFileRepository));
            repo->type = 0;
            repo->path = NULL;
            repo->database = NULL;
            repo->next = NULL;

            repo->path = STRDUP(path);
            repo->type = TIG_FILE_REPOSITORY_DIRECTORY;
            repo->next = tig_file_repositories_head;
            tig_file_repositories_head = repo;

            tig_debug_printf("TIG File: Added path \"%s\"\n", path);

            compat_join_path(cache_path, sizeof(cache_path), tig_file_repositories_head->path, CACHE_DIR_NAME);
            tig_file_rmdir_recursively_native(cache_path);

            return true;
        } else {
            database = tig_database_open(path);
            if (database != NULL) {
                repo = (TigFileRepository*)MALLOC(sizeof(TigFileRepository));
                repo->type = 0;
                repo->path = NULL;
                repo->database = NULL;
                repo->next = NULL;

                repo->path = STRDUP(path);
                repo->type = TIG_FILE_REPOSITORY_DATABASE;
                repo->database = database;

                repo->next = tig_file_repositories_head;
                tig_file_repositories_head = repo;

                tig_debug_printf("TIG File: Added database \"%s\"\n", path);

                return true;
            }
        }
    }

    tig_debug_printf("TIG File: Error - Invalid path \"%s\"\n", path);
    return false;
}

// 0x52EF30
bool tig_file_repository_remove_native(const char* file_name)
{
    TigFileRepository* repo;
    TigFileRepository* prev;
    TigFileRepository* next;
    bool removed = false;
    char path[TIG_MAX_PATH];

    prev = NULL;
    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if (SDL_strcasecmp(file_name, repo->path) == 0) {
            next = repo->next;
            if ((repo->type & TIG_FILE_DATABASE) != 0) {
                tig_database_close(repo->database);
            } else {
                compat_join_path(path, sizeof(path), repo->path, CACHE_DIR_NAME);
                tig_file_rmdir_recursively_native(path);
            }

            if (prev != NULL) {
                prev->next = next;
            } else {
                tig_file_repositories_head = next;
            }

            FREE(repo->path);
            FREE(repo);

            repo = next;
            removed = true;
        } else {
            prev = repo;
            repo = repo->next;
        }
    }

    return removed;
}

// 0x52F000
bool tig_file_repository_remove_all()
{
    TigFileRepository* curr;
    TigFileRepository* next;
    char path[TIG_MAX_PATH];

    curr = tig_file_repositories_head;
    while (curr != NULL) {
        next = curr->next;
        if ((curr->type & 1) != 0) {
            tig_database_close(curr->database);
        } else {
            compat_join_path(path, sizeof(path), curr->path, CACHE_DIR_NAME);
            tig_file_rmdir_recursively_native(path);
        }

        FREE(curr->path);
        FREE(curr);

        curr = next;
    }

    tig_file_repositories_head = NULL;

    return true;
}

// 0x52F0A0
bool tig_file_repository_guid(const char* path, TigGuid* guid)
{
    TigFileRepository* curr = tig_file_repositories_head;
    while (curr != NULL) {
        if ((curr->type & TIG_FILE_DATABASE) != 0) {
            if (SDL_strcasecmp(curr->database->path, path) == 0) {
                *guid = curr->database->guid;
                return true;
            }
        }
        curr = curr->next;
    }

    return false;
}

// 0x52F100
int tig_file_mkdir_ex_native(const char* path)
{
    char temp_path[TIG_MAX_PATH];
    size_t temp_path_length;
    TigFileRepository* repo;

    temp_path[0] = '\0';

    if (path[0] != '.' && path[0] != '\\' && path[1] != ':' && path[0] != '/') {
        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_PLAIN) != 0) {
                strcpy(temp_path, repo->path);
                break;
            }
            repo = repo->next;
        }
    }

    compat_append_path(temp_path, sizeof(temp_path), path);

    temp_path_length = strlen(temp_path);
    if (temp_path[temp_path_length] == PATH_SEPARATOR) {
        temp_path[temp_path_length] = '\0';
    }

    if (!SDL_CreateDirectory(temp_path)) {
        return -1;
    }

    return 0;
}

// 0x52F230
int tig_file_rmdir_ex_native(const char* path)
{
    char temp_path[TIG_MAX_PATH];
    TigFileRepository* repo;

    temp_path[0] = '\0';

    if (path[0] != '.' && path[0] != '\\' && path[1] != ':' && path[0] != '/') {
        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                strcpy(temp_path, repo->path);
                break;
            }
            repo = repo->next;
        }
    }

    compat_append_path(temp_path, sizeof(temp_path), path);

    if (!SDL_RemovePath(temp_path)) {
        return -1;
    }

    return 0;
}

// 0x52F370
void tig_file_ignore(const char* path, unsigned int flags)
{
    TigFileIgnore* ignore;
    size_t path_length;
    char* pch;

    if ((flags & (TIG_FILE_DATABASE | TIG_FILE_PLAIN)) != 0) {
        if (fpattern_isvalid(path)) {
            ignore = (TigFileIgnore*)MALLOC(sizeof(*ignore));
            ignore->flags = flags;
            ignore->path = strdup(path);
            ignore->next = tig_file_ignore_head;
            tig_file_ignore_head = ignore;

            // Trim *.* pattern from the end of string.
            path_length = strlen(ignore->path);
            if (path_length > 3) {
                pch = &(ignore->path[path_length - 3]);
                if (strcmp(pch, "*.*") == 0) {
                    *pch = '\0';
                }
            }
        }
    }
}

// 0x52F410
bool tig_file_extract_native(const char* filename, char* path)
{
    TigDatabaseEntry* database_entry;
    TigFile* in;
    TigFile* out;
    TigFileRepository* first_directory_repo;
    TigFileRepository* repo;
    TigFindFileData ffd;
    char tmp[TIG_MAX_PATH];
    unsigned int ignored;

    first_directory_repo = NULL;
    if (filename[0] == '.' || filename[0] == '\\' || filename[1] == ':' || filename[0] == '/') {
        strcpy(path, filename);
        return true;
    }

    compat_join_path(path, TIG_MAX_PATH, CACHE_DIR_NAME, filename);

    ignored = tig_file_ignored(filename);

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
            if (first_directory_repo == NULL) {
                first_directory_repo = repo;
            }

            if ((ignored & TIG_FILE_IGNORE_DIRECTORY) == 0) {
                // Check if file exists in a directory bundle.
                compat_join_path(tmp, sizeof(tmp), repo->path, filename);
                compat_resolve_path(tmp);
                if (tig_find_first_file(tmp, &ffd)) {
                    strcpy(path, tmp);
                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);

                // Check if file is already expanded.
                compat_join_path(tmp, sizeof(tmp), repo->path, path);
                compat_resolve_path(tmp);
                if (tig_find_first_file(tmp, &ffd)) {
                    strcpy(path, tmp);
                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);
            }
        } else if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0) {
                if (tig_database_get_entry(repo->database, filename, &database_entry)) {
                    compat_splitpath(path, NULL, tmp, NULL, NULL);
                    tig_file_mkdir_ex_native(tmp);

                    out = tig_file_fopen_native(path, "wb");
                    if (out == NULL) {
                        return false;
                    }

                    in = tig_file_create();
                    in->impl.database_file_stream = tig_database_fopen_entry(repo->database, database_entry, "rb");
                    in->flags |= TIG_FILE_DATABASE;

                    if (!tig_file_copy_internal(out, in)) {
                        tig_file_fclose(out);
                        tig_file_fclose(in);
                        return false;
                    }

                    tig_file_fclose(out);
                    tig_file_fclose(in);

                    if (first_directory_repo == NULL) {
                        // If we're here it means out file was sucessfully
                        // created in the first directory repo, we just need it
                        // to look deeper.
                        while (repo != NULL) {
                            if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                                first_directory_repo = repo;
                                break;
                            }
                            repo = repo->next;
                        }
                    }

                    compat_join_path(tmp, sizeof(tmp), first_directory_repo->path, path);
                    strcpy(path, tmp);

                    return true;
                }
            }
        }
        repo = repo->next;
    }

    return false;
}

// 0x52F760
void tig_file_list_create_native(TigFileList* list, const char* pattern)
{
    char mutable_pattern[TIG_MAX_PATH];
    size_t pattern_length;
    TigFileInfo info;
    TigFindFileData directory_ffd;
    TigFileRepository* repo;
    unsigned int ignored;
    TigDatabaseFindFileData database_ffd;
    char path[TIG_MAX_PATH];
    char fname[COMPAT_MAX_FNAME];
    char ext[COMPAT_MAX_EXT];

    strcpy(mutable_pattern, pattern);

    pattern_length = strlen(mutable_pattern);
    if (pattern_length > 2 && strcmp(&(mutable_pattern[pattern_length - 3]), "*.*") == 0) {
        mutable_pattern[pattern_length - 2] = '\0';
    }

    list->count = 0;
    list->entries = NULL;

    if (mutable_pattern[0] == '.' || mutable_pattern[0] == '\\' || mutable_pattern[1] == ':' || mutable_pattern[0] == '/') {
        if (tig_find_first_file(mutable_pattern, &directory_ffd)) {
            do {
                tig_file_process_attribs(directory_ffd.path_info.type, &(info.attributes));
                info.size = (size_t)directory_ffd.path_info.size;
                strcpy(info.path, directory_ffd.name);
                info.modify_time = SDL_NS_TO_SECONDS(directory_ffd.path_info.modify_time);

                tig_file_list_add(list, &info);
            } while (tig_find_next_file(&directory_ffd));
        }
        tig_find_close(&directory_ffd);
    } else {
        ignored = tig_file_ignored(pattern);

        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
                if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0) {
                    if (tig_database_find_first_entry(repo->database, mutable_pattern, &database_ffd)) {
                        do {
                            info.attributes = TIG_FILE_ATTRIBUTE_0x80 | TIG_FILE_ATTRIBUTE_READONLY;
                            if ((database_ffd.is_directory & 0x1) != 0) {
                                info.attributes |= TIG_FILE_ATTRIBUTE_SUBDIR;
                            }
                            info.size = database_ffd.size;

                            compat_splitpath(database_ffd.name, NULL, NULL, fname, ext);
                            compat_makepath(info.path, NULL, NULL, fname, ext);

                            tig_file_list_add(list, &info);
                        } while (tig_database_find_next_entry(&database_ffd));
                    }
                    tig_database_find_close(&database_ffd);
                }
            } else if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                if ((ignored & TIG_FILE_IGNORE_DIRECTORY) == 0) {
                    compat_join_path(path, sizeof(path), repo->path, mutable_pattern);

                    if (tig_find_first_file(path, &directory_ffd)) {
                        do {
                            tig_file_process_attribs(directory_ffd.path_info.type, &(info.attributes));
                            info.size = (size_t)directory_ffd.path_info.size;
                            strcpy(info.path, directory_ffd.name);
                            info.modify_time = SDL_NS_TO_SECONDS(directory_ffd.path_info.modify_time);

                            tig_file_list_add(list, &info);
                        } while (tig_find_next_file(&directory_ffd));
                    }
                    tig_find_close(&directory_ffd);
                }
            }
            repo = repo->next;
        }
    }
}

// 0x52FB20
void tig_file_list_destroy(TigFileList* file_list)
{
    if (file_list->entries != NULL) {
        FREE(file_list->entries);
    }

    file_list->count = 0;
    file_list->entries = NULL;
}

// 0x52FB40
int tig_file_filelength(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_filelength(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        long pos;
        long size;

        pos = ftell(stream->impl.plain_file_stream);
        if (pos == -1) {
            return -1;
        }

        if (fseek(stream->impl.plain_file_stream, 0, SEEK_END) != 0) {
            return -1;
        }

        size = ftell(stream->impl.plain_file_stream);
        if (size == -1) {
            return -1;
        }

        if (fseek(stream->impl.plain_file_stream, pos, SEEK_SET) != 0) {
            return -1;
        }

        return (int)size;
    }

    return -1;
}

// 0x52FB80
bool tig_file_exists_native(const char* file_name, TigFileInfo* info)
{
    TigFindFileData ffd;
    TigFileRepository* repo;
    TigDatabaseEntry* database_entry;
    unsigned int ignored;
    char path[TIG_MAX_PATH];
    char fname[COMPAT_MAX_FNAME];
    char ext[COMPAT_MAX_EXT];

    ignored = tig_file_ignored(file_name);

    if (file_name[0] == '.' || file_name[0] == '\\' || file_name[1] == ':' || file_name[0] == '/') {
        if (!tig_find_first_file(file_name, &ffd)) {
            tig_find_close(&ffd);
            return false;
        }

        if (info != NULL) {
            tig_file_process_attribs(ffd.path_info.type, &(info->attributes));
            info->size = (size_t)ffd.path_info.size;
            strcpy(info->path, ffd.name);
            info->modify_time = SDL_NS_TO_SECONDS(ffd.path_info.modify_time);
        }

        tig_find_close(&ffd);
        return true;
    }

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DIRECTORY) == 0) {
                compat_join_path(path, sizeof(path), repo->path, file_name);

                if (tig_find_first_file(path, &ffd)) {
                    if (info != NULL) {
                        tig_file_process_attribs(ffd.path_info.type, &(info->attributes));
                        info->size = (size_t)ffd.path_info.size;
                        strcpy(info->path, ffd.name);
                        info->modify_time = SDL_NS_TO_SECONDS(ffd.path_info.modify_time);
                    }

                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);
            }
        } else if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0) {
                if (tig_database_get_entry(repo->database, file_name, &database_entry)) {
                    if (info != NULL) {
                        info->attributes = TIG_FILE_ATTRIBUTE_0x80 | TIG_FILE_ATTRIBUTE_READONLY;
                        if ((database_entry->flags & TIG_DATABASE_ENTRY_DIRECTORY) != 0) {
                            info->attributes |= TIG_FILE_ATTRIBUTE_SUBDIR;
                        }
                        info->size = database_entry->size;

                        compat_splitpath(database_entry->path, NULL, NULL, fname, ext);
                        compat_makepath(info->path, 0, 0, fname, ext);
                    }

                    return true;
                }
            }
        }
        repo = repo->next;
    }

    return false;
}

// 0x52FE60
bool tig_file_exists_in_path_native(const char* search_path, const char* file_name, TigFileInfo* info)
{
    TigFindFileData ffd;
    TigFileRepository* repo;
    TigDatabaseEntry* database_entry;
    unsigned int ignored;
    char path[TIG_MAX_PATH];
    char fname[COMPAT_MAX_FNAME];
    char ext[COMPAT_MAX_EXT];

    if (file_name[0] == '.' || file_name[0] == '\\' || file_name[1] == ':' || file_name[0] == '/') {
        return tig_file_exists_native(file_name, info);
    }

    ignored = tig_file_ignored(file_name);

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) != 0
                && SDL_strcasecmp(search_path, repo->path) == 0) {
                compat_join_path(path, sizeof(path), repo->path, file_name);

                if (tig_find_first_file(path, &ffd)) {
                    if (info != NULL) {
                        tig_file_process_attribs(ffd.path_info.type, &(info->attributes));
                        info->size = (size_t)ffd.path_info.size;
                        strcpy(info->path, ffd.name);
                        info->modify_time = SDL_NS_TO_SECONDS(ffd.path_info.modify_time);
                    }

                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);
            }
        } else if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0
                && SDL_strcasecmp(search_path, repo->path) == 0) {
                if (tig_database_get_entry(repo->database, file_name, &database_entry)) {
                    if (info != NULL) {
                        info->attributes = TIG_FILE_ATTRIBUTE_0x80 | TIG_FILE_ATTRIBUTE_READONLY;
                        if ((database_entry->flags & TIG_DATABASE_ENTRY_DIRECTORY) != 0) {
                            info->attributes |= TIG_FILE_ATTRIBUTE_SUBDIR;
                        }
                        info->size = database_entry->size;

                        compat_splitpath(database_entry->path, NULL, NULL, fname, ext);
                        compat_makepath(info->path, 0, 0, fname, ext);
                    }

                    return true;
                }
            }
        }
        repo = repo->next;
    }

    return false;
}

// 0x5300F0
int tig_file_remove_native(const char* file_name)
{
    TigFileRepository* repo;
    char path[TIG_MAX_PATH];
    TigDatabaseEntry* database_entry;

    if (file_name[0] == '.' || file_name[0] == '\\' || file_name[1] == ':' || file_name[0] == '/') {
        return SDL_RemovePath(file_name) ? 0 : 1;
    }

    if ((tig_file_ignored(file_name) & 0x2) != 0) {
        return 1;
    }

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_PLAIN) != 0) {
            compat_join_path(path, sizeof(path), repo->path, file_name);

            if (SDL_RemovePath(path)) {
                repo = repo->next;
                while (repo != NULL) {
                    if ((repo->type & TIG_FILE_DATABASE) != 0
                        && tig_database_get_entry(repo->database, file_name, &database_entry)) {
                        database_entry->flags &= ~0x200;
                        database_entry->flags |= 0x100;
                        break;
                    }
                    repo = repo->next;
                }

                return 0;
            }
        }
        repo = repo->next;
    }

    return 1;
}

// 0x5301F0
int tig_file_rename_native(const char* old_file_name, const char* new_file_name)
{
    TigFileRepository* repo;
    char old_path[TIG_MAX_PATH];
    char new_path[TIG_MAX_PATH];

    if (old_file_name[0] == '.' || old_file_name[0] == '\\' || old_file_name[1] == ':' || old_file_name[0] == '/') {
        return SDL_RenamePath(old_file_name, new_file_name) ? 0 : 1;
    }

    if ((tig_file_ignored(old_file_name) & 0x2) != 0) {
        return 1;
    }

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_PLAIN) != 0) {
            compat_join_path(old_path, sizeof(old_path), repo->path, old_file_name);
            compat_join_path(new_path, sizeof(new_path), repo->path, new_file_name);

            if (SDL_RenamePath(old_path, new_path)) {
                return 0;
            }
        }
        repo = repo->next;
    }

    return 1;
}

// 0x530330
int tig_file_fclose(TigFile* stream)
{
    bool success;

    success = tig_file_close_internal(stream);
    tig_file_destroy(stream);

    if (!success) {
        return EOF;
    }

    return 0;
}

// 0x530360
int tig_file_fflush(TigFile* stream)
{
    if (stream == NULL) {
        return fflush(stdin);
    }

    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fflush(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fflush(stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x5303A0
TigFile* tig_file_fopen_native(const char* path, const char* mode)
{
    TigFile* stream;

    stream = tig_file_create();
    if (tig_file_open_internal_native(path, mode, stream) == 0) {
        tig_file_destroy(stream);
        return NULL;
    }

    return stream;
}

// 0x5303D0
TigFile* tig_file_reopen_native(const char* path, const char* mode, TigFile* stream)
{
    tig_file_close_internal(stream);

    if (tig_file_open_internal_native(path, mode, stream) == 0) {
        tig_file_destroy(stream);
        return NULL;
    }

    return stream;
}

// 0x530410
int tig_file_setbuf(TigFile* stream, char* buffer)
{
    if (buffer != NULL) {
        return tig_file_setvbuf(stream, buffer, _IOFBF, 512);
    } else {
        return tig_file_setvbuf(stream, NULL, _IONBF, 512);
    }
}

// 0x530440
int tig_file_setvbuf(TigFile* stream, char* buffer, int mode, size_t size)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_setvbuf(stream->impl.database_file_stream, buffer, mode, size);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return setvbuf(stream->impl.plain_file_stream, buffer, mode, size);
    }

    return 1;
}

// 0x530490
int tig_file_fprintf(TigFile* stream, const char* format, ...)
{
    int rc;
    va_list args;

    va_start(args, format);
    rc = tig_file_vfprintf(stream, format, args);
    va_end(args);

    return rc;
}

// 0x5304B0
int sub_5304B0()
{
    return -1;
}

// 0x5304C0
int tig_file_vfprintf(TigFile* stream, const char* format, va_list args)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_vfprintf(stream->impl.database_file_stream, format, args);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return vfprintf(stream->impl.plain_file_stream, format, args);
    }

    return -1;
}

// 0x530510
int tig_file_fgetc(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fgetc(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fgetc(stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x530540
char* tig_file_fgets(char* buffer, int max_count, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fgets(buffer, max_count, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fgets(buffer, max_count, stream->impl.plain_file_stream);
    }

    return NULL;
}

// 0x530580
int tig_file_fputc(int ch, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fputc(ch, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fputc(ch, stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x5305C0
int tig_file_fputs(const char* str, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fputs(str, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fputs(str, stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x530600
int tig_file_ungetc(int ch, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_ungetc(ch, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return ungetc(ch, stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x530640
size_t tig_file_fread(void* buffer, size_t size, size_t count, TigFile* stream)
{
    if (size == 0 || count == 0) {
        return 0;
    }

    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fread(buffer, size, count, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fread(buffer, size, count, stream->impl.plain_file_stream);
    }

    return count - 1;
}

// 0x5306A0
size_t tig_file_fwrite(const void* buffer, size_t size, size_t count, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fwrite(buffer, size, count, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fwrite(buffer, size, count, stream->impl.plain_file_stream);
    }

    return count - 1;
}

// 0x5306F0
int tig_file_fgetpos(TigFile* stream, int* pos_ptr)
{
    int pos;

    pos = tig_file_ftell(stream);
    if (pos == -1) {
        return 1;
    }

    *pos_ptr = pos;

    return 0;
}

// 0x530720
int tig_file_fseek(TigFile* stream, int offset, int origin)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fseek(stream->impl.database_file_stream, offset, origin);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fseek(stream->impl.plain_file_stream, offset, origin);
    }

    return 1;
}

// 0x530770
int tig_file_fsetpos(TigFile* stream, int* pos)
{
    return tig_file_fseek(stream, *pos, SEEK_SET);
}

// 0x530790
int tig_file_ftell(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_ftell(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return ftell(stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x5307C0
void tig_file_rewind(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        tig_database_rewind(stream->impl.database_file_stream);
    } else if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        rewind(stream->impl.plain_file_stream);
    }
}

// 0x5307F0
void tig_file_clearerr(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        tig_database_clearerr(stream->impl.database_file_stream);
    } else if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        clearerr(stream->impl.plain_file_stream);
    }
}

// 0x530820
int tig_file_feof(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_feof(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return feof(stream->impl.plain_file_stream);
    }

    return 0;
}

// 0x530850
int tig_file_ferror(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_ferror(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return ferror(stream->impl.plain_file_stream);
    }

    return 0;
}

// 0x5308A0
void sub_5308A0(int a1, int a2)
{
    (void)a1;
    (void)a2;
    // tig_database_pack(a1, a2);
}

// 0x5308C0
void sub_5308C0(int a1, int a2)
{
    (void)a1;
    (void)a2;
    // tig_database_unpack(a1, a2);
}

// 0x5308E0
bool tig_file_lock(const char* filename, const void* owner, size_t size)
{
    char path[TIG_MAX_PATH];
    TigFileRepository* repo;
    FILE* stream;

    if (filename[0] == '.' || filename[0] == '\\' || filename[1] == ':') {
        strcpy(path, filename);
    } else {
        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                break;
            }
            repo = repo->next;
        }

        if (repo == NULL) {
            return false;
        }

        sprintf(path, "%s\\%s", repo->path, filename);
    }

    stream = fopen(path, "wbx");
    if (stream == NULL) {
        return false;
    }

    if (size > 1024) {
        size = 1024;
    }

    if (fwrite(owner, size, 1, stream) != 1) {
        fclose(stream);
        SDL_RemovePath(path);
        return false;
    }

    if (fclose(stream) != 0) {
        SDL_RemovePath(path);
        return false;
    }

    return true;
}

// 0x530A10
bool tig_file_unlock(const char* filename, const void* owner, size_t size)
{
    bool unlocked = true;
    char drive[COMPAT_MAX_DRIVE];
    char dir[COMPAT_MAX_DIR];
    char path[TIG_MAX_PATH];
    TigFileList file_list;
    unsigned int index;

    compat_splitpath(filename, drive, dir, NULL, NULL);

    tig_file_list_create(&file_list, filename);

    for (index = 0; index < file_list.count; index++) {
        if ((file_list.entries[index].attributes & (TIG_FILE_ATTRIBUTE_SUBDIR | TIG_FILE_ATTRIBUTE_0x80)) == 0) {
            compat_makepath(path, drive, dir, file_list.entries[index].path, NULL);
            if (!tig_file_locked_by(path, owner, size)
                || tig_file_remove(path) == -1) {
                unlocked = false;
            }
        }
    }

    tig_file_list_destroy(&file_list);

    return unlocked;
}

// 0x530AE0
bool tig_file_locked_by(const char* filename, const void* owner, size_t size)
{
    char path[TIG_MAX_PATH];
    TigFileRepository* repo;

    if (filename[0] == '.' || filename[0] == '\\' || filename[1] == ':') {
        strcpy(path, filename);
    } else {
        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                break;
            }
            repo = repo->next;
        }

        if (repo == NULL) {
            return false;
        }

        sprintf(path, "%s\\%s", repo->path, filename);
    }

    return sub_5310C0(path, owner, size);
}

// 0x530B90
bool sub_530B90(const char* pattern)
{
    TigFileList list;
    int count;

    tig_file_list_create(&list, pattern);
    count = list.count;
    tig_file_list_destroy(&list);

    return count != 0;
}

// 0x530BD0
TigFile* tig_file_create()
{
    TigFile* stream;

    stream = (TigFile*)MALLOC(sizeof(*stream));
    stream->path = 0;
    stream->flags = 0;
    stream->impl.plain_file_stream = 0;

    return stream;
}

// 0x530BF0
void tig_file_destroy(TigFile* stream)
{
    FREE(stream);
}

// 0x530C00
bool tig_file_close_internal(TigFile* stream)
{
    bool success = false;

    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        if (tig_database_fclose(stream->impl.database_file_stream) == 0) {
            success = true;
        }
    } else if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        if (fclose(stream->impl.plain_file_stream) == 0) {
            success = true;
        }
    }

    if ((stream->flags & TIG_FILE_DELETE_ON_CLOSE) != 0) {
        tig_file_remove(stream->path);
        FREE(stream->path);
        stream->path = NULL;
        stream->flags &= ~TIG_FILE_DELETE_ON_CLOSE;
    }

    return success;
}

// 0x530C70
int tig_file_open_internal_native(const char* path, const char* mode, TigFile* stream)
{
    unsigned int ignored;
    TigFileRepository* repo;
    TigFileRepository* writeable_repo;
    TigDatabaseEntry* database_entry;
    char mutable_path[TIG_MAX_PATH];

    stream->flags &= ~(TIG_FILE_DATABASE | TIG_FILE_PLAIN);

    if (path[0] == '.' || path[0] == '\\' || path[1] == ':' || path[0] == '/') {
        stream->impl.plain_file_stream = fopen(path, mode);
        if (stream->impl.plain_file_stream == NULL) {
            return 0;
        }

        stream->flags |= TIG_FILE_PLAIN;
    } else {
        ignored = tig_file_ignored(path);

        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0
                && (ignored & TIG_FILE_IGNORE_DATABASE) == 0
                && tig_database_get_entry(repo->database, path, &database_entry)) {
                if ((database_entry->flags & (TIG_DATABASE_ENTRY_0x100 | TIG_DATABASE_ENTRY_0x200)) != 0
                    || mode[0] == 'w') {
                    writeable_repo = tig_file_repositories_head;
                    while (writeable_repo != repo) {
                        if ((writeable_repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                            compat_join_path(mutable_path, sizeof(mutable_path), writeable_repo->path, path);
                            compat_resolve_path(mutable_path);

                            stream->impl.plain_file_stream = fopen(mutable_path, mode);
                            if (stream->impl.plain_file_stream != NULL) {
                                stream->flags |= TIG_FILE_PLAIN;
                                database_entry->flags &= ~TIG_DATABASE_ENTRY_0x100;
                                database_entry->flags |= TIG_DATABASE_ENTRY_0x200;
                                break;
                            }
                        }
                        writeable_repo = writeable_repo->next;
                    }
                }

                if ((stream->flags & TIG_FILE_PLAIN) == 0) {
                    database_entry->flags &= ~(TIG_DATABASE_ENTRY_0x100 | TIG_DATABASE_ENTRY_0x200);
                    stream->impl.database_file_stream = tig_database_fopen_entry(repo->database, database_entry, mode);
                    stream->flags |= TIG_FILE_DATABASE;
                }
                break;
            }
            repo = repo->next;
        }

        if ((stream->flags & (TIG_FILE_DATABASE | TIG_FILE_PLAIN)) == 0
            && (ignored & TIG_FILE_PLAIN) == 0) {
            repo = tig_file_repositories_head;
            while (repo != NULL) {
                if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                    compat_join_path(mutable_path, sizeof(mutable_path), repo->path, path);
                    compat_resolve_path(mutable_path);

                    stream->impl.plain_file_stream = fopen(mutable_path, mode);
                    if (stream->impl.plain_file_stream != NULL) {
                        stream->flags |= TIG_FILE_PLAIN;
                        break;
                    }
                }
                repo = repo->next;
            }
        }
    }

    return stream->flags & (TIG_FILE_DATABASE | TIG_FILE_PLAIN);
}

// 0x530F90
void tig_file_process_attribs(SDL_PathType type, unsigned int* flags)
{
    *flags = 0;

    if (type == SDL_PATHTYPE_DIRECTORY) {
        *flags |= TIG_FILE_ATTRIBUTE_SUBDIR;
    }
}

// 0x530FD0
void tig_file_list_add(TigFileList* list, TigFileInfo* info)
{
    if (list->count != 0) {
        int l = 0;
        int r = list->count - 1;
        int m;
        int cmp;

        while (l <= r) {
            m = (l + r) / 2;
            cmp = SDL_strcasecmp(list->entries[m].path, info->path);
            if (cmp < 0) {
                l = m + 1;
            } else if (cmp > 0) {
                r = m - 1;
            } else {
                // Already added.
                return;
            }
        }

        list->entries = (TigFileInfo*)REALLOC(list->entries, sizeof(TigFileInfo) * (list->count + 1));
        memmove(&(list->entries[l + 1]), &(list->entries[l]), sizeof(TigFileInfo) * (list->count - l));
        list->entries[l] = *info;
        list->count++;
    } else {
        list->entries = (TigFileInfo*)MALLOC(sizeof(TigFileInfo));
        list->entries[0] = *info;
        list->count++;
    }
}

// 0x5310C0
bool sub_5310C0(const char* filename, const void* owner, size_t size)
{
    FILE* stream;
    uint8_t file_owner[1024];

    stream = fopen(filename, "rb");
    if (stream == NULL) {
        return false;
    }

    if (size > 1024) {
        size = 1024;
    }

    if (fread(file_owner, size, 1, stream) != 0) {
        fclose(stream);
        return false;
    }

    if (memcmp(owner, file_owner, size) != 0) {
        fclose(stream);
        return false;
    }

    fclose(stream);

    return true;
}

// 0x531170
unsigned int tig_file_ignored(const char* path)
{
    if (!fpattern_isvalid(path)) {
        return 0;
    }

    off_62B2A8 = tig_file_ignore_head;
    while (off_62B2A8 != NULL) {
        if (fpattern_matchn(off_62B2A8->path, path)) {
            return off_62B2A8->flags;
        }
        off_62B2A8 = off_62B2A8->next;
    }

    return 0;
}

// 0x5311D0
bool tig_file_copy_native(const char* src, const char* dst)
{
    TigFile* in;
    TigFile* out;

    if (!tig_file_exists_native(src, NULL)) {
        return false;
    }

    in = tig_file_fopen_native(src, "rb");
    if (in == NULL) {
        return false;
    }

    out = tig_file_fopen_native(dst, "wb");
    if (out == NULL) {
        tig_file_fclose(in);
        return false;
    }

    if (!tig_file_copy_internal(out, in)) {
        tig_file_fclose(out);
        tig_file_fclose(in);
        return false;
    }

    tig_file_fclose(out);
    tig_file_fclose(in);
    return true;
}

// 0x531250
bool tig_file_copy_internal(TigFile* dst, TigFile* src)
{
    unsigned char buffer[1 << 15];
    unsigned int size;

    size = tig_file_filelength(src);
    if (src == 0) {
        return false;
    }

    while (size >= sizeof(buffer)) {
        if (tig_file_fread(buffer, sizeof(buffer), 1, src) != 1) {
            return false;
        }

        if (tig_file_fwrite(buffer, sizeof(buffer), 1, dst) != 1) {
            return false;
        }

        size -= sizeof(buffer);
    }

    if (tig_file_fread(buffer, size, 1, src) != 1) {
        return false;
    }

    if (tig_file_fwrite(buffer, size, 1, dst) != 1) {
        return false;
    }

    return true;
}

// 0x531320
int tig_file_rmdir_recursively_native(const char* path)
{
    char mutable_path[TIG_MAX_PATH];
    TigFindFileData ffd;

    compat_join_path(mutable_path, sizeof(mutable_path), path, "*.*");

    if (tig_find_first_file(mutable_path, &ffd)) {
        do {
            compat_join_path(mutable_path, sizeof(mutable_path), path, ffd.name);

            if (ffd.path_info.type == SDL_PATHTYPE_DIRECTORY) {
                if (strcmp(ffd.name, ".") != 0
                    && strcmp(ffd.name, "..") != 0) {
                    tig_file_rmdir_recursively_native(mutable_path);
                }
            } else {
                SDL_RemovePath(mutable_path);
            }
        } while (tig_find_next_file(&ffd));
    }
    tig_find_close(&ffd);

    if (!SDL_RemovePath(path)) {
        return -1;
    }

    return 0;
}

static Sint64 tig_file_io_size(void* userdata)
{
    TigFile* stream = (TigFile*)userdata;

    return tig_file_filelength(stream);
}

static Sint64 tig_file_io_seek(void* userdata, Sint64 offset, SDL_IOWhence whence)
{
    TigFile* stream = (TigFile*)userdata;
    int stdio_whence;

    if (offset < INT_MIN || offset > INT_MAX) {
        return -1;
    }

    switch (whence) {
    case SDL_IO_SEEK_SET:
        stdio_whence = SEEK_SET;
        break;
    case SDL_IO_SEEK_CUR:
        stdio_whence = SEEK_CUR;
        break;
    case SDL_IO_SEEK_END:
        stdio_whence = SEEK_END;
        break;
    default:
        SDL_SetError("Unknown value for 'whence'");
        return -1;
    }

    if (tig_file_fseek(stream, (int)offset, stdio_whence) != 0) {
        return -1;
    }

    return tig_file_ftell(stream);
}

static size_t tig_file_io_read(void* userdata, void* ptr, size_t size, SDL_IOStatus* status)
{
    TigFile* stream = (TigFile*)userdata;
    size_t bytes = tig_file_fread(ptr, 1, size, stream);

    (void)status;

    return bytes;
}

static size_t tig_file_io_write(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status)
{
    TigFile* stream = (TigFile*)userdata;
    size_t bytes = tig_file_fwrite(ptr, 1, size, stream);

    (void)status;

    return bytes;
}

static bool tig_file_io_close(void* userdata)
{
    TigFile* stream = (TigFile*)userdata;

    if (tig_file_fclose(stream) != 0) {
        return false;
    }

    return true;
}

SDL_IOStream* tig_file_io_open(const char* path, const char* mode)
{
    TigFile* stream;
    SDL_IOStreamInterface iface;
    SDL_IOStream* io;

    stream = tig_file_fopen(path, mode);
    if (stream == NULL) {
        return NULL;
    }

    SDL_INIT_INTERFACE(&iface);
    iface.size = tig_file_io_size;
    iface.seek = tig_file_io_seek;
    iface.read = tig_file_io_read;
    iface.write = tig_file_io_write;
    iface.close = tig_file_io_close;

    io = SDL_OpenIO(&iface, stream);
    if (io == NULL) {
        return NULL;
    }

    return io;
}

bool tig_file_mkdir(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_mkdir_native(native_path);
}

bool tig_file_rmdir(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_rmdir_native(native_path);
}

bool tig_file_empty_directory(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_empty_directory_native(native_path);
}

bool tig_file_is_empty_directory(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_is_empty_directory_native(native_path);
}

bool tig_file_is_directory(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_is_directory_native(native_path);
}

bool tig_file_archive(const char* dst, const char* src)
{
    char native_dst[TIG_MAX_PATH];
    char native_src[TIG_MAX_PATH];

    strcpy(native_dst, dst);
    compat_windows_path_to_native(native_dst);
    compat_resolve_path(native_dst);

    strcpy(native_src, src);
    compat_windows_path_to_native(native_src);
    compat_resolve_path(native_src);

    return tig_file_archive_native(native_dst, native_src);
}

bool tig_file_unarchive(const char* src, const char* dst)
{
    char native_src[TIG_MAX_PATH];
    char native_dst[TIG_MAX_PATH];

    strcpy(native_src, src);
    compat_windows_path_to_native(native_src);
    compat_resolve_path(native_src);

    strcpy(native_dst, dst);
    compat_windows_path_to_native(native_dst);
    compat_resolve_path(native_dst);

    return tig_file_unarchive_native(native_src, native_dst);
}

bool tig_file_repository_add(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_repository_add_native(native_path);
}

bool tig_file_repository_remove(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_repository_remove_native(native_path);
}

int tig_file_mkdir_ex(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_mkdir_ex_native(native_path);
}

int tig_file_rmdir_ex(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_rmdir_ex_native(native_path);
}

bool tig_file_extract(const char* filename, char* path)
{
    char native_filename[TIG_MAX_PATH];

    strcpy(native_filename, filename);
    compat_windows_path_to_native(native_filename);
    compat_resolve_path(native_filename);

    return tig_file_extract_native(native_filename, path);
}

void tig_file_list_create(TigFileList* list, const char* pattern)
{
    char native_pattern[TIG_MAX_PATH];

    strcpy(native_pattern, pattern);
    compat_windows_path_to_native(native_pattern);
    compat_resolve_path(native_pattern);

    tig_file_list_create_native(list, native_pattern);
}

bool tig_file_exists(const char* path, TigFileInfo* info)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_exists_native(native_path, info);
}

bool tig_file_exists_in_path(const char* search_path, const char* file_name, TigFileInfo* info)
{
    char native_search_path[TIG_MAX_PATH];
    char native_file_name[TIG_MAX_PATH];

    strcpy(native_search_path, search_path);
    compat_windows_path_to_native(native_search_path);
    compat_resolve_path(native_search_path);

    strcpy(native_file_name, file_name);
    compat_windows_path_to_native(native_file_name);
    compat_resolve_path(native_file_name);

    return tig_file_exists_in_path_native(native_search_path, native_file_name, info);
}

int tig_file_remove(const char* path)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_remove_native(native_path);
}

int tig_file_rename(const char* old_file_name, const char* new_file_name)
{
    char native_old_file_name[TIG_MAX_PATH];
    char native_new_file_name[TIG_MAX_PATH];

    strcpy(native_old_file_name, old_file_name);
    compat_windows_path_to_native(native_old_file_name);
    compat_resolve_path(native_old_file_name);

    strcpy(native_new_file_name, new_file_name);
    compat_windows_path_to_native(native_new_file_name);
    compat_resolve_path(native_new_file_name);

    return tig_file_rename_native(native_old_file_name, native_new_file_name);
}

TigFile* tig_file_fopen(const char* path, const char* mode)
{
    char native_path[TIG_MAX_PATH];

    if (path[0] == '\0') {
        return NULL;
    }

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_fopen_native(native_path, mode);
}

TigFile* tig_file_reopen(const char* path, const char* mode, TigFile* stream)
{
    char native_path[TIG_MAX_PATH];

    strcpy(native_path, path);
    compat_windows_path_to_native(native_path);
    compat_resolve_path(native_path);

    return tig_file_reopen_native(native_path, mode, stream);
}

bool tig_file_copy(const char* src, const char* dst)
{
    char native_src[TIG_MAX_PATH];
    char native_dst[TIG_MAX_PATH];

    strcpy(native_src, src);
    compat_windows_path_to_native(native_src);
    compat_resolve_path(native_src);

    strcpy(native_dst, dst);
    compat_windows_path_to_native(native_dst);
    compat_resolve_path(native_dst);

    return tig_file_copy_native(native_src, native_dst);
}
