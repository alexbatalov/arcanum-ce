#ifndef TIG_FILE_H_
#define TIG_FILE_H_

#include <stdlib.h>

#include "tig/guid.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIG_FILE_ATTRIBUTE_0x01 0x01
#define TIG_FILE_ATTRIBUTE_READONLY 0x02
#define TIG_FILE_ATTRIBUTE_HIDDEN 0x04
#define TIG_FILE_ATTRIBUTE_0x08 0x08
#define TIG_FILE_ATTRIBUTE_SYSTEM 0x10
#define TIG_FILE_ATTRIBUTE_SUBDIR 0x20
#define TIG_FILE_ATTRIBUTE_ARCHIVE 0x40
#define TIG_FILE_ATTRIBUTE_0x80 0x80

typedef struct TigFile TigFile;

typedef struct TigFileInfo {
    char path[TIG_MAX_PATH];
    unsigned int attributes;
    size_t size;
    time_t modify_time;
} TigFileInfo;

typedef struct TigFileList {
    unsigned int count;
    TigFileInfo* entries;
} TigFileList;

bool tig_file_mkdir(const char* path);
bool tig_file_rmdir(const char* path);
bool tig_file_empty_directory(const char* path);
bool tig_file_is_empty_directory(const char* path);
bool tig_file_is_directory(const char* path);
bool tig_file_copy_directory(const char* dst, const char* src);
bool tig_file_archive(const char* dst, const char* src);
bool tig_file_unarchive(const char* src, const char* dst);
int tig_file_init(TigInitInfo* init_info);
void tig_file_exit();
bool tig_file_repository_add(const char* path);
bool tig_file_repository_remove(const char* path);
bool tig_file_repository_remove_all();
bool tig_file_repository_guid(const char* path, TigGuid* guid);
int tig_file_mkdir_ex(const char* path);
int tig_file_rmdir_ex(const char* path);
bool tig_file_extract(const char* filename, char* path);
void tig_file_list_create(TigFileList* list, const char* pattern);
void tig_file_list_destroy(TigFileList* file_list);
int tig_file_filelength(TigFile* stream);
bool tig_file_exists(const char* file_name, TigFileInfo* info);
bool tig_file_exists_in_path(const char* search_path, const char* file_name, TigFileInfo* info);
int tig_file_remove(const char* file_name);
int tig_file_rename(const char* old_file_name, const char* new_file_name);
int tig_file_fclose(TigFile* stream);
int tig_file_fflush(TigFile* stream);
TigFile* tig_file_fopen(const char* path, const char* mode);
TigFile* tig_file_reopen(const char* path, const char* mode, TigFile* stream);
int tig_file_setbuf(TigFile* stream, char* buffer);
int tig_file_setvbuf(TigFile* stream, char* buffer, int mode, size_t size);
int tig_file_fprintf(TigFile* stream, const char* format, ...);
int sub_5304B0();
int tig_file_vfprintf(TigFile* stream, const char* format, va_list args);
int tig_file_fgetc(TigFile* stream);
char* tig_file_fgets(char* buffer, int max_count, TigFile* stream);
int tig_file_fputc(int ch, TigFile* stream);
int tig_file_fputs(const char* str, TigFile* stream);
int tig_file_ungetc(int ch, TigFile* stream);
size_t tig_file_fread(void* buffer, size_t size, size_t count, TigFile* stream);
size_t tig_file_fwrite(const void* buffer, size_t size, size_t count, TigFile* stream);
int tig_file_fgetpos(TigFile* stream, int* pos_ptr);
int tig_file_fseek(TigFile* stream, int offset, int origin);
int tig_file_fsetpos(TigFile* stream, int* pos);
int tig_file_ftell(TigFile* stream);
void tig_file_rewind(TigFile* stream);
void tig_file_clearerr(TigFile* stream);
int tig_file_feof(TigFile* stream);
int tig_file_ferror(TigFile* stream);
void sub_5308A0(int a1, int a2);
void sub_5308C0(int a1, int a2);
bool tig_file_lock(const char* filename, const void* owner, size_t size);
bool tig_file_unlock(const char* filename, const void* owner, size_t size);
bool tig_file_locked_by(const char* filename, const void* owner, size_t size);
bool sub_530B90(const char* pattern);
bool tig_file_copy(const char* src, const char* dst);

SDL_IOStream* tig_file_io_open(const char* path, const char* mode);

#ifdef __cplusplus
}
#endif

#endif /* TIG_FILE_H_ */
