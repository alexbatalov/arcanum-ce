#ifndef ARCANUM_GAME_SA_H_
#define ARCANUM_GAME_SA_H_

#include <tig/tig.h>

typedef struct SizeableArray {
    int size;
    int count;
    int bitset_id;
} SizeableArray;

typedef bool(SizeableArrayEnumerateCallback)(void* entry, int index);

void sa_allocate(SizeableArray** sa_ptr, int size);
void sa_deallocate(SizeableArray** sa_ptr);
void sa_copy(SizeableArray** dst, SizeableArray** src);
void sa_array_copy_to_flat(void* dst, SizeableArray** sa_ptr, int cnt, int size);
bool sa_write_file(SizeableArray** sa_ptr, TigFile* stream);
bool sa_read_file(SizeableArray** sa_ptr, TigFile* stream);
bool sa_enumerate(SizeableArray** sa_ptr, SizeableArrayEnumerateCallback* callback);
void sa_set(SizeableArray** sa_ptr, int key, const void* value);
void sa_get(SizeableArray** sa_ptr, int key, void* value);
bool sa_has(SizeableArray** sa_ptr, int key);
void sa_remove(SizeableArray** sa_ptr, int key);
int sa_count(SizeableArray** sa_ptr);
int sa_total_byte_size(SizeableArray** sa_ptr);
void sa_write_mem(SizeableArray** src, SizeableArray* dst);
void sa_read_mem(SizeableArray** sa_ptr, uint8_t** data);
bool sa_read_no_dealloc(SizeableArray** sa_ptr, TigFile* stream);

#endif /* ARCANUM_GAME_SA_H_ */
