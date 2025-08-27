#ifndef TIG_COMPAT_H_
#define TIG_COMPAT_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COMPAT_MAX_DRIVE 3
#define COMPAT_MAX_DIR 256
#define COMPAT_MAX_FNAME 256
#define COMPAT_MAX_EXT 256

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

void compat_windows_path_to_native(char* path);
void compat_resolve_path(char* path);
void compat_append_path(char* path, size_t size, const char* comp);
void compat_join_path_ex(char* path, size_t size, ...);
#define compat_join_path(path, size, ...) compat_join_path_ex(path, size, __VA_ARGS__, NULL)
void compat_splitpath(const char* path, char* drive, char* dir, char* fname, char* ext);
void compat_makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext);

#ifdef __cplusplus
}
#endif

#endif /* TIG_COMPAT_H_ */
