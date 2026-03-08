#include "game/obj_private.h"

#include <stdio.h>

#include "game/map.h"
#include "game/obj_file.h"
#include "game/object.h"
#include "game/sector.h"

/**
 * Initial capacity for the `BitsetDescriptor` array, and the granularity at
 * which it is grown when more bitsets are needed.
 */
#define BITSET_DESCRIPTORS_INITIAL_CAPACITY 4096
#define BITSET_DESCRIPTORS_GROWTH_STEP BITSET_DESCRIPTORS_INITIAL_CAPACITY

/**
 * Initial capacity for the free indices array, and the granularity at
 * which it is grown when more bitsets are freed.
 */
#define BITSET_FREE_LIST_INITIAL_CAPACITY 4096
#define BITSET_FREE_LIST_GROWTH_STEP BITSET_FREE_LIST_INITIAL_CAPACITY

/**
 * Initial capacity for the flat storage array that backs all live bitsets, and
 * the granularity at which it is grown when more space is needed.
 */
#define BITSET_STORAGE_INITIAL_CAPACITY 8192
#define BITSET_STORAGE_GROWTH_STEP BITSET_STORAGE_INITIAL_CAPACITY

/**
 * Every newly allocated (or recycled) bitset starts with this many 32-bit
 * storage items, giving a minimum capacity of `BITSET_MIN_STORAGE_ITEMS` *
 * `BITS_PER_STORAGE_ITEM` bits.
 */
#define BITSET_MIN_STORAGE_ITEMS 2

/**
 * Number of bits that fit in one storage item (int).
 */
#define BITS_PER_STORAGE_ITEM 32

/**
 * Number of entries in the 16-bit population-count lookup table.
 */
#define POPCOUNT_TABLE_SIZE 65536

/**
 * Number of entries in the split-mask table used by `bitset_count_bits`.
 */
#define POPCOUNT_MASKS_COUNT (BITS_PER_STORAGE_ITEM + 1)

typedef struct BitsetDescriptor {
    /* 0000 */ int cnt;
    /* 0004 */ int offset;
} BitsetDescriptor;

typedef struct PopCountMask {
    uint16_t lo_mask;
    uint16_t hi_mask;
} PopCountMask;

static void write_buffer_ensure_size(WriteBuffer* wb, int size);
static void bitset_grow(int id, int cnt);
static void bitset_shrink(int id, int cnt);
static void bitset_shift_offsets(int start, int end, int delta);
static int bitset_index_of(int bit);
static int bitset_mask_of(int bit);
static void popcount_table_init();
static void popcount_masks_init();

// 0x6036A8
static bool dword_6036A8;

/**
 * Lookup table for every 16-bit value.
 *
 * `popcount_table[n]` is the number of set bits in the 16-bit value `n`.
 *
 * Size: `65536`
 *
 * 0x6036FC
 */
static uint8_t* popcount_tbl;

/**
 * Capacity of the `bitset_descriptors` array.
 *
 * 0x603700
 */
static int bitset_descriptors_capacity;

/**
 * Capacity of the `bitset_free_list` array.
 *
 * 0x603704
 */
static int bitset_free_list_capacity;

/**
 * Total number of 32-bit storage items currently in use across all live
 * bitsets.
 *
 * 0x603708
 */
static int bitset_storage_size;

/**
 * List of recycled bitset indices available for reuse.
 *
 * Capacity: `bitset_free_list_capacity`
 * Size: `bitset_free_list_count`
 *
 * 0x60370C
 */
static int* bitset_free_list;

/**
 * Array of `BitsetDescriptor` entries, one per allocated bitset.
 *
 * Capacity: `bitset_descriptors_capacity`
 * Size: `bitset_count`
 *
 * 0x603710
 */
static BitsetDescriptor* bitset_descriptors;

/**
 * Current number of entries on the `bitset_free_list`.
 *
 * 0x603714
 */
static int bitset_free_list_count;

/**
 * Flat array that stores the actual bit data for all live bitsets. Each
 * bitset owns a contiguous slice of this array.
 *
 * Capacity: `bitset_storage_capacity`
 * Size: `bitset_storage_size`
 *
 * 0x603718
 */
static int* bitset_storage;

/**
 * Total allocated capacity of `bitset_storage` in number of ints.
 *
 * 0x60371C
 */
static int bitset_storage_capacity;

/**
 * Split-mask table used to count set bits in the first N bits of a 32-bit value
 * via two 16-bit lookups into `popcount_table`.
 *
 * Size: `33`
 *
 * 0x603720
 */
static PopCountMask* popcount_masks;

/**
 * Total number of bitset ever created.
 *
 * 0x603724
 */
static int bitset_count;

/**
 * Flag indicating whether the bitset pool has been initialized.
 *
 * 0x603728
 */
static bool bitset_pool_initialized;

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

/**
 * Initializes the bitset pool and allocates all backing arrays.
 *
 * 0x4E59B0
 */
void bitset_pool_init()
{
    bitset_count = 0;
    bitset_descriptors_capacity = BITSET_DESCRIPTORS_INITIAL_CAPACITY;

    bitset_free_list_count = 0;
    bitset_free_list_capacity = BITSET_FREE_LIST_INITIAL_CAPACITY;

    bitset_storage_size = 0;
    bitset_storage_capacity = BITSET_STORAGE_INITIAL_CAPACITY;

    bitset_descriptors = (BitsetDescriptor*)MALLOC(sizeof(*bitset_descriptors) * bitset_descriptors_capacity);
    bitset_free_list = (int*)MALLOC(sizeof(*bitset_free_list) * bitset_free_list_capacity);
    bitset_storage = (int*)MALLOC(sizeof(*bitset_storage) * bitset_storage_capacity);

    popcount_tbl = (uint8_t*)MALLOC(POPCOUNT_TABLE_SIZE);
    popcount_masks = (PopCountMask*)MALLOC(sizeof(*popcount_masks) * POPCOUNT_MASKS_COUNT);
    popcount_table_init();
    popcount_masks_init();

    bitset_pool_initialized = true;
}

/**
 * Shuts down the bitset pool and frees all backing storage.
 *
 * 0x4E5A50
 */
void bitset_pool_exit()
{
    FREE(popcount_masks);
    FREE(popcount_tbl);
    FREE(bitset_storage);
    FREE(bitset_free_list);
    FREE(bitset_descriptors);

    bitset_pool_initialized = false;
}

/**
 * Allocates a new bitset from the pool and returns its ID.
 *
 * 0x4E5AA0
 */
int bitset_alloc()
{
    int index;

    // Reuse a recycled slot from the free list if one is available.
    if (bitset_free_list_count != 0) {
        return bitset_free_list[--bitset_free_list_count];
    }

    // No recycled slots, grow the descriptor array on demand.
    if (bitset_count == bitset_descriptors_capacity) {
        bitset_descriptors_capacity += BITSET_DESCRIPTORS_GROWTH_STEP;
        bitset_descriptors = (BitsetDescriptor*)REALLOC(bitset_descriptors, sizeof(BitsetDescriptor) * bitset_descriptors_capacity);
    }

    // Assign the storage item offset for this new entry. The first entry starts
    // at 0, every subsequent entry is packed directly after the previous one.
    index = bitset_count++;
    if (index == 0) {
        bitset_descriptors[0].offset = index;
    } else {
        bitset_descriptors[index].offset = bitset_descriptors[index - 1].cnt + bitset_descriptors[index - 1].offset;
    }

    // Grow newly allocated bitset to the minimum size.
    bitset_descriptors[index].cnt = 0;
    bitset_grow(index, BITSET_MIN_STORAGE_ITEMS);

    return index;
}

/**
 * Returns a bitset to the pool for future reuse.
 *
 * 0x4E5B40
 */
void bitset_free(int id)
{
    BitsetDescriptor* desc;
    int index;

    desc = &(bitset_descriptors[id]);

    // Shrink to the minimum size to reclaim excess storage items in
    // `bitset_storage`.
    if (desc->cnt != BITSET_MIN_STORAGE_ITEMS) {
        bitset_shrink(id, desc->cnt - BITSET_MIN_STORAGE_ITEMS);
    }

    // Clear all remaining bits.
    for (index = desc->offset; index < desc->cnt + desc->offset; index++) {
        bitset_storage[index] = 0;
    }

    // Push onto the free list, growing the list on demand.
    if (bitset_free_list_count == bitset_free_list_capacity) {
        bitset_free_list_capacity += BITSET_FREE_LIST_GROWTH_STEP;
        bitset_free_list = (int*)REALLOC(bitset_free_list, sizeof(int) * bitset_free_list_capacity);
    }

    bitset_free_list[bitset_free_list_count++] = id;
}

/**
 * Creates a copy the specified bitset.
 *
 * 0x4E5BF0
 */
int bitset_copy(int src)
{
    int dst;
    int delta;
    int index;

    dst = bitset_alloc();

    // Resize the new bitset to match the source's storage item count.
    delta = bitset_descriptors[src].cnt - bitset_descriptors[dst].cnt;
    if (delta != 0) {
        bitset_grow(dst, delta);
    }

    // Bulk-copy all storage items from src into dst.
    for (index = 0; index < bitset_descriptors[src].cnt; index++) {
        bitset_storage[bitset_descriptors[dst].offset + index] = bitset_storage[bitset_descriptors[src].offset + index];
    }

    return dst;
}

/**
 * Sets or clears a single bit in a bitset.
 *
 * 0x4E5C60
 */
void bitset_set(int id, int pos, bool val)
{
    int idx;
    int mask;

    idx = bitset_index_of(pos);
    if (idx >= bitset_descriptors[id].cnt) {
        if (!val) {
            // Clearing an out-of-range bit is a no-op, those bits are always 0.
            return;
        }

        // Grow the bitset to include the storage item that contains `pos`.
        bitset_grow(id, idx - bitset_descriptors[id].cnt + 1);
    }

    mask = bitset_mask_of(pos);
    if (val) {
        bitset_storage[idx + bitset_descriptors[id].offset] |= mask;
    } else {
        bitset_storage[idx + bitset_descriptors[id].offset] &= ~mask;
    }
}

/**
 * Tests whether the bit at a specified position in a bitset is set to 1.
 *
 * 0x4E5CE0
 */
bool bitset_test(int id, int pos)
{
    int idx;
    int mask;

    idx = bitset_index_of(pos);
    if (idx > bitset_descriptors[id].cnt - 1) {
        return 0;
    }

    mask = bitset_mask_of(pos);
    return (mask & bitset_storage[idx + bitset_descriptors[id].offset]) != 0;
}

/**
 * Returns the number of set bits strictly before position `pos` in the
 * bitset with ID `id`.
 *
 * 0x4E5D30
 */
int bitset_rank(int id, int pos)
{
    int cnt = 0;
    int base_item_offset;
    int idx;
    int stop_item_offset;

    base_item_offset = bitset_descriptors[id].offset;
    idx = bitset_index_of(pos);
    stop_item_offset = base_item_offset + idx;
    if (idx < bitset_descriptors[id].cnt) {
        // Count only the bits in the target storage item that precede `pos`.
        cnt += bitset_count_bits(bitset_storage[stop_item_offset], pos % BITS_PER_STORAGE_ITEM);
    } else {
        // `pos` is past the end, count up to the last allocated storage item
        // instead.
        stop_item_offset = base_item_offset + bitset_descriptors[id].cnt;
    }

    // Count all set bits in every complete storage item that comes before the
    // target.
    while (base_item_offset < stop_item_offset) {
        cnt += bitset_count_bits(bitset_storage[base_item_offset++], BITS_PER_STORAGE_ITEM);
    }

    return cnt;
}

/**
 * Iterates over all set bits in a bitset, invoking `callback` for each.
 *
 * 0x4E5DB0
 */
bool bitset_enumerate(int id, bool (*callback)(int))
{
    int start;
    int end;
    int pos;
    int idx;
    int bit;
    uint32_t flags;

    start = bitset_descriptors[id].offset;
    end = bitset_descriptors[id].cnt + start;

    pos = 0;
    for (idx = start; idx < end; idx++) {
        flags = 1;
        for (bit = 0; bit < 32; bit++) {
            if ((flags & bitset_storage[idx]) != 0) {
                if (!callback(pos)) {
                    return false;
                }
            }
            flags <<= 1;
            pos++;
        }
    }

    return true;
}

/**
 * Serializes the bitset with ID `id` to a `TigFile` stream.
 *
 * Returns `true` on success, `false` on write error.
 *
 * 0x4E5E20
 */
bool bitset_write_file(int id, TigFile* stream)
{
    if (tig_file_fwrite(&(bitset_descriptors[id].cnt), sizeof(int), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(bitset_storage[bitset_descriptors[id].offset]), sizeof(int) * bitset_descriptors[id].cnt, 1, stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Deserializes a bitset from a `TigFile` stream.
 *
 * Returns `true` on success, `false` on read error.
 *
 * 0x4E5E80
 */
bool bitset_read_file(int* out_id, TigFile* stream)
{
    int id;
    int cnt;

    id = bitset_alloc();

    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Grow if the stored size exceeds the freshly allocated minimum.
    if (cnt != bitset_descriptors[id].cnt && cnt - bitset_descriptors[id].cnt > 0) {
        bitset_grow(id, cnt - bitset_descriptors[id].cnt);
    }

    // Read exactly `cnt` storage items.
    if (tig_file_fread(&(bitset_storage[bitset_descriptors[id].offset]), sizeof(*bitset_storage) * cnt, 1, stream) != 1) {
        return false;
    }

    *out_id = id;

    return true;
}

/**
 * Returns the number of bytes required to serialize the specified bitset.
 *
 * 0x4E5F10
 */
int bitset_serialized_size(int id)
{
    return sizeof(int) + sizeof(int) * (bitset_descriptors[id].cnt);
}

/**
 * Serializes a bitset to a memory buffer.
 *
 * 0x4E5F30
 */
void bitset_write_mem(int id, uint8_t* dst)
{
    int offset;
    int cnt;

    offset = bitset_descriptors[id].offset;
    cnt = bitset_descriptors[id].cnt;

    memcpy(dst, &cnt, sizeof(cnt));
    memcpy(dst + sizeof(cnt), &(bitset_storage[offset]), sizeof(*bitset_storage) * cnt);
}

/**
 * Deserializes a bitset from a memory buffer.
 *
 * 0x4E5F70
 */
void bitset_read_mem(int* id_ptr, uint8_t** data)
{
    int id;
    int cnt;

    id = bitset_alloc();

    sub_4E4C50(&cnt, sizeof(cnt), data);

    if (cnt != bitset_descriptors[id].cnt) {
        bitset_grow(id, cnt - bitset_descriptors[id].cnt);
    }

    sub_4E4C50(&(bitset_storage[bitset_descriptors[id].offset]), sizeof(*bitset_storage) * cnt, data);

    *id_ptr = id;
}

/**
 * Counts the number of set bits in the first `n` bits of a 32-bit value.
 *
 * Uses the precomputed `popcount_table` (one byte per 16-bit value) together
 * with `popcount_masks[n]` to isolate exactly the `n` lowest bits, split across
 * both 16-bit halves.
 *
 * 0x4E5FE0
 */
int bitset_count_bits(int value, int n)
{
    return popcount_tbl[popcount_masks[n].lo_mask & (value & 0xFFFF)]
        + popcount_tbl[popcount_masks[n].hi_mask & ((value >> 16) & 0xFFFF)];
}

/**
 * Grows bitset `id` by `cnt` additional 32-bit storage items.
 *
 * All added storage items are initialized to 0. Because every bitset occupies a
 * contiguous slice of the shared `bitset_storage` array, inserting new items
 * into one bitset requires shifting the data of all higher-indexed bitsets to
 * the right and updating their `offset` fields accordingly.
 *
 * 0x4E6040
 */
void bitset_grow(int id, int cnt)
{
    int* base;
    int idx;

    if (bitset_storage_size + cnt > bitset_storage_capacity) {
        bitset_storage_capacity += ((bitset_storage_size + cnt - bitset_storage_capacity - 1) / BITSET_STORAGE_GROWTH_STEP + 1) * BITSET_STORAGE_GROWTH_STEP;
        bitset_storage = REALLOC(bitset_storage, sizeof(*bitset_storage) * bitset_storage_capacity);
    }

    // Shift the data of all bitsets that come after `id` to make room.
    if (id != bitset_count - 1) {
        base = &(bitset_storage[bitset_descriptors[id + 1].offset]);
        memmove(&(base[cnt]),
            base,
            sizeof(*base) * (bitset_storage_size - bitset_descriptors[id + 1].offset));

        // Adjust the stored offset of every subsequent bitset.
        bitset_shift_offsets(id + 1, bitset_count - 1, cnt);
    }

    // Update the descriptor and usage counter.
    bitset_descriptors[id].cnt += cnt;
    bitset_storage_size += cnt;

    // Zero-initialize the newly appended items.
    for (idx = bitset_descriptors[id].cnt + bitset_descriptors[id].offset - cnt; idx < bitset_descriptors[id].cnt + bitset_descriptors[id].offset; idx++) {
        bitset_storage[idx] = 0;
    }
}

/**
 * Shrinks bitset `id` by removing `cnt` 32-bit storage items from its tail.
 *
 * Shifts the data of all higher-indexed bitsets to the left and updates their
 * `offset` fields accordingly.
 *
 * 0x4E6130
 */
void bitset_shrink(int id, int cnt)
{
    int* base;

    // Close the gap by shifting higher bitset data to the left.
    if (id != bitset_count - 1) {
        base = &(bitset_storage[bitset_descriptors[id + 1].offset]);
        memmove(&(base[-cnt]),
            base,
            sizeof(*base) * (bitset_storage_size - bitset_descriptors[id + 1].offset));

        // Adjust the stored offset of every subsequent bitset.
        bitset_shift_offsets(id + 1, bitset_count - 1, -cnt);
    }

    bitset_descriptors[id].cnt -= cnt;
    bitset_storage_size -= cnt;
}

/**
 * Adds `delta` to the `offset` of every bitset within the specified range.
 *
 * Called by `bitset_grow` and `bitset_shrink` after inserting or removing
 * indices so that all descriptor offsets stay consistent with the flat array.
 *
 * 0x4E61B0
 */
void bitset_shift_offsets(int start, int end, int delta)
{
    int index;

    for (index = start; index <= end; index++) {
        bitset_descriptors[index].offset += delta;
    }
}

/**
 * Returns the index of a given bit position in contiguous data array.
 *
 * 0x4E61E0
 */
int bitset_index_of(int pos)
{
    return pos / 32;
}

/**
 * Returns the single-bit mask for a given bit position.
 *
 * 0x4E61F0
 */
int bitset_mask_of(int pos)
{
    return 1 << (pos % 32);
}

/**
 * Fills `popcount_tbl` so that `popcount_tbl[n]` gives the number of set
 * bits in the 16-bit value `n`.
 *
 * 0x4E6210
 */
void popcount_table_init()
{
    int idx;
    uint8_t cnt;
    uint16_t mask;
    int bit;

    for (idx = 0; idx < POPCOUNT_TABLE_SIZE; idx++) {
        cnt = 0;
        mask = 1;
        for (bit = 16; bit != 0; bit--) {
            if ((mask & idx) != 0) {
                cnt++;
            }
            mask <<= 1;
        }
        popcount_tbl[idx] = cnt;
    }
}

/**
 * Fills `popcount_masks` so that `popcount_masks[n]` isolates exactly the
 * first `n `bits of a 32-bit value, split across the two 16-bit halves.
 *
 * 0x4E6240
 */
void popcount_masks_init()
{
    int idx;
    uint16_t step = 1;
    uint16_t accum = 0;

    // Entry 0: no bits selected in either half.
    popcount_masks[0].lo_mask = 0;
    popcount_masks[0].hi_mask = 0;

    for (idx = 1; idx <= 16; idx++) {
        accum += step;
        step <<= 1;

        // Entries 1–16: all idx target bits are within the low 16-bit half.
        popcount_masks[idx].lo_mask = accum;
        popcount_masks[idx].hi_mask = 0;

        // Entries 17–32: the low half is fully selected (0xFFFF). The high half
        // selects the remaining idx bits (mirroring the low-half pattern).
        popcount_masks[idx + 16].lo_mask = 0xFFFF;
        popcount_masks[idx + 16].hi_mask = accum;
    }
}
