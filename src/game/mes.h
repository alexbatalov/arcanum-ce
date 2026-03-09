#ifndef ARCANUM_GAME_MES_H_
#define ARCANUM_GAME_MES_H_

#include <stdbool.h>

#define MAX_STRING 2000

typedef int mes_file_handle_t;
#define MES_FILE_HANDLE_INVALID ((mes_file_handle_t)(-1))

typedef struct MesFileEntry {
    /* 0000 */ int num;
    /* 0004 */ char* str;
} MesFileEntry;

bool mes_load(const char* path, mes_file_handle_t* mes_file_handle_ptr);
void mes_get_msg(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry);
bool mes_find_first(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry);
bool mes_find_next(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry);
int mes_entries_count(mes_file_handle_t mes_file_handle);
bool mes_get_entry(mes_file_handle_t mes_file_handle, int index, MesFileEntry* mes_file_entry);
int mes_entries_count_in_range(mes_file_handle_t mes_file_handle, int start, int end);
bool mes_search(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry);
bool mes_unload(mes_file_handle_t mes_file_handle);
int mes_num_entries(mes_file_handle_t mes_file_handle);
void mes_merge(mes_file_handle_t dst, mes_file_handle_t src);
void mes_stats(void);

#endif /* ARCANUM_GAME_MES_H_ */
