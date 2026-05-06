#ifndef ARCANUM_GAME_OBJ_ID_H_
#define ARCANUM_GAME_OBJ_ID_H_

#include "game/context.h"

#define OID_STR_LEN 40

typedef struct ObjectID_P {
    int64_t location;
    int temp_id;
    int map;
} ObjectID_P;

#define OID_TYPE_HANDLE ((int16_t)(-2))
#define OID_TYPE_BLOCKED ((int16_t)(-1))
#define OID_TYPE_NULL ((int16_t)0)
#define OID_TYPE_A ((int16_t)1)
#define OID_TYPE_GUID ((int16_t)2)
#define OID_TYPE_P ((int16_t)3)

typedef struct ObjectID {
    /* 0000 */ int16_t type;
    /* 0002 */ int16_t padding_2;
    /* 0004 */ int padding_4;
    /* 0008 */ union {
        int64_t h;
        int a;
        TigGuid g;
        ObjectID_P p;
    } d;
} ObjectID;

// Serializeable.
static_assert(sizeof(ObjectID) == 0x18, "wrong size");

void objid_create_guid(ObjectID* oid);
void objid_id_perm_by_load_order(ObjectID* oid, int64_t obj);
bool objid_is_valid(ObjectID a);
bool objid_compare(ObjectID a, ObjectID b);
bool objid_is_equal(ObjectID a, ObjectID b);
ObjectID sub_4E6540(int a1);
void objid_id_to_str(char* buffer, ObjectID oid);
bool objid_id_from_str(ObjectID* oid, const char* str);

#endif /* ARCANUM_GAME_OBJ_ID_H_ */
