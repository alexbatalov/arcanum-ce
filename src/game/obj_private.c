#include "game/obj_private.h"

#include <stdio.h>

#include "game/map.h"
#include "game/obj_file.h"
#include "game/object.h"
#include "game/sector.h"

typedef struct S603710 {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
} S603710;

typedef struct S603720 {
    uint16_t field_0;
    uint16_t field_2;
} S603720;

static void write_buffer_ensure_size(WriteBuffer* wb, int size);
static void sub_4E6040(int a1, int a2);
static void sub_4E6130(int a1, int a2);
static void sub_4E61B0(int start, int end, int inc);
static int sub_4E61E0(int a1);
static int sub_4E61F0(int a1);
static void sub_4E6210();
static void sub_4E6240();

// 0x6036A8
static bool dword_6036A8;

// 0x6036FC
static uint8_t* dword_6036FC;

// 0x603700
static int dword_603700;

// 0x603704
static int dword_603704;

// 0x603708
static int dword_603708;

// Capacity: 0x603704
// Size: 0x603714
//
// 0x60370C
static int* dword_60370C;

// Capacity: 0x603700
// Size: 0x603724
//
// 0x603710
static S603710* dword_603710;

// 0x603714
static int dword_603714;

// Capacity: 0x60371C
//
// 0x603718
static int* dword_603718;

// 0x60371C
static int dword_60371C;

// 0x603720
static S603720* dword_603720;

// 0x603724
static int dword_603724;

// 0x603728
static bool dword_603728;

// 0x4E3F80
void sub_4E3F80()
{
    dword_6036A8 = true;
}

// 0x4E3F90
void sub_4E3F90()
{
    dword_6036A8 = false;
}

// 0x4E3FA0
void sub_4E3FA0(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        *(int*)a1->ptr = 0;
        break;
    case SA_TYPE_INT64:
    case SA_TYPE_STRING:
    case SA_TYPE_HANDLE:
        if (*(void**)a1->ptr != NULL) {
            FREE(*(void**)a1->ptr);
            *(void**)a1->ptr = NULL;
        }
        break;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_deallocate((SizeableArray**)a1->ptr);
        }
        break;
    case SA_TYPE_PTR:
        *(intptr_t*)a1->ptr = 0;
        break;
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_deallocate((SizeableArray**)a1->ptr);
        }
        break;
    }
}

// 0x4E4000
void sub_4E4000(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        *(int*)a1->ptr = a1->storage.value;
        break;
    case SA_TYPE_INT64:
        if (a1->storage.value64 != 0) {
            if (*(int64_t**)a1->ptr == NULL) {
                *(int64_t**)a1->ptr = MALLOC(sizeof(int64_t));
            }
            **(int64_t**)a1->ptr = a1->storage.value64;
        } else {
            if (*(int64_t**)a1->ptr != NULL) {
                FREE(*(int64_t**)a1->ptr);
                *(int64_t**)a1->ptr = NULL;
            }
        }
        break;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
        if (*(SizeableArray**)a1->ptr == NULL) {
            sa_allocate((SizeableArray**)a1->ptr, sizeof(int));
        }
        sa_set((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        break;
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
        if (*(SizeableArray**)a1->ptr == NULL) {
            sa_allocate((SizeableArray**)a1->ptr, sizeof(int64_t));
        }
        sa_set((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        break;
    case SA_TYPE_SCRIPT:
        if (*(SizeableArray**)a1->ptr == NULL) {
            sa_allocate((SizeableArray**)a1->ptr, sizeof(Script));
        }
        sa_set((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        break;
    case SA_TYPE_QUEST:
        if (*(SizeableArray**)a1->ptr == NULL) {
            sa_allocate((SizeableArray**)a1->ptr, sizeof(PcQuestState));
        }
        sa_set((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        break;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            FREE(*(char**)a1->ptr);
        }
        *(char**)a1->ptr = STRDUP(a1->storage.str);
        break;
    case SA_TYPE_HANDLE:
        if (a1->storage.oid.type != OID_TYPE_NULL) {
            if (*(ObjectID**)a1->ptr == NULL) {
                *(ObjectID**)a1->ptr = MALLOC(sizeof(ObjectID));
            }
            **(ObjectID**)a1->ptr = a1->storage.oid;
            break;
        } else {
            if (*(ObjectID**)a1->ptr != NULL) {
                FREE(*(ObjectID**)a1->ptr);
                *(ObjectID**)a1->ptr = NULL;
            }
        }
        break;
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr == NULL) {
            sa_allocate((SizeableArray**)a1->ptr, sizeof(ObjectID));
        }
        sa_set((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        break;
    case SA_TYPE_PTR:
        *(intptr_t*)a1->ptr = a1->storage.ptr;
        break;
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr == NULL) {
            sa_allocate((SizeableArray**)a1->ptr, sizeof(intptr_t));
        }
        sa_set((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        break;
    }
}

// 0x4E4180
void sub_4E4180(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        a1->storage.value = *(int*)a1->ptr;
        break;
    case SA_TYPE_INT64:
        if (*(int64_t**)a1->ptr != NULL) {
            a1->storage.value64 = **(int64_t**)a1->ptr;
        } else {
            a1->storage.value64 = 0;
        }
        break;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.value = 0;
        }
        break;
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.value64 = 0;
        }
        break;
    case SA_TYPE_SCRIPT:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            memset(&(a1->storage.scr), 0, sizeof(a1->storage.scr));
        }
        break;
    case SA_TYPE_QUEST:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            memset(&(a1->storage.quest), 0, sizeof(a1->storage.quest));
        }
        break;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            a1->storage.str = STRDUP(*(char**)a1->ptr);
        } else {
            a1->storage.str = NULL;
        }
        break;
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)a1->ptr != NULL) {
            a1->storage.oid = **(ObjectID**)a1->ptr;
        } else {
            a1->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    case SA_TYPE_PTR:
        a1->storage.ptr = *(intptr_t*)a1->ptr;
        break;
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.ptr = 0;
        }
        break;
    }
}

// 0x4E4280
void sub_4E4280(ObjSa* a1, void* value)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        *(int*)value = *(int*)a1->ptr;
        break;
    case SA_TYPE_INT64:
        if (*(int64_t**)a1->ptr != NULL) {
            *(int64_t**)value = (int64_t*)MALLOC(sizeof(int64_t));
            **(int64_t**)value = **(int64_t**)a1->ptr;
        } else {
            *(int64_t**)value = NULL;
        }
        break;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sub_4E74A0((SizeableArray**)value, (SizeableArray**)a1->ptr);
        } else {
            *(SizeableArray**)value = NULL;
        }
        break;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            *(char**)value = STRDUP(*(char**)a1->ptr);
        } else {
            *(char**)value = NULL;
        }
        break;
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)a1->ptr != NULL) {
            *(ObjectID**)value = (ObjectID*)MALLOC(sizeof(ObjectID));
            **(ObjectID**)value = **(ObjectID**)a1->ptr;
        } else {
            *(ObjectID**)value = NULL;
        }
        break;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    }
}

// 0x4E4360
bool sub_4E4360(ObjSa* a1, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        if (!objf_read(a1->ptr, sizeof(int), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_INT64:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            if (*(int64_t**)a1->ptr != NULL) {
                FREE(*(int64_t**)a1->ptr);
                *(int64_t**)a1->ptr = NULL;
            }
            return true;
        }
        if (*(int64_t**)a1->ptr == NULL) {
            *(int64_t**)a1->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }
        if (!objf_read(*(int64_t**)a1->ptr, sizeof(int64_t), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(SizeableArray**)a1->ptr = NULL;
            return true;
        }
        if (!sa_read((SizeableArray**)a1->ptr, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_STRING:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (*(char**)a1->ptr != NULL) {
            FREE(*(char**)a1->ptr);
        }
        if (!presence) {
            *(char**)a1->ptr = NULL;
            return true;
        }
        if (!objf_read(&size, sizeof(size), stream)) {
            return false;
        }
        *(char**)a1->ptr = (char*)MALLOC(size + 1);
        if (!objf_read(*(char**)a1->ptr, size + 1, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_HANDLE:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            if (*(ObjectID**)a1->ptr != NULL) {
                FREE(*(ObjectID**)a1->ptr);
                *(ObjectID**)a1->ptr = NULL;
            }
            return true;
        }
        if (*(ObjectID**)a1->ptr == NULL) {
            *(ObjectID**)a1->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }
        if (!objf_read(*(ObjectID**)a1->ptr, sizeof(ObjectID), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    default:
        return false;
    }
}

// 0x4E44F0
bool sub_4E44F0(ObjSa* a1, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        if (!objf_read(a1->ptr, sizeof(int), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_INT64:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(int64_t**)a1->ptr = NULL;
            return true;
        }
        *(int64_t**)a1->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        if (!objf_read(*(int64_t**)a1->ptr, sizeof(int64_t), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(SizeableArray**)a1->ptr = NULL;
            return true;
        }
        if (!sa_read_no_dealloc((SizeableArray**)a1->ptr, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_STRING:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(char**)a1->ptr = NULL;
            return true;
        }
        if (!objf_read(&size, sizeof(size), stream)) {
            return false;
        }
        *(char**)a1->ptr = (char*)MALLOC(size + 1);
        if (!objf_read(*(char**)a1->ptr, size + 1, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_HANDLE:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(ObjectID**)a1->ptr = NULL;
            return true;
        }
        *(ObjectID**)a1->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        if (!objf_read(*(ObjectID**)a1->ptr, sizeof(ObjectID), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    default:
        return false;
    }
}

// 0x4E4660
void sub_4E4660(ObjSa* a1, uint8_t** data)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        sub_4E4C50(a1->ptr, sizeof(int), data);
        return;
    case SA_TYPE_INT64:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (!presence) {
            if (*(int64_t**)a1->ptr != NULL) {
                FREE(*(int64_t**)a1->ptr);
                *(int64_t**)a1->ptr = NULL;
            }
            return;
        }
        if (*(int64_t**)a1->ptr == NULL) {
            *(int64_t**)a1->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }
        sub_4E4C50(*(int64_t**)a1->ptr, sizeof(int64_t), data);
        return;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (!presence) {
            if (*(SizeableArray**)a1->ptr != NULL) {
                FREE(*(SizeableArray**)a1->ptr);
                *(SizeableArray**)a1->ptr = NULL;
            }
            return;
        }
        sub_4E7820((SizeableArray**)a1->ptr, data);
        return;
    case SA_TYPE_STRING:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (*(char**)a1->ptr != NULL) {
            FREE(*(char**)a1->ptr);
        }
        if (!presence) {
            *(char**)a1->ptr = NULL;
            return;
        }
        sub_4E4C50(&size, sizeof(size), data);
        *(char**)a1->ptr = (char*)MALLOC(size + 1);
        sub_4E4C50(*(char**)a1->ptr, size + 1, data);
        return;
    case SA_TYPE_HANDLE:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (!presence) {
            if (*(ObjectID**)a1->ptr != NULL) {
                FREE(*(ObjectID**)a1->ptr);
                *(ObjectID**)a1->ptr = NULL;
            }
            return;
        }
        if (*(ObjectID**)a1->ptr == NULL) {
            *(ObjectID**)a1->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }
        sub_4E4C50(*(ObjectID**)a1->ptr, sizeof(ObjectID), data);
        return;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    }
}

// 0x4E47E0
bool sub_4E47E0(ObjSa* a1, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        if (!objf_write((int*)a1->ptr, sizeof(int), stream)) return false;
        return true;
    case SA_TYPE_INT64:
        if (*(int64_t**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!objf_write(*(int64_t**)a1->ptr, sizeof(int64_t), stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!sa_write((SizeableArray**)a1->ptr, stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            size = (int)strlen(*(char**)a1->ptr);
            if (!objf_write(&size, sizeof(size), stream)) return false;
            if (!objf_write(*(char**)a1->ptr, size + 1, stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!objf_write(*(ObjectID**)a1->ptr, sizeof(ObjectID), stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    default:
        return false;
    }
}

// 0x4E4990
void sub_4E4990(ObjSa* a1, WriteBuffer* wb)
{
    // uint8_t presence;
    // int size;

    // switch (a1->type) {
    // case 3:
    //     write_buffer_append(a1->d.int_value, sizeof(a1->d.int_value), wb);
    //     break;
    // case 4:
    //     if (*a1->d.b != NULL) {
    //         presence = 1;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //         write_buffer_append(*a1->d.b, sizeof(*a1->d.b), wb);
    //     } else {
    //         presence = 0;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //     }
    //     break;
    // case 5:
    // case 6:
    // case 7:
    // case 8:
    // case 9:
    // case 10:
    // case 13:
    //     if (*a1->d.a.sa_ptr != NULL) {
    //         presence = 1;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //         size = sub_4E77B0(a1->d.a.sa_ptr);
    //         write_buffer_ensure_size(wb, size);
    //         sub_4E77E0(a1->d.a.sa_ptr, wb->field_4);
    //         wb->field_4 += size;
    //         wb->field_C -= size;
    //     } else {
    //         presence = 0;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //     }
    //     break;
    // case 11:
    //     if (*a1->d.str != NULL) {
    //         presence = 1;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //         size = strlen(*a1->d.str);
    //         write_buffer_append(&size, sizeof(size), wb);
    //         write_buffer_append(*a1->d.str, size + 1, wb);
    //     } else {
    //         presence = 0;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //     }
    //     break;
    // case 12:
    //     if (*a1->d.oid != NULL) {
    //         presence = 1;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //         write_buffer_append(*a1->d.oid, sizeof(**a1->d.oid), wb);
    //     } else {
    //         presence = 0;
    //         write_buffer_append(&presence, sizeof(presence), wb);
    //     }
    //     break;
    // }
}

// 0x4E4B70
void sub_4E4B70(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sub_4E7760((SizeableArray**)a1->ptr, a1->idx);
        }
        break;
    }
}

// 0x4E4BA0
int sub_4E4BA0(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            return sa_count((SizeableArray**)a1->ptr);
        }
        break;
    }

    return 0;
}

// 0x4E4BD0
void write_buffer_init(WriteBuffer* wb)
{
    wb->base = (uint8_t*)MALLOC(256);
    wb->curr = wb->base;
    wb->size = 256;
    wb->remaining = wb->size;
}

// 0x4E4C00
void write_buffer_append(const void* data, int size, WriteBuffer* wb)
{
    write_buffer_ensure_size(wb, size);
    memcpy(wb->curr, data, size);
    wb->curr += size;
    wb->remaining -= size;
}

// 0x4E4C50
void sub_4E4C50(void* buffer, int size, uint8_t** data)
{
    memcpy(buffer, *data, size);
    (*data) += size;
}

// 0x4E4C80
void write_buffer_ensure_size(WriteBuffer* wb, int size)
{
    int extra_size;
    int new_size;

    extra_size = size - wb->remaining;
    if (extra_size > 0) {
        new_size = (extra_size / 256 + 1) * 256;
        wb->size += new_size;
        wb->base = (uint8_t*)REALLOC(wb->base, wb->size);
        wb->curr = wb->base + wb->size - wb->remaining - new_size;
        wb->remaining += new_size;
    }
}

// 0x4E59B0
void sub_4E59B0()
{
    dword_603724 = 0;
    dword_603700 = 4096;
    dword_603714 = 0;
    dword_603704 = 4096;
    dword_603708 = 0;
    dword_60371C = 8192;
    dword_603710 = (S603710*)MALLOC(sizeof(*dword_603710) * dword_603700);
    dword_60370C = (int*)MALLOC(sizeof(*dword_60370C) * dword_603704);
    dword_603718 = (int*)MALLOC(sizeof(*dword_603718) * dword_60371C);
    dword_6036FC = (uint8_t*)MALLOC(65536);
    dword_603720 = (S603720*)MALLOC(sizeof(*dword_603720) * 33);
    sub_4E6210();
    sub_4E6240();
    dword_603728 = true;
}

// 0x4E5A50
void sub_4E5A50()
{
    FREE(dword_603720);
    FREE(dword_6036FC);
    FREE(dword_603718);
    FREE(dword_60370C);
    FREE(dword_603710);
    dword_603728 = false;
}

// 0x4E5AA0
int sub_4E5AA0()
{
    int index;

    if (dword_603714 != 0) {
        return dword_60370C[--dword_603714];
    }

    if (dword_603724 == dword_603700) {
        dword_603700 += 4096;
        dword_603710 = (S603710*)REALLOC(dword_603710, sizeof(S603710) * dword_603700);
    }

    index = dword_603724++;
    if (index == 0) {
        dword_603710[0].field_4 = index;
    } else {
        dword_603710[index].field_4 = dword_603710[index - 1].field_0 + dword_603710[index - 1].field_4;
    }

    dword_603710[index].field_0 = 0;
    sub_4E6040(index, 2);

    return index;
}

// 0x4E5B40
int sub_4E5B40(int a1)
{
    S603710* v1;
    int index;

    v1 = &(dword_603710[a1]);
    if (v1->field_0 != 2) {
        sub_4E6130(a1, v1->field_0 - 2);
    }

    for (index = v1->field_4; index < v1->field_0 + v1->field_4; index++) {
        dword_603718[index] = 0;
    }

    if (dword_603714 == dword_603704) {
        dword_603704 += 4096;
        dword_60370C = (int*)REALLOC(dword_60370C, sizeof(int) * dword_603704);
    }

    dword_60370C[dword_603714] = a1;

    return ++dword_603714;
}

// 0x4E5BF0
int sub_4E5BF0(int a1)
{
    int v1;
    int v2;
    int index;

    v1 = sub_4E5AA0();
    v2 = dword_603710[a1].field_0 - dword_603710[v1].field_0;
    if (v2 != 0) {
        sub_4E6040(v1, v2);
    }

    for (index = 0; index < dword_603710[a1].field_0; index++) {
        dword_603718[dword_603710[v1].field_4 + index] = dword_603718[dword_603710[a1].field_4 + index];
    }

    return v1;
}

// 0x4E5C60
void sub_4E5C60(int a1, int a2, bool a3)
{
    int v1;
    int v2;

    v1 = sub_4E61E0(a2);
    if (v1 >= dword_603710[a1].field_0) {
        if (!a3) {
            return;
        }

        sub_4E6040(a1, v1 - dword_603710[a1].field_0 + 1);
    }

    v2 = sub_4E61F0(a2);
    if (a3) {
        dword_603718[v1 + dword_603710[a1].field_4] |= v2;
    } else {
        dword_603718[v1 + dword_603710[a1].field_4] &= ~v2;
    }
}

// 0x4E5CE0
int sub_4E5CE0(int a1, int a2)
{
    int v1;
    int v2;

    v1 = sub_4E61E0(a2);
    if (v1 > dword_603710[a1].field_0 - 1) {
        return 0;
    }

    v2 = sub_4E61F0(a2);
    return v2 & dword_603718[v1 + dword_603710[a1].field_4];
}

// 0x4E5D30
int sub_4E5D30(int a1, int a2)
{
    int v1 = 0;
    int v2;
    int v3;
    int v4;

    v2 = dword_603710[a1].field_4;
    v3 = sub_4E61E0(a2);
    v4 = v2 + v3;
    if (v3 < dword_603710[a1].field_0) {
        v1 += sub_4E5FE0(dword_603718[v4], a2 % 32);
    } else {
        v4 = v2 + dword_603710[a1].field_0;
    }

    while (v2 < v4) {
        v1 += sub_4E5FE0(dword_603718[v2++], 32);
    }

    return v1;
}

// 0x4E5DB0
bool sub_4E5DB0(int a1, bool (*callback)(int))
{
    int v1;
    int v2;
    int pos;
    int idx;
    int bit;
    unsigned int flags;

    v1 = dword_603710[a1].field_4;
    v2 = dword_603710[a1].field_0 + v1;

    pos = 0;
    for (idx = v1; idx < v2; idx++) {
        flags = 1;
        for (bit = 0; bit < 32; bit++) {
            if ((flags & dword_603718[idx]) != 0) {
                if (!callback(pos)) {
                    return false;
                }
            }
            flags *= 2;
            pos++;
        }
    }

    return true;
}

// 0x4E5E20
bool sub_4E5E20(int a1, TigFile* stream)
{
    if (tig_file_fwrite(&(dword_603710[a1].field_0), sizeof(int), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(dword_603718[dword_603710[a1].field_4]), sizeof(int) * dword_603710[a1].field_0, 1, stream) != 1) {
        return false;
    }

    return true;
}

// 0x4E5E80
bool sub_4E5E80(int* a1, TigFile* stream)
{
    int v1;
    int v2;

    v1 = sub_4E5AA0();

    if (tig_file_fread(&v2, sizeof(v2), 1, stream) != 1) {
        return false;
    }

    if (v2 != dword_603710[v1].field_0 && v2 - dword_603710[v1].field_0 > 0) {
        sub_4E6040(v1, v2 - dword_603710[v1].field_0);
    }

    if (tig_file_fread(&(dword_603718[dword_603710[v1].field_4]), 4 * v2, 1, stream) != 1) {
        return false;
    }

    *a1 = v1;

    return true;
}

// 0x4E5F10
int sub_4E5F10(int a1)
{
    return 4 * (dword_603710[a1].field_0 + 1);
}

// 0x4E5F30
void sub_4E5F30(int a1, void* a2)
{
    int v1;
    int v2;

    v1 = dword_603710[a1].field_4;
    v2 = dword_603710[a1].field_0;

    *(int*)a2 = dword_603710[a1].field_0;
    memcpy((int*)a2 + 1, &(dword_603718[v1]), sizeof(int) * v2);
}

// 0x4E5F70
void sub_4E5F70(int* a1, uint8_t** data)
{
    int v1;
    int v2;

    v1 = sub_4E5AA0();

    sub_4E4C50(&v2, sizeof(v2), data);

    if (v2 != dword_603710[v1].field_0) {
        sub_4E6040(v1, v2 - dword_603710[v1].field_0);
    }

    sub_4E4C50(&(dword_603718[dword_603710[v1].field_4]), 4 * v2, data);

    *a1 = v1;
}

// 0x4E5FE0
int sub_4E5FE0(int a1, int a2)
{
    return dword_6036FC[dword_603720[a2].field_0 & (a1 & 0xFFFF)]
        + dword_6036FC[dword_603720[a2].field_2 & ((a1 >> 16) & 0xFFFF)];
}

// 0x4E6040
void sub_4E6040(int a1, int a2)
{
    int* v1;
    int idx;

    if (dword_603708 + a2 > dword_60371C) {
        dword_60371C += ((dword_603708 + a2 - dword_60371C - 1) / 8192 + 1) * 8192;
        dword_603718 = REALLOC(dword_603718, sizeof(*dword_603718) * dword_60371C);
    }

    if (a1 != dword_603724 - 1) {
        v1 = &(dword_603718[dword_603710[a1 + 1].field_4]);
        memmove(&(v1[a2]), v1, sizeof(*v1) * (dword_603708 - dword_603710[a1 + 1].field_4));
        sub_4E61B0(a1 + 1, dword_603724 - 1, a2);
    }

    dword_603710[a1].field_0 += a2;
    dword_603708 += a2;

    for (idx = dword_603710[a1].field_0 + dword_603710[a1].field_4 - a2; idx < dword_603710[a1].field_0 + dword_603710[a1].field_4; idx++) {
        dword_603718[idx] = 0;
    }
}

// 0x4E6130
void sub_4E6130(int a1, int a2)
{
    int* v1;

    if (a1 != dword_603724 - 1) {
        v1 = &(dword_603718[dword_603710[a1 + 1].field_4]);
        memmove(&(v1[-a2]), v1, sizeof(*v1) * (dword_603708 - dword_603710[a1 + 1].field_4));
        sub_4E61B0(a1 + 1, dword_603724 - 1, -a2);
    }

    dword_603710[a1].field_0 -= a2;
    dword_603708 -= a2;
}

// 0x4E61B0
void sub_4E61B0(int start, int end, int inc)
{
    int index;

    for (index = start; index <= end; index++) {
        dword_603710[index].field_4 += inc;
    }
}

// 0x4E61E0
int sub_4E61E0(int a1)
{
    return a1 / 32;
}

// 0x4E61F0
int sub_4E61F0(int a1)
{
    return 1 << (a1 % 32);
}

// 0x4E6210
void sub_4E6210()
{
    int idx;
    uint8_t v1;
    int v2;
    int v3;

    for (idx = 0; idx <= 65535; idx++) {
        v1 = 0;
        v2 = 1;
        for (v3 = 16; v3 != 0; v3--) {
            if ((v2 & idx) != 0) {
                v1++;
            }
            v2 *= 2;
        }
        dword_6036FC[idx] = v1;
    }
}

// 0x4E6240
void sub_4E6240()
{
    int idx;
    int v1 = 1;
    int v2 = 0;

    dword_603720[0].field_0 = 0;
    dword_603720[0].field_2 = 0;

    for (idx = 1; idx <= 16; idx++) {
        v2 += v1;
        v1 *= 2;

        dword_603720[idx].field_0 = v2;
        dword_603720[idx].field_2 = 0;
        dword_603720[idx + 16].field_0 = -1;
        dword_603720[idx + 16].field_2 = v2;
    }
}
