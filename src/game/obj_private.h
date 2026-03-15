#ifndef ARCANUM_GAME_OBJ_PRIVATE_H_
#define ARCANUM_GAME_OBJ_PRIVATE_H_

#include "game/obj.h"
#include "game/obj_id.h"
#include "game/obj_pool.h"
#include "game/quest.h"
#include "game/sa.h"
#include "game/script.h"

typedef enum ObjDataType {
    OD_TYPE_INVALID = 0,
    OD_TYPE_BEGIN = 1,
    OD_TYPE_END = 2,
    OD_TYPE_INT32 = 3,
    OD_TYPE_INT64 = 4,
    OD_TYPE_INT32_ARRAY = 5,
    OD_TYPE_INT64_ARRAY = 6,
    OD_TYPE_UINT32_ARRAY = 7,
    OD_TYPE_UINT64_ARRAY = 8,
    OD_TYPE_SCRIPT_ARRAY = 9,
    OD_TYPE_QUEST_ARRAY = 10,
    OD_TYPE_STRING = 11,
    OD_TYPE_HANDLE = 12,
    OD_TYPE_HANDLE_ARRAY = 13,
    OD_TYPE_PTR = 14,
    OD_TYPE_PTR_ARRAY = 15,
} ObjDataType;

typedef struct WriteBuffer {
    uint8_t* base;
    uint8_t* curr;
    int size;
    int remaining;
} WriteBuffer;

typedef struct ObjDataDescriptor {
    /* 0000 */ int type;
    /* 0004 */ void* ptr;
    /* 0008 */ int idx;
    /* 000C */ int field_C;
    /* 0010 */ union {
        int value;
        int64_t value64;
        char* str;
        ObjectID oid;
        Script scr;
        PcQuestState quest;
        intptr_t ptr;
    } storage;
} ObjDataDescriptor;

void obj_data_init(void);
void obj_data_exit(void);
void obj_data_reset(ObjDataDescriptor* descr);
void obj_data_store(ObjDataDescriptor* descr);
void obj_data_fetch(ObjDataDescriptor* descr);
void obj_data_copy(ObjDataDescriptor* descr, void* value);
bool obj_data_read_file_slow(ObjDataDescriptor* descr, TigFile* stream);
bool obj_data_read_file_fast(ObjDataDescriptor* descr, TigFile* stream);
void obj_data_read_mem(ObjDataDescriptor* descr, uint8_t** data);
bool obj_data_write_file(ObjDataDescriptor* descr, TigFile* stream);
void obj_data_write_mem(ObjDataDescriptor* descr, WriteBuffer* wb);
void obj_data_remove_idx(ObjDataDescriptor* descr);
int obj_data_count_idx(ObjDataDescriptor* descr);
void write_buffer_init(WriteBuffer* wb);
void write_buffer_append(const void* data, int size, WriteBuffer* wb);
void mem_read_advance(void* buffer, int size, uint8_t** data);
void bitset_pool_init(void);
void bitset_pool_exit(void);
int bitset_alloc(void);
void bitset_free(int id);
int bitset_copy(int id);
void bitset_set(int id, int pos, bool val);
bool bitset_test(int id, int pos);
int bitset_rank(int id, int pos);
bool bitset_enumerate(int id, bool (*callback)(int));
bool bitset_write_file(int id, TigFile* stream);
bool bitset_read_file(int* id, TigFile* stream);
int bitset_serialized_size(int id);
void bitset_write_mem(int id, uint8_t* data);
void bitset_read_mem(int* id, uint8_t** data);
int bitset_count_bits(int value, int n);

#endif /* ARCANUM_GAME_OBJ_PRIVATE_H_ */
