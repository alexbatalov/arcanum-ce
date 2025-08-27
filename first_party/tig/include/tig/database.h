#ifndef TIG_DATABASE_H_
#define TIG_DATABASE_H_

#include "tig/guid.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TigDatabase TigDatabase;
typedef struct TigDatabaseEntry TigDatabaseEntry;
typedef struct TigDatabaseFileHandle TigDatabaseFileHandle;
typedef struct TigDatabaseFindFileData TigDatabaseFindFileData;

typedef struct TigDatabase {
    /* 0000 */ char* path;
    /* 0004 */ unsigned int entries_count;
    /* 0008 */ TigDatabaseEntry* entries;
    /* 000C */ TigDatabaseFileHandle* open_file_handles_head;
    /* 0010 */ TigDatabase* next;
    /* 0014 */ TigGuid guid;
    /* 0024 */ int field_24;
    /* 0028 */ char* name_table;
} TigDatabase;

#define TIG_DATABASE_ENTRY_PLAIN 0x01
#define TIG_DATABASE_ENTRY_COMPRESSED 0x02
#define TIG_DATABASE_ENTRY_0x100 0x100
#define TIG_DATABASE_ENTRY_0x200 0x200
#define TIG_DATABASE_ENTRY_DIRECTORY 0x400
#define TIG_DATABASE_ENTRY_IGNORED 0x800

typedef struct TigDatabaseEntry {
    /* 0000 */ char* path;
    /* 0004 */ unsigned int flags;
    /* 0008 */ unsigned int size;
    /* 000C */ unsigned int compressed_size;
    /* 0010 */ int offset;
} TigDatabaseEntry;

typedef struct TigDatabaseFindFileData {
    /* 0000 */ TigDatabase* database;
    /* 0004 */ char* pattern;
    /* 0008 */ int path_segments_count;
    /* 000C */ char* name;
    /* 0010 */ int index;
    /* 0014 */ unsigned int size;
    /* 0018 */ bool is_directory;
} TigDatabaseFindFileData;

TigDatabase* tig_database_open(const char* path);
bool tig_database_close(TigDatabase* database);
bool tig_database_find_first_entry(TigDatabase* database, const char* pattern, TigDatabaseFindFileData* ffd);
bool tig_database_find_next_entry(TigDatabaseFindFileData* ffd);
bool tig_database_find_close(TigDatabaseFindFileData* ffd);
int tig_database_filelength(TigDatabaseFileHandle* stream);
bool tig_database_get_entry(TigDatabase* database, const char* path, TigDatabaseEntry** entry_ptr);
int tig_database_fclose(TigDatabaseFileHandle* stream);
int tig_database_fflush(TigDatabaseFileHandle* stream);
TigDatabaseFileHandle* tig_database_fopen(TigDatabase* database, const char* file_name, const char* mode);
TigDatabaseFileHandle* tig_database_reopen(TigDatabase* database, const char* path, const char* mode, TigDatabaseFileHandle* stream);
TigDatabaseFileHandle* tig_database_fopen_entry(TigDatabase* database, TigDatabaseEntry* entry, const char* mode);
int tig_database_setbuf(TigDatabaseFileHandle* stream, char* buffer);
int tig_database_setvbuf(TigDatabaseFileHandle* fp, char* buffer, int mode, size_t size);
int tig_database_vfprintf(TigDatabaseFileHandle* stream, const char* fmt, va_list args);
int tig_database_fgetc(TigDatabaseFileHandle* stream);
int tig_database_fputc(int ch, TigDatabaseFileHandle* stream);
int tig_database_fputs(const char* buffer, TigDatabaseFileHandle* stream);
int tig_database_ungetc(int ch, TigDatabaseFileHandle* stream);
char* tig_database_fgets(char* buffer, int max_count, TigDatabaseFileHandle* stream);
int tig_database_fputc(int ch, TigDatabaseFileHandle* stream);
int tig_database_fputs(const char* buffer, TigDatabaseFileHandle* stream);
int tig_database_ungetc(int ch, TigDatabaseFileHandle* stream);
size_t tig_database_fread(void* buffer, size_t size, size_t count, TigDatabaseFileHandle* stream);
size_t tig_database_fwrite(const void* buffer, size_t size, size_t count, TigDatabaseFileHandle* stream);
int tig_database_fgetpos(TigDatabaseFileHandle* stream, int* off);
int tig_database_fseek(TigDatabaseFileHandle* stream, int offset, int origin);
int tig_database_fsetpos(TigDatabaseFileHandle* stream, int off);
int tig_database_ftell(TigDatabaseFileHandle* stream);
void tig_database_rewind(TigDatabaseFileHandle* stream);
void tig_database_clearerr(TigDatabaseFileHandle* stream);
int tig_database_feof(TigDatabaseFileHandle* stream);
int tig_database_ferror(TigDatabaseFileHandle* stream);

#ifdef __cplusplus
}
#endif

#endif /* TIG_DATABASE_H_ */
