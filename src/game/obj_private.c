#include "game/obj_private.h"

#include <stdio.h>

#include "game/map.h"
#include "game/obj_file.h"
#include "game/object.h"
#include "game/sector.h"

/**
 * Initial allocation size for the `WriteBuffer`, and the granularity at which
 * it is grown when more bytes are appended.
 */
#define WRITE_BUFFER_INITIAL_SIZE 256
#define WRITE_BUFFER_GROWTH_STEP WRITE_BUFFER_INITIAL_SIZE

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
static void popcount_table_init(void);
static void popcount_masks_init(void);

/**
 * Flag indicating that obj data system has been initialized.
 *
 * 0x6036A8
 */
static bool obj_data_initialized;

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

/**
 * Initializes the obj data system.
 *
 * 0x4E3F80
 */
void obj_data_init(void)
{
    obj_data_initialized = true;
}

/**
 * Shuts down the obj data system.
 *
 * 0x4E3F90
 */
void obj_data_exit(void)
{
    obj_data_initialized = false;
}

/**
 * Resets an obj field to its zero/null state, freeing heap memory where the
 * field type requires it.
 *
 * Operates on the live field pointed to by `descr->ptr`. After this call the
 * field holds its "empty" representation: 0 for scalars, NULL for pointers,
 * and a deallocated `SizeableArray` for array types.
 *
 * 0x4E3FA0
 */
void obj_data_reset(ObjDataDescriptor* descr)
{
    switch (descr->type) {
    case OD_TYPE_INT32:
        *(int*)descr->ptr = 0;
        break;
    case OD_TYPE_INT64:
    case OD_TYPE_STRING:
    case OD_TYPE_HANDLE:
        // These types store a heap pointer, free it and nullify the pointer.
        if (*(void**)descr->ptr != NULL) {
            FREE(*(void**)descr->ptr);
            *(void**)descr->ptr = NULL;
        }
        break;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
        // Array types own a `SizeableArray` on the heap, deallocate.
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_deallocate((SizeableArray**)descr->ptr);
        }
        break;
    case OD_TYPE_PTR:
        // CE: Raw pointer field: zero without freeing (unowned reference).
        *(intptr_t*)descr->ptr = 0;
        break;
    case OD_TYPE_PTR_ARRAY:
        // CE: Raw pointer array: same behaviour as for other arrays.
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_deallocate((SizeableArray**)descr->ptr);
        }
        break;
    }
}

/**
 * Persists data by copying from transient storage data back to the live obj
 * field.
 *
 * For types that heap-allocate (INT64, STRING, HANDLE, arrays) memory is
 * allocated on demand and freed when the stored value is empty/null.
 *
 * 0x4E4000
 */
void obj_data_store(ObjDataDescriptor* descr)
{
    switch (descr->type) {
    case OD_TYPE_INT32:
        *(int*)descr->ptr = descr->storage.value;
        break;
    case OD_TYPE_INT64:
        if (descr->storage.value64 != 0) {
            // Allocate the backing int64 on demand if not yet present.
            if (*(int64_t**)descr->ptr == NULL) {
                *(int64_t**)descr->ptr = MALLOC(sizeof(int64_t));
            }
            **(int64_t**)descr->ptr = descr->storage.value64;
        } else {
            // Zero value: free the backing storage and nullify the pointer.
            if (*(int64_t**)descr->ptr != NULL) {
                FREE(*(int64_t**)descr->ptr);
                *(int64_t**)descr->ptr = NULL;
            }
        }
        break;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
        if (*(SizeableArray**)descr->ptr == NULL) {
            // Allocate array for storing 32-bit values.
            sa_allocate((SizeableArray**)descr->ptr, sizeof(int));
        }
        sa_set((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        break;
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
        if (*(SizeableArray**)descr->ptr == NULL) {
            // Allocate array for storing 64-bit values.
            sa_allocate((SizeableArray**)descr->ptr, sizeof(int64_t));
        }
        sa_set((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        break;
    case OD_TYPE_SCRIPT_ARRAY:
        if (*(SizeableArray**)descr->ptr == NULL) {
            // Allocate array for storing `Script` objects (script number,
            // flags, and counters).
            sa_allocate((SizeableArray**)descr->ptr, sizeof(Script));
        }
        sa_set((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        break;
    case OD_TYPE_QUEST_ARRAY:
        if (*(SizeableArray**)descr->ptr == NULL) {
            // Allocate array for storing `PcQuestState` objects (datetime and
            // state).
            sa_allocate((SizeableArray**)descr->ptr, sizeof(PcQuestState));
        }
        sa_set((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        break;
    case OD_TYPE_STRING:
        // Replace any existing string with a fresh duplicate of the stored one.
        if (*(char**)descr->ptr != NULL) {
            FREE(*(char**)descr->ptr);
        }
        *(char**)descr->ptr = STRDUP(descr->storage.str);
        break;
    case OD_TYPE_HANDLE:
        if (descr->storage.oid.type != OID_TYPE_NULL) {
            // Non-null handle: allocate `ObjectID` on demand and copy into it.
            if (*(ObjectID**)descr->ptr == NULL) {
                *(ObjectID**)descr->ptr = MALLOC(sizeof(ObjectID));
            }
            **(ObjectID**)descr->ptr = descr->storage.oid;
            break;
        } else {
            // Null handle: free any existing `ObjectID` and nullify the
            // pointer.
            if (*(ObjectID**)descr->ptr != NULL) {
                FREE(*(ObjectID**)descr->ptr);
                *(ObjectID**)descr->ptr = NULL;
            }
        }
        break;
    case OD_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)descr->ptr == NULL) {
            // Allocate array for storing `ObjectID` objects.
            sa_allocate((SizeableArray**)descr->ptr, sizeof(ObjectID));
        }
        sa_set((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        break;
    case OD_TYPE_PTR:
        *(intptr_t*)descr->ptr = descr->storage.ptr;
        break;
    case OD_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)descr->ptr == NULL) {
            // Allocate array for storing raw pointers (these are useful at
            // runtime in transient properties).
            sa_allocate((SizeableArray**)descr->ptr, sizeof(intptr_t));
        }
        sa_set((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        break;
    }
}

/**
 * Fetches the current value from the live obj field into the transient
 * storage.
 *
 * 0x4E4180
 */
void obj_data_fetch(ObjDataDescriptor* descr)
{
    switch (descr->type) {
    case OD_TYPE_INT32:
        descr->storage.value = *(int*)descr->ptr;
        break;
    case OD_TYPE_INT64:
        if (*(int64_t**)descr->ptr != NULL) {
            descr->storage.value64 = **(int64_t**)descr->ptr;
        } else {
            descr->storage.value64 = 0;
        }
        break;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_get((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        } else {
            descr->storage.value = 0;
        }
        break;
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_get((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        } else {
            descr->storage.value64 = 0;
        }
        break;
    case OD_TYPE_SCRIPT_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_get((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        } else {
            memset(&(descr->storage.scr), 0, sizeof(descr->storage.scr));
        }
        break;
    case OD_TYPE_QUEST_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_get((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        } else {
            memset(&(descr->storage.quest), 0, sizeof(descr->storage.quest));
        }
        break;
    case OD_TYPE_STRING:
        // Duplicate the string so the caller owns an independent copy.
        if (*(char**)descr->ptr != NULL) {
            descr->storage.str = STRDUP(*(char**)descr->ptr);
        } else {
            descr->storage.str = NULL;
        }
        break;
    case OD_TYPE_HANDLE:
        if (*(ObjectID**)descr->ptr != NULL) {
            descr->storage.oid = **(ObjectID**)descr->ptr;
        } else {
            descr->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    case OD_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_get((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        } else {
            descr->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    case OD_TYPE_PTR:
        descr->storage.ptr = *(intptr_t*)descr->ptr;
        break;
    case OD_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_get((SizeableArray**)descr->ptr, descr->idx, &(descr->storage));
        } else {
            descr->storage.ptr = 0;
        }
        break;
    }
}

/**
 * Performs a deep copy of the live obj field into a caller-supplied
 * destination buffer.
 *
 * `value` must point to storage large enough for the field's type.
 * Heap-allocated data is duplicated - the caller is responsible for freeing it.
 *
 * 0x4E4280
 */
void obj_data_copy(ObjDataDescriptor* descr, void* value)
{
    switch (descr->type) {
    case OD_TYPE_INT32:
        *(int*)value = *(int*)descr->ptr;
        break;
    case OD_TYPE_INT64:
        if (*(int64_t**)descr->ptr != NULL) {
            *(int64_t**)value = (int64_t*)MALLOC(sizeof(int64_t));
            **(int64_t**)value = **(int64_t**)descr->ptr;
        } else {
            *(int64_t**)value = NULL;
        }
        break;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_copy((SizeableArray**)value, (SizeableArray**)descr->ptr);
        } else {
            *(SizeableArray**)value = NULL;
        }
        break;
    case OD_TYPE_STRING:
        if (*(char**)descr->ptr != NULL) {
            *(char**)value = STRDUP(*(char**)descr->ptr);
        } else {
            *(char**)value = NULL;
        }
        break;
    case OD_TYPE_HANDLE:
        if (*(ObjectID**)descr->ptr != NULL) {
            *(ObjectID**)value = (ObjectID*)MALLOC(sizeof(ObjectID));
            **(ObjectID**)value = **(ObjectID**)descr->ptr;
        } else {
            *(ObjectID**)value = NULL;
        }
        break;
    case OD_TYPE_PTR:
    case OD_TYPE_PTR_ARRAY:
        assert(0);
    }
}

/**
 * Reads an obj field from a `TigFile` stream into the live field.
 *
 * Unlike `obj_data_read_file_fast`, this variant handles the case where the
 * field may already hold live heap data: existing allocations are freed before
 * new ones are created.
 *
 * Returns `true` on success, `false` otherwise.
 *
 * 0x4E4360
 */
bool obj_data_read_file_slow(ObjDataDescriptor* descr, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (descr->type) {
    case OD_TYPE_INT32:
        if (!objf_read(descr->ptr, sizeof(int), stream)) {
            return false;
        }
        return true;
    case OD_TYPE_INT64:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: free any existing allocation and null the pointer.
            if (*(int64_t**)descr->ptr != NULL) {
                FREE(*(int64_t**)descr->ptr);
                *(int64_t**)descr->ptr = NULL;
            }
            return true;
        }

        // Present: allocate if needed...
        if (*(int64_t**)descr->ptr == NULL) {
            *(int64_t**)descr->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }

        // ...and read the value.
        if (!objf_read(*(int64_t**)descr->ptr, sizeof(int64_t), stream)) {
            return false;
        }

        return true;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: free any existing allocation and null the pointer.
            *(SizeableArray**)descr->ptr = NULL;
            return true;
        }

        // Present: delegate deallocate/allocate/read dance to
        // `SerializeableArray`.
        if (!sa_read_file((SizeableArray**)descr->ptr, stream)) {
            return false;
        }

        return true;
    case OD_TYPE_STRING:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        // Always free any existing string before reading a new value.
        if (*(char**)descr->ptr != NULL) {
            FREE(*(char**)descr->ptr);
        }

        if (!presence) {
            // Not present: previous pointer is already freed, just nullify the
            // pointer.
            *(char**)descr->ptr = NULL;
            return true;
        }

        // Present: read string length...
        if (!objf_read(&size, sizeof(size), stream)) {
            return false;
        }

        // ...then allocate and read the string.
        *(char**)descr->ptr = (char*)MALLOC(size + 1);
        if (!objf_read(*(char**)descr->ptr, size + 1, stream)) {
            return false;
        }

        return true;
    case OD_TYPE_HANDLE:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: free any existing allocation and null the pointer.
            if (*(ObjectID**)descr->ptr != NULL) {
                FREE(*(ObjectID**)descr->ptr);
                *(ObjectID**)descr->ptr = NULL;
            }
            return true;
        }

        // Present: allocate if needed...
        if (*(ObjectID**)descr->ptr == NULL) {
            *(ObjectID**)descr->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }

        // ...and read the value.
        if (!objf_read(*(ObjectID**)descr->ptr, sizeof(ObjectID), stream)) {
            return false;
        }

        return true;
    case OD_TYPE_PTR:
    case OD_TYPE_PTR_ARRAY:
        // These fields should only be stored in transient obj storage and
        // should never be serialized.
        assert(0);
    default:
        return false;
    }
}

/**
 * Reads an obj field from a `TigFile` stream into a freshly zeroed field.
 *
 * This is a simpler, faster variant of `obj_data_read_file_slow` intended for
 * brand-new obj whose fields have never been initialised. It does NOT free or
 * check for existing heap allocations. All output pointers are assumed to be
 * NULL on entry.
 *
 * Returns `true` on success, `false` otherwise.
 *
 * 0x4E44F0
 */
bool obj_data_read_file_fast(ObjDataDescriptor* descr, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (descr->type) {
    case OD_TYPE_INT32:
        if (!objf_read(descr->ptr, sizeof(int), stream)) {
            return false;
        }
        return true;
    case OD_TYPE_INT64:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: nullify the pointer.
            *(int64_t**)descr->ptr = NULL;
            return true;
        }

        // Present: allocate and read the value.
        *(int64_t**)descr->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        if (!objf_read(*(int64_t**)descr->ptr, sizeof(int64_t), stream)) {
            return false;
        }

        return true;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: nullify the pointer.
            *(SizeableArray**)descr->ptr = NULL;
            return true;
        }

        // Present: allocate and read the value.
        if (!sa_read_no_dealloc((SizeableArray**)descr->ptr, stream)) {
            return false;
        }

        return true;
    case OD_TYPE_STRING:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: nullify the pointer.
            *(char**)descr->ptr = NULL;
            return true;
        }

        // Present: read string length...
        if (!objf_read(&size, sizeof(size), stream)) {
            return false;
        }

        // ...then allocate and read the string.
        *(char**)descr->ptr = (char*)MALLOC(size + 1);
        if (!objf_read(*(char**)descr->ptr, size + 1, stream)) {
            return false;
        }

        return true;
    case OD_TYPE_HANDLE:
        // Read presence byte to determine whether the value is stored.
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }

        if (!presence) {
            // Not present: nullify the pointer.
            *(ObjectID**)descr->ptr = NULL;
            return true;
        }

        // Present: allocate and read the value.
        *(ObjectID**)descr->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        if (!objf_read(*(ObjectID**)descr->ptr, sizeof(ObjectID), stream)) {
            return false;
        }

        return true;
    case OD_TYPE_PTR:
    case OD_TYPE_PTR_ARRAY:
        // These fields should only be stored in transient obj storage and
        // should never be serialized.
        assert(0);
    default:
        return false;
    }
}

/**
 * Reads an obj field from a memory buffer.
 *
 * Mirrors `obj_data_read_file_slow`, but reads from a raw byte stream rather
 * than a `TigFile`. The buffer pointer `*data` is advanced by the number of
 * bytes consumed. Existing heap allocations in the field are freed before new
 * ones are created.
 *
 * 0x4E4660
 */
void obj_data_read_mem(ObjDataDescriptor* descr, uint8_t** data)
{
    uint8_t presence;
    int size;

    switch (descr->type) {
    case OD_TYPE_INT32:
        mem_read_advance(descr->ptr, sizeof(int), data);
        return;
    case OD_TYPE_INT64:
        // Read presence byte to determine whether the value is stored.
        mem_read_advance(&presence, sizeof(presence), data);

        if (!presence) {
            // Not present: free any existing allocation and null the pointer.
            if (*(int64_t**)descr->ptr != NULL) {
                FREE(*(int64_t**)descr->ptr);
                *(int64_t**)descr->ptr = NULL;
            }
            return;
        }

        // Present: allocate if needed...
        if (*(int64_t**)descr->ptr == NULL) {
            *(int64_t**)descr->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }

        // ...and read the value.
        mem_read_advance(*(int64_t**)descr->ptr, sizeof(int64_t), data);
        return;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
        // Read presence byte to determine whether the value is stored.
        mem_read_advance(&presence, sizeof(presence), data);

        if (!presence) {
            // Not present: free any existing allocation and null the pointer.
            if (*(SizeableArray**)descr->ptr != NULL) {
                FREE(*(SizeableArray**)descr->ptr);
                *(SizeableArray**)descr->ptr = NULL;
            }
            return;
        }

        // Present: delegate deallocate/allocate/read dance to
        // `SerializeableArray`.
        sa_read_mem((SizeableArray**)descr->ptr, data);
        return;
    case OD_TYPE_STRING:
        // Read presence byte to determine whether the value is stored.
        mem_read_advance(&presence, sizeof(presence), data);

        // Always free any existing string before reading a new value.
        if (*(char**)descr->ptr != NULL) {
            FREE(*(char**)descr->ptr);
        }

        if (!presence) {
            // Not present: previous pointer is already freed, just nullify the
            // pointer.
            *(char**)descr->ptr = NULL;
            return;
        }

        // Present: read string length...
        mem_read_advance(&size, sizeof(size), data);

        // ...then allocate and read the string.
        *(char**)descr->ptr = (char*)MALLOC(size + 1);
        mem_read_advance(*(char**)descr->ptr, size + 1, data);
        return;
    case OD_TYPE_HANDLE:
        // Read presence byte to determine whether the value is stored.
        mem_read_advance(&presence, sizeof(presence), data);

        if (!presence) {
            // Not present: free any existing allocation and null the pointer.
            if (*(ObjectID**)descr->ptr != NULL) {
                FREE(*(ObjectID**)descr->ptr);
                *(ObjectID**)descr->ptr = NULL;
            }
            return;
        }

        // Present: allocate if needed...
        if (*(ObjectID**)descr->ptr == NULL) {
            *(ObjectID**)descr->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }

        // ...and read the value.
        mem_read_advance(*(ObjectID**)descr->ptr, sizeof(ObjectID), data);
        return;
    case OD_TYPE_PTR:
    case OD_TYPE_PTR_ARRAY:
        // These fields should only be stored in transient obj storage and
        // should never be serialized.
        assert(0);
    }
}

/**
 * Writes an obj field to a `TigFile` stream.
 *
 * Returns `true` on success, `false` otherwise.
 *
 * 0x4E47E0
 */
bool obj_data_write_file(ObjDataDescriptor* descr, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (descr->type) {
    case OD_TYPE_INT32:
        if (!objf_write((int*)descr->ptr, sizeof(int), stream)) return false;
        return true;
    case OD_TYPE_INT64:
        if (*(int64_t**)descr->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!objf_write(*(int64_t**)descr->ptr, sizeof(int64_t), stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!sa_write_file((SizeableArray**)descr->ptr, stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case OD_TYPE_STRING:
        if (*(char**)descr->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            size = (int)strlen(*(char**)descr->ptr);
            if (!objf_write(&size, sizeof(size), stream)) return false;
            if (!objf_write(*(char**)descr->ptr, size + 1, stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case OD_TYPE_HANDLE:
        if (*(ObjectID**)descr->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!objf_write(*(ObjectID**)descr->ptr, sizeof(ObjectID), stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case OD_TYPE_PTR:
    case OD_TYPE_PTR_ARRAY:
        // These fields should only be stored in transient obj storage and
        // should never be serialized.
        assert(0);
    default:
        return false;
    }
}

// 0x4E4990
void obj_data_write_mem(ObjDataDescriptor* a1, WriteBuffer* wb)
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
    //         size = sa_total_byte_size(a1->d.a.sa_ptr);
    //         write_buffer_ensure_size(wb, size);
    //         sa_write_mem(a1->d.a.sa_ptr, wb->field_4);
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

/**
 * Erases the element at `descr->idx` from an array-type field.
 *
 * Has no effect for non-array types or when the array pointer is NULL.
 *
 * 0x4E4B70
 */
void obj_data_remove_idx(ObjDataDescriptor* descr)
{
    switch (descr->type) {
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
    case OD_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            sa_remove((SizeableArray**)descr->ptr, descr->idx);
        }
        break;
    }
}

/**
 * Returns the number of elements in an array-type field.
 *
 * Returns `0` for non-array types or when the array pointer is NULL.
 *
 * 0x4E4BA0
 */
int obj_data_count_idx(ObjDataDescriptor* descr)
{
    switch (descr->type) {
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
    case OD_TYPE_SCRIPT_ARRAY:
    case OD_TYPE_QUEST_ARRAY:
    case OD_TYPE_HANDLE_ARRAY:
    case OD_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)descr->ptr != NULL) {
            return sa_count((SizeableArray**)descr->ptr);
        }
        break;
    }

    return 0;
}

/**
 * Initializes a `WriteBuffer`.
 *
 * The caller is responsible to free `base` when done.
 *
 * 0x4E4BD0
 */
void write_buffer_init(WriteBuffer* wb)
{
    wb->base = (uint8_t*)MALLOC(WRITE_BUFFER_INITIAL_SIZE);
    wb->curr = wb->base;
    wb->size = WRITE_BUFFER_INITIAL_SIZE;
    wb->remaining = wb->size;
}

/**
 * Appends `size` bytes from `data` to a `WriteBuffer`, growing it if needed.
 *
 * 0x4E4C00
 */
void write_buffer_append(const void* data, int size, WriteBuffer* wb)
{
    write_buffer_ensure_size(wb, size);
    memcpy(wb->curr, data, size);
    wb->curr += size;
    wb->remaining -= size;
}

/**
 * Copies `size` bytes from `*data` into `buffer` and advances `*data` by
 * `size` bytes.
 *
 * Acts as a streaming read from a raw byte buffer. This function is used to
 * read from an arbitrary byte buffer without tracking the read offset manually.
 * It may overflow, though, and there is no protection against it.
 *
 * 0x4E4C50
 */
void mem_read_advance(void* buffer, int size, uint8_t** data)
{
    memcpy(buffer, *data, size);
    (*data) += size;
}

/**
 * Ensures that a `WriteBuffer` has at least `size` bytes of remaining space.
 *
 * If the current remaining capacity is insufficient, the buffer is grown in
 * multiples of `WRITE_BUFFER_GROWTH_STEP` bytes. After reallocation, base may
 * point to a new address.
 *
 * 0x4E4C80
 */
void write_buffer_ensure_size(WriteBuffer* wb, int size)
{
    int extra_size;
    int growth;

    extra_size = size - wb->remaining;
    if (extra_size > 0) {
        // Round the growth amount up to the nearest `WRITE_BUFFER_GROWTH_STEP`.
        growth = (extra_size / WRITE_BUFFER_GROWTH_STEP + 1) * WRITE_BUFFER_GROWTH_STEP;
        wb->size += growth;
        wb->base = (uint8_t*)REALLOC(wb->base, wb->size);

        // Recalculate `curr` relative to the possibly relocated `base`.
        wb->curr = wb->base + wb->size - wb->remaining - growth;
        wb->remaining += growth;
    }
}

/**
 * Initializes the bitset pool and allocates all backing arrays.
 *
 * 0x4E59B0
 */
void bitset_pool_init(void)
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
void bitset_pool_exit(void)
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
int bitset_alloc(void)
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

    mem_read_advance(&cnt, sizeof(cnt), data);

    if (cnt != bitset_descriptors[id].cnt) {
        bitset_grow(id, cnt - bitset_descriptors[id].cnt);
    }

    mem_read_advance(&(bitset_storage[bitset_descriptors[id].offset]), sizeof(*bitset_storage) * cnt, data);

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
    return 1u << (pos % 32);
}

/**
 * Fills `popcount_tbl` so that `popcount_tbl[n]` gives the number of set
 * bits in the 16-bit value `n`.
 *
 * 0x4E6210
 */
void popcount_table_init(void)
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
void popcount_masks_init(void)
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
