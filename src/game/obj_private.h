#ifndef ARCANUM_GAME_OBJ_PRIVATE_H_
#define ARCANUM_GAME_OBJ_PRIVATE_H_

#include "game/obj.h"
#include "game/obj_id.h"
#include "game/obj_pool.h"
#include "game/quest.h"
#include "game/sa.h"
#include "game/script.h"

typedef enum SaType {
    SA_TYPE_INVALID = 0,
    SA_TYPE_BEGIN = 1,
    SA_TYPE_END = 2,
    SA_TYPE_INT32 = 3,
    SA_TYPE_INT64 = 4,
    SA_TYPE_INT32_ARRAY = 5,
    SA_TYPE_INT64_ARRAY = 6,
    SA_TYPE_UINT32_ARRAY = 7,
    SA_TYPE_UINT64_ARRAY = 8,
    SA_TYPE_SCRIPT = 9,
    SA_TYPE_QUEST = 10,
    SA_TYPE_STRING = 11,
    SA_TYPE_HANDLE = 12,
    SA_TYPE_HANDLE_ARRAY = 13,
    SA_TYPE_PTR = 14,
    SA_TYPE_PTR_ARRAY = 15,
} SaType;

typedef struct WriteBuffer {
    uint8_t* base;
    uint8_t* curr;
    int size;
    int remaining;
} WriteBuffer;

typedef struct ObjSa {
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
} ObjSa;

void sub_4E3F80();
void sub_4E3F90();
void sub_4E3FA0(ObjSa* a1);
void sub_4E4000(ObjSa* a1);
void sub_4E4180(ObjSa* a1);
void sub_4E4280(ObjSa* a1, void* value);
bool sub_4E4360(ObjSa* a1, TigFile* stream);
bool sub_4E44F0(ObjSa* a1, TigFile* stream);
void sub_4E4660(ObjSa* a1, uint8_t** data);
bool sub_4E47E0(ObjSa* a1, TigFile* stream);
void sub_4E4990(ObjSa* a1, WriteBuffer* wb);
void sub_4E4B70(ObjSa* a1);
int sub_4E4BA0(ObjSa* a1);
void write_buffer_init(WriteBuffer* wb);
void write_buffer_append(const void* data, int size, WriteBuffer* wb);
void sub_4E4C50(void* buffer, int size, uint8_t** data);
void sub_4E59B0();
void sub_4E5A50();
int sub_4E5AA0();
int sub_4E5B40(int a1);
int sub_4E5BF0(int a1);
void sub_4E5C60(int a1, int a2, bool a3);
int sub_4E5CE0(int a1, int a2);
int sub_4E5D30(int a1, int a2);
bool sub_4E5DB0(int a1, bool (*callback)(int));
bool sub_4E5E20(int a1, TigFile* stream);
bool sub_4E5E80(int* a1, TigFile* stream);
int sub_4E5F10(int a1);
void sub_4E5F30(int a1, void* a2);
void sub_4E5F70(int* a1, uint8_t** data);
int sub_4E5FE0(int a1, int a2);

#endif /* ARCANUM_GAME_OBJ_PRIVATE_H_ */
