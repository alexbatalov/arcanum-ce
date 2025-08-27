#include "tig/compat.h"

#ifdef _WIN32
#include <stdlib.h>
#else
#include <dirent.h>
#endif

void compat_windows_path_to_native(char* path)
{
#ifdef _WIN32
    (void)path;
#else
    char* pch = path;
    while (*pch != '\0') {
        if (*pch == '\\') {
            *pch = '/';
        }
        pch++;
    }
#endif
}

void compat_resolve_path(char* path)
{
#ifdef _WIN32
    (void)path;
#else
    char* pch = path;

    DIR* dir;
    if (pch[0] == '/') {
        dir = opendir("/");
        pch++;
    } else {
        dir = opendir(".");
    }

    while (dir != NULL) {
        char* sep = strchr(pch, '/');
        size_t length;
        if (sep != NULL) {
            length = sep - pch;
        } else {
            length = strlen(pch);
        }

        bool found = false;

        struct dirent* entry = readdir(dir);
        while (entry != NULL) {
            if (strlen(entry->d_name) == length && SDL_strncasecmp(pch, entry->d_name, length) == 0) {
                strncpy(pch, entry->d_name, length);
                found = true;
                break;
            }
            entry = readdir(dir);
        }

        closedir(dir);
        dir = NULL;

        if (!found) {
            break;
        }

        if (sep == NULL) {
            break;
        }

        *sep = '\0';
        dir = opendir(path);
        *sep = '/';

        pch = sep + 1;
    }
#endif
}

void compat_append_path(char* path, size_t size, const char* comp)
{
    size_t path_len = strlen(path);
    size_t comp_len = strlen(comp);

    if (path_len + comp_len + 1 >= size) {
        return;
    }

    if (path_len != 0) {
        path[path_len++] = PATH_SEPARATOR;
        path[path_len] = '\0';
    }

    strncat(path, comp, comp_len);
}

void compat_join_path_ex(char* path, size_t size, ...)
{
    va_list args;
    const char* comp;
    size_t comp_len;
    size_t path_len = 0;

    path[0] = '\0';

    va_start(args, size);

    while ((comp = va_arg(args, const char*)) != NULL) {
        comp_len = strlen(comp);
        if (path_len + comp_len + 1 >= size) {
            break;
        }

        if (path_len != 0) {
            path[path_len++] = PATH_SEPARATOR;
            path[path_len] = '\0';
        }

        strncat(path, comp, comp_len);
        path_len += comp_len;
    }

    va_end(args);
}

void compat_splitpath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
#ifdef _WIN32
    _splitpath(path, drive, dir, fname, ext);
#else
    const char* drive_start = path;
    if (path[0] == '/' && path[1] == '/') {
        path += 2;
        while (*path != '\0' && *path != '/' && *path != '.') {
            path++;
        }
    }

    if (drive != NULL) {
        size_t drive_size = path - drive_start;
        if (drive_size > COMPAT_MAX_DRIVE - 1) {
            drive_size = COMPAT_MAX_DRIVE - 1;
        }
        strncpy(drive, path, drive_size);
        drive[drive_size] = '\0';
    }

    const char* dir_start = path;
    const char* fname_start = path;
    const char* ext_start = NULL;

    const char* end = path;
    while (*end != '\0') {
        if (*end == '/') {
            fname_start = end + 1;
        } else if (*end == '.') {
            ext_start = end;
        }
        end++;
    }

    if (ext_start == NULL) {
        ext_start = end;
    }

    if (dir != NULL) {
        size_t dir_size = fname_start - dir_start;
        if (dir_size > COMPAT_MAX_DIR - 1) {
            dir_size = COMPAT_MAX_DIR - 1;
        }
        strncpy(dir, path, dir_size);
        dir[dir_size] = '\0';
    }

    if (fname != NULL) {
        size_t fname_length = ext_start - fname_start;
        if (fname_length > COMPAT_MAX_FNAME - 1) {
            fname_length = COMPAT_MAX_FNAME - 1;
        }
        strncpy(fname, fname_start, fname_length);
        fname[fname_length] = '\0';
    }

    if (ext != NULL) {
        size_t ext_length = end - ext_start;
        if (ext_length > COMPAT_MAX_EXT - 1) {
            ext_length = COMPAT_MAX_EXT - 1;
        }
        strncpy(ext, ext_start, ext_length);
        ext[ext_length] = '\0';
    }
#endif
}

void compat_makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext)
{
#ifdef _WIN32
    _makepath(path, drive, dir, fname, ext);
#else
    path[0] = '\0';

    if (drive != NULL) {
        if (*drive != '\0') {
            strcpy(path, drive);
            path = strchr(path, '\0');

            if (path[-1] == '/') {
                path--;
            } else {
                *path = '/';
            }
        }
    }

    if (dir != NULL) {
        if (*dir != '\0') {
            if (*dir != '/' && *path == '/') {
                path++;
            }

            strcpy(path, dir);
            path = strchr(path, '\0');

            if (path[-1] == '/') {
                path--;
            } else {
                *path = '/';
            }
        }
    }

    if (fname != NULL && *fname != '\0') {
        if (*fname != '/' && *path == '/') {
            path++;
        }

        strcpy(path, fname);
        path = strchr(path, '\0');
    } else {
        if (*path == '/') {
            path++;
        }
    }

    if (ext != NULL) {
        if (*ext != '\0') {
            if (*ext != '.') {
                *path++ = '.';
            }

            strcpy(path, ext);
            path = strchr(path, '\0');
        }
    }

    *path = '\0';
#endif
}
