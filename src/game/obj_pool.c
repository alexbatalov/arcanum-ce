#include "game/obj_pool.h"

#include "game/map.h"
#include "game/object.h"

/**
 * Hard cap on the total number of objects the pool can ever hold.
 *
 * The pool is divided into fixed-size buckets of `OBJ_POOL_BUCKET_SIZE` entries
 * each. New buckets are allocated on demand as the pool grows toward this
 * limit. Reaching this limit causes an immediate quit, as this likely means
 * a bug in implementation or a memory leak.
 */
#define OBJ_POOL_CAP 0x200000

/**
 * Number of object entries per bucket.
 *
 * Buckets are the unit of pool growth. Each bucket is a single contiguous heap
 * allocation of `OBJ_POOL_BUCKET_SIZE x obj_pool_element_byte_size` bytes.
 */
#define OBJ_POOL_BUCKET_SIZE 0x2000

/**
 * Maximum number of bucket pointers the bucket array can hold.
 */
#define OBJ_POOL_BUCKETS_CAPACITY (OBJ_POOL_CAP / OBJ_POOL_BUCKET_SIZE)

/**
 * Initial capacity for the free indices array, and the granularity at
 * which it is grown when more objects are freed.
 */
#define OBJ_POOL_FREED_INDEXES_INITIAL_CAPACITY 4096
#define OBJ_POOL_FREED_INDEXES_GROW_STEP 2048

/**
 * Initial capacity for the permanent OID table, and the granularity at which
 * it is grown when more entries are added.
 */
#define OBJ_POOL_PERM_OID_TABLE_INITIAL_CAPACITY 1024
#define OBJ_POOL_PERM_OID_TABLE_GROW_STEP 512

/**
 * Object handle bit-field layout.
 *
 * A 64-bit handle is composed of three fields packed into specific bit ranges:
 *   - index (bits 29-63): index into obj pool storage
 *   - seq (bits 3-25): generation sequence number
 *   - tag (bits 0-2): handle identifier
 *
 * The marker tag in the low 3 bits lets callers cheaply distinguish handles
 * from other integer values.
 *
 * The sequence number detects stale handles: once a slot is freed its sequence
 * is reset to 0, so any previously issued handle whose sequence no longer
 * matches the stored one is considered invalid (ABA-detection).
 *
 * See:
 *  - https://en.wikipedia.org/wiki/ABA_problem
 */
#define HANDLE_INDEX_SHIFT 29

#define HANDLE_SEQ_SHIFT 3
#define HANDLE_SEQ_MAX 0x7FFFFF

#define HANDLE_MARKER_MASK 0x7
#define HANDLE_MARKER_VALUE 2

/**
 * Per-entry status flags stored in `ObjPoolEntryHeader::status`.
 *
 *  - `STATUS_HANDLE`: slot is live and holds a valid object.
 *  - `STATUS_RELEASED`: slot has been freed and its index is on the freed-index
 *  stack awaiting reuse.
 */
#define STATUS_HANDLE 'H'
#define STATUS_RELEASED 'P'

/**
 * Header prepended to every pool entry in the storage.
 *
 * Laid out as a single 32-bit value:
 *  - seq (bits 8-31) - generation sequence number (24 bits)
 *  - status (bits 0-7) - `STATUS_HANDLE` or `STATUS_RELEASED`
 *
 * The header immediately precedes the user-defined object data within each
 * element. Together they form one pool element of `obj_pool_element_byte_size`
 * bytes.
 */
typedef struct ObjPoolEntryHeader {
    int status : 8;
    int seq : 24;
} ObjPoolEntryHeader;

/**
 * One entry in the permanent OID lookup table.
 *
 * Maps a persistent `ObjectID` to the live pool handle of the in-memory
 * object it corresponds to. The table is kept sorted by oid to allow binary
 * search.
 */
typedef struct PermOidLookupEntry {
    /* 0000 */ ObjectID oid;
    /* 0018 */ int64_t obj;
} PermOidLookupEntry;

static int acquire_index(void);
static void release_index(int index);
static bool grow_pool(void);
static void recycle_index(int index);
static bool find_perm_oid(ObjectID oid, int* index_ptr);
static int64_t make_handle(int index, int seq);
static int index_from_handle(int64_t obj);
static ObjPoolEntryHeader* element_hdr_at_index(int index);
static void* element_data_at_index(int index);
static void sequence_to_hdr(ObjPoolEntryHeader* hdr, int seq);
static int sequence_from_hdr(ObjPoolEntryHeader* hdr);

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

/**
 * Byte size of `ObjPoolEntryHeader`.
 *
 * CE: Due to pointer math involved, this value needs to be properly aligned so
 * that the object data that follows immediately after header the is also
 * properly aligned.
 *
 * 0x5B9258
 */
static int obj_pool_entry_header_byte_size = ALIGN(sizeof(ObjPoolEntryHeader), sizeof(void*));

/**
 * Monotonically increasing sequence counter used when minting new handles.
 *
 * Seeded from wall-clock time at init so that handles issued across separate
 * game sessions are unlikely to collide. Wraps from `HANDLE_SEQ_MAX` back to
 * `1`. `0` is reserved as the "empty slot" sentinel.
 *
 * 0x6036B0
 */
static int obj_pool_next_seq;

/**
 * When true the pool is operating in editor mode.
 *
 * In editor mode the permanent OID spatial fallback lookup in
 * `obj_pool_perm_lookup` is disabled, because the map state may not be fully
 * loaded.
 *
 * 0x6036B4
 */
static bool obj_pool_editor;

/**
 * Sorted array of oid-to-obj pairs for permanent object lookups.
 *
 * Capacity: `obj_pool_perm_oid_table_capacity`
 * Size: `obj_pool_perm_oid_table_size`
 *
 * 0x6036B8
 */
static PermOidLookupEntry* obj_pool_perm_oid_table;

/**
 * Pointer to the array of bucket pointers.
 *
 * The `obj_pool_buckets[n]` points to the raw byte storage for bucket `n`. Each
 * bucket holds `OBJ_POOL_BUCKET_SIZE` elements of `obj_pool_element_byte_size`
 * bytes each.
 *
 * Capacity: `OBJ_POOL_BUCKETS_CAPACITY`
 * Size: `obj_pool_num_buckets`
 *
 * 0x6036BC
 */
static uint8_t** obj_pool_buckets;

/**
 * Capacity of the `obj_pool_perm_oid_table` array.
 *
 * 0x6036C0
 */
static int obj_pool_perm_oid_table_capacity;

/**
 * Number of live (`STATUS_HANDLE`) objects currently in the pool.
 *
 * 0x6036C4
 */
static int obj_pool_num_objects;

/**
 * Maximum number of entries the pool can hold without allocating a new bucket.
 *
 * Always a multiple of `OBJ_POOL_BUCKET_SIZE`. Grows by one bucket each call
 * to `grow_pool`.
 *
 * 0x6036C8
 */
static int obj_pool_capacity;

/**
 * Number of buckets currently allocated.
 *
 * 0x6036CC
 */
static int obj_pool_num_buckets;

/**
 * Number of elements in `obj_pool_freed_indexes` array.
 *
 * 0x6036D0
 */
static int obj_pool_freed_indexes_size;

/**
 * Byte size of one bucket allocation.
 *
 * Precomputed as `obj_pool_element_byte_size * OBJ_POOL_BUCKET_SIZE` so that
 * `grow_pool` avoids a multiplication each time.
 *
 * 0x6036D4
 */
static int obj_pool_bucket_byte_size;

/**
 * Stack of freed pool indices available for reuse.
 *
 * When an object is deallocated its index is pushed here. `acquire_index`
 * pops from this stack before falling back to appending to the live range,
 * keeping the pool compact.
 *
 * Capacity: `obj_pool_freed_indexes_capacity`
 * Size: `obj_pool_freed_indexes_size`
 *
 * 0x6036D8
 */
static int* obj_pool_freed_indexes;

/**
 * Number of elements in `obj_pool_perm_oid_table` array.
 *
 * 0x6036DC
 */
static int obj_pool_perm_oid_table_size;

/**
 * Capacity of the `obj_pool_freed_indexes` array.
 *
 * 0x6036E0
 */
static int obj_pool_freed_indexes_capacity;

/**
 * Byte size of one pool element (header + user data).
 *
 * Precomputed as `obj_pool_entry_header_byte_size` + object size so that
 * `element_hdr_at_index` and `element_data_at_index` avoid per-call
 * arithmetic.
 *
 * 0x6036E4
 */
static int obj_pool_element_byte_size;

/**
 * Object count threshold at which `grow_pool` starts emitting warnings.
 *
 * When the pool exceeds this size the game prints a warning each time a new
 * bucket is allocated so developers are alerted before the hard cap is reached.
 *
 * 0x6036E8
 */
static int obj_pool_warn;

/**
 * Pending specific handle requested via obj_handle_request().
 *
 * When non-null, `obj_pool_allocate` will honour this exact handle value
 * instead of minting a fresh one.
 *
 * 0x6036F0
 */
static int64_t obj_handle_requested;

/**
 * Flag indicating that obj pool system was initialized.
 *
 * 0x6036F8
 */
static bool obj_pool_initialized;

/**
 * Initializes the obj pool system.
 *
 * 0x4E4CD0
 */
void obj_pool_init(int size, bool editor)
{
    obj_pool_buckets = (unsigned char**)CALLOC(256, sizeof(*obj_pool_buckets));
    obj_pool_editor = editor;

    // Warn once the pool grows past 25% of the hard cap.
    obj_pool_warn = OBJ_POOL_CAP / 4;

    obj_pool_capacity = OBJ_POOL_BUCKET_SIZE;

    // Seed the sequence counter from wall-clock time, clamped to the lower
    // half of the range and made odd (0 is the released-slot sentinel, so
    // it must never be used as an initial sequence).
    obj_pool_next_seq = ((int)time(0) & (HANDLE_SEQ_MAX / 2)) * 2 + 1;

    // Precalculate element and bucket size.
    obj_pool_element_byte_size = obj_pool_entry_header_byte_size + size;
    obj_pool_bucket_byte_size = obj_pool_element_byte_size * OBJ_POOL_BUCKET_SIZE;

    // Allocate the first bucket immediately so the pool is usable right away.
    obj_pool_buckets[0] = (unsigned char*)MALLOC(obj_pool_bucket_byte_size);
    obj_pool_num_buckets = 1;
    obj_pool_num_objects = 0;

    // Allocate stack of free indices.
    obj_pool_freed_indexes_capacity = OBJ_POOL_FREED_INDEXES_INITIAL_CAPACITY;
    obj_pool_freed_indexes = (int*)MALLOC(sizeof(*obj_pool_freed_indexes) * obj_pool_freed_indexes_capacity);
    obj_pool_freed_indexes_size = 0;

    // Allocate oid-to-handle lookup table.
    obj_pool_perm_oid_table_capacity = OBJ_POOL_PERM_OID_TABLE_INITIAL_CAPACITY;
    obj_pool_perm_oid_table_size = 0;
    obj_handle_requested = OBJ_HANDLE_NULL;
    obj_pool_perm_oid_table = (PermOidLookupEntry*)MALLOC(sizeof(*obj_pool_perm_oid_table) * obj_pool_perm_oid_table_capacity);

    obj_pool_initialized = true;
}

/**
 * Shuts down the obj pool system.
 *
 * 0x4E4DB0
 */
void obj_pool_exit(void)
{
    int index;
    ObjPoolEntryHeader* hdr;
    int seq;
    int64_t obj;

    if (obj_pool_num_objects != 0) {
        tig_debug_printf("Destroying %d leftover objects.\n", obj_pool_num_objects);
        for (index = obj_pool_freed_indexes_size + obj_pool_num_objects - 1; index >= 0; index--) {
            hdr = element_hdr_at_index(index);
            if (hdr->status == STATUS_HANDLE) {
                seq = sequence_from_hdr(hdr);
                obj = make_handle(index, seq);
                obj_deallocate(obj);
            }
        }
    }

    // Release all buckets.
    for (index = 0; index < obj_pool_num_buckets; index++) {
        FREE(obj_pool_buckets[index]);
    }

    // Release remaining storage.
    FREE(obj_pool_freed_indexes);
    FREE(obj_pool_perm_oid_table);
    FREE(obj_pool_buckets);

    obj_pool_initialized = false;
}

/**
 * Allocates one object from the pool and returns a handle to it.
 *
 * On success writes the new handle into `*obj_ptr` and returns a pointer to
 * the object's data storage.
 *
 * If `obj_handle_request` was called beforehand this function honours that
 * specific handle instead of minting a fresh one.
 *
 * 0x4E4E60
 */
void* obj_pool_allocate(int64_t* obj_ptr)
{
    int index;
    ObjPoolEntryHeader* hdr;
    void* object;

    if (obj_pool_num_objects == OBJ_POOL_CAP) {
        tig_message_post_quit(0);
        return NULL;
    }

    if (obj_handle_requested != OBJ_HANDLE_NULL) {
        // Restore a specific handle. The requested slot was already placed on
        // the freed-index stack by `obj_handle_request` so we decrement
        // `obj_pool_freed_indexes_size` here to account for that.
        *obj_ptr = obj_handle_requested;

        index = index_from_handle(*obj_ptr);

        hdr = element_hdr_at_index(index);
        hdr->status = STATUS_HANDLE;

        // Restore the sequence number that was encoded in the requested handle.
        sequence_to_hdr(hdr, (*obj_ptr >> HANDLE_SEQ_SHIFT) & HANDLE_SEQ_MAX);
        object = element_data_at_index(index);

        obj_pool_num_objects++;
        obj_pool_freed_indexes_size--;
        obj_handle_requested = OBJ_HANDLE_NULL;
    } else {
        // Normal allocation path: pick a free index and mint a fresh handle
        // using the current sequence counter.
        index = acquire_index();
        object = element_data_at_index(index);
        *obj_ptr = make_handle(index, obj_pool_next_seq);

        hdr = element_hdr_at_index(index);
        sequence_to_hdr(hdr, obj_pool_next_seq);

        // Advance the sequence counter, wrapping at `HANDLE_SEQ_MAX`. `0` is
        // skipped because it is the sentinel value for released slots.
        obj_pool_next_seq++;
        if (obj_pool_next_seq > HANDLE_SEQ_MAX) {
            obj_pool_next_seq = 1;
        }
    }

    return object;
}

/**
 * Returns a pointer to the object data for the given handle.
 *
 * This is a direct, unchecked lookup. The caller is responsible for ensuring
 * the handle is valid before calling.
 *
 * NOTE: In the original code (and this implementation) "locking" is a no-op,
 * there is no reference counting or pinning.
 *
 * 0x4E4F80
 */
void* obj_pool_lock(int64_t obj)
{
    return element_data_at_index(index_from_handle(obj));
}

/**
 * Releases a previously locked object pointer.
 *
 * NOTE: This is a no-op. See `obj_pool_lock` for explanation.
 *
 * 0x4E4FA0
 */
void obj_pool_unlock(int64_t obj)
{
    (void)obj;
}

/**
 * Returns the pool slot used by `obj` to the free list.
 *
 * The entry's sequence is zeroed so that any outstanding copies of the old
 * handle will fail the sequence check in `obj_handle_is_valid`.
 *
 * 0x4E4FB0
 */
void obj_pool_deallocate(int64_t obj)
{
    release_index(index_from_handle(obj));
}

/**
 * Inserts or updates the oid-to-obj pair in the permanent OID lookup table.
 *
 * If `oid` is already present its handle is updated in place. Otherwise a new
 * entry is inserted at the position determined by `find_perm_oid` so the table
 * stays.
 *
 * 0x4E4FD0
 */
void obj_pool_perm_oid_set(ObjectID oid, int64_t obj)
{
    int index;

    if (find_perm_oid(oid, &index)) {
        // Entry already exists - just update the handle.
        obj_pool_perm_oid_table[index].obj = obj;
        return;
    }

    // Grow the table if it is at capacity.
    if (obj_pool_perm_oid_table_size == obj_pool_perm_oid_table_capacity) {
        obj_pool_perm_oid_table_capacity += OBJ_POOL_PERM_OID_TABLE_GROW_STEP;
        if (obj_pool_perm_oid_table_capacity > OBJ_POOL_CAP) {
            return;
        }

        obj_pool_perm_oid_table = (PermOidLookupEntry*)REALLOC(obj_pool_perm_oid_table, sizeof(*obj_pool_perm_oid_table) * obj_pool_perm_oid_table_capacity);
    }

    // Shift entries from `index` onward to make room, preserving sort order.
    if (index != obj_pool_perm_oid_table_size) {
        memmove(&(obj_pool_perm_oid_table[index + 1]),
            &(obj_pool_perm_oid_table[index]),
            sizeof(*obj_pool_perm_oid_table) * (obj_pool_perm_oid_table_size - index));
    }

    obj_pool_perm_oid_table[index].oid = oid;
    obj_pool_perm_oid_table[index].obj = obj;
    obj_pool_perm_oid_table_size++;
}

/**
 * Looks up the handle for the obj by its ID.
 *
 * First checks the in-memory OID table. On a cache miss, for positional IDs
 * (`OID_TYPE_P`) on the current map, performs a spatial search at the OID's
 * recorded location to find the object and caches the result.
 *
 * Returns `OBJ_HANDLE_NULL` if the object cannot be found.
 *
 * 0x4E50E0
 */
int64_t obj_pool_perm_lookup(ObjectID oid)
{
    int idx;
    ObjectList objects;
    ObjectNode* node;

    // Fast path: the OID is already cached and the handle is still valid.
    if (find_perm_oid(oid, &idx)) {
        if (obj_handle_is_valid(obj_pool_perm_oid_table[idx].obj)) {
            return obj_pool_perm_oid_table[idx].obj;
        }
    }

    // Spatial fallback is only supported for positional OIDs on the current
    // map.
    if (oid.type != OID_TYPE_P) {
        return OBJ_HANDLE_NULL;
    }

    // The editor may have a partially loaded map, so skip the fallback there.
    if (obj_pool_editor) {
        return OBJ_HANDLE_NULL;
    }

    // Make sure positional OID matches current map.
    if (map_current_map() != oid.d.p.map) {
        return OBJ_HANDLE_NULL;
    }

    // Retrieve all static objects at the stored tile location.
    object_list_location(oid.d.p.location, OBJ_TM_TRAP | OBJ_TM_WALL | OBJ_TM_PORTAL | OBJ_TM_SCENERY, &objects);

    if (objects.num_sectors != 1) {
        tig_debug_println("Warning: objp_perm_lookup found sectors != 1");
        object_list_destroy(&objects);
        return OBJ_HANDLE_NULL;
    }

    // Find the specific object by its temporary ID within the tile.
    node = objects.head;
    while (node != NULL) {
        if (obj_field_int32_get(node->obj, OBJ_F_TEMP_ID) == oid.d.p.temp_id) {
            break;
        }
        node = node->next;
    }

    if (node == NULL) {
        // Object not found.
        object_list_destroy(&objects);
        return OBJ_HANDLE_NULL;
    }

    // Cache the result so subsequent lookups hit the fast path.
    obj_pool_perm_oid_set(oid, node->obj);

    object_list_destroy(&objects);

    // Sanity check - it has to work.
    if (!find_perm_oid(oid, &idx)) {
        tig_debug_println("Error: objp_perm_lookup can't find handle just added with positional id.");
        return OBJ_HANDLE_NULL;
    }

    return obj_pool_perm_oid_table[idx].obj;
}

/**
 * Reverse-looks up the `ObjectID` registered for a given handle.
 *
 * Performs a linear scan of the permanent OID table. Returns an `ObjectID` with
 * type `OID_TYPE_NULL` if the handle is not found.
 *
 * 0x4E5280
 */
ObjectID obj_pool_perm_reverse_lookup(int64_t obj)
{
    ObjectID oid;
    int index;

    for (index = 0; index < obj_pool_perm_oid_table_size; index++) {
        if (obj_pool_perm_oid_table[index].obj == obj) {
            return obj_pool_perm_oid_table[index].oid;
        }
    }

    oid.type = OID_TYPE_NULL;

    return oid;
}

/**
 * Removes the entry for `oid` from the permanent OID lookup table.
 *
 * NOTE: This is a pure guess based on the calling code.
 *
 * 0x4E52F0
 */
void obj_pool_perm_oid_remove(ObjectID oid)
{
    (void)oid;
}

/**
 * Clears permanent OID lookup table, retaining only entries whose OID type is
 * `OID_TYPE_A`.
 *
 * 0x4E5300
 */
void obj_pool_perm_clear(void)
{
    PermOidLookupEntry* scratch;
    int cnt;
    int read_idx;
    int write_idx;

    cnt = 0;
    scratch = MALLOC(sizeof(*scratch) * obj_pool_perm_oid_table_size);

    // Walk backward through the table, collecting `OID_TYPE_A` entries into the
    // tail of scratch (also backward) so their original order is preserved.
    read_idx = obj_pool_perm_oid_table_size - 1;
    write_idx = read_idx;
    while (read_idx >= 0) {
        if (obj_pool_perm_oid_table[read_idx].oid.type == OID_TYPE_A) {
            scratch[write_idx] = obj_pool_perm_oid_table[read_idx];
            write_idx--;
            cnt++;
        }
        read_idx--;
    }

    // Replace the table contents with only the retained entries.
    memset(obj_pool_perm_oid_table, 0, sizeof(*obj_pool_perm_oid_table) * obj_pool_perm_oid_table_size);
    memcpy(obj_pool_perm_oid_table, &(scratch[write_idx + 1]), sizeof(*obj_pool_perm_oid_table) * cnt);
    FREE(scratch);
    obj_pool_perm_oid_table_size = cnt;
}

/**
 * Starts an iteration over all live objects in the pool.
 *
 * Writes the first live object handle into `*obj_ptr` and the corresponding
 * pool index into `*iter_ptr`. Returns `true` if at least one live object
 * exists, `false` if the pool is empty.
 *
 * Use `obj_pool_walk_next` to continue the iteration.
 *
 * 0x4E53C0
 */
bool obj_pool_walk_first(int64_t* obj_ptr, int* iter_ptr)
{
    int iter;
    ObjPoolEntryHeader* hdr;

    if (obj_pool_num_objects == 0) {
        return false;
    }

    iter = obj_pool_freed_indexes_size + obj_pool_num_objects - 1;
    while (iter >= 0) {
        hdr = element_hdr_at_index(iter);
        if (hdr->status == STATUS_HANDLE) {
            *obj_ptr = make_handle(iter, sequence_from_hdr(hdr));
            *iter_ptr = iter;
            return true;
        }
        iter--;
    }

    *iter_ptr = iter;
    return false;
}

/**
 * Advances an in-progress pool iteration to the next live object.
 *
 * `*iter_ptr` must be the value written by the previous call to
 * `obj_pool_walk_first` or `obj_pool_walk_next`. Writes the next live handle
 * into `*obj_ptr` and updates `*iter_ptr`. Returns `false` when iteration is
 * exhausted.
 *
 * 0x4E5420
 */
bool obj_pool_walk_next(int64_t* obj_ptr, int* iter_ptr)
{
    int iter;
    ObjPoolEntryHeader* hdr;

    iter = *iter_ptr - 1;
    while (iter >= 0) {
        hdr = element_hdr_at_index(iter);
        if (hdr->status == STATUS_HANDLE) {
            *obj_ptr = make_handle(iter, sequence_from_hdr(hdr));
            *iter_ptr = iter;
            return true;
        }
        iter--;
    }

    *iter_ptr = iter;
    return false;
}

/**
 * Returns true if `obj` is a valid handle (`OBJ_HANDLE_NULL` is valid).
 *
 * 0x4E5470
 */
bool obj_handle_is_valid(int64_t obj)
{
    int index;
    ObjPoolEntryHeader* hdr;

    // Treat the null handle as a valid (but empty) value.
    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    // Quick sanity check: low 3 bits must match the handle marker.
    if ((obj & HANDLE_MARKER_MASK) != HANDLE_MARKER_VALUE) {
        return false;
    }

    // The index must fall within the currently used range of pool slots.
    index = (int)(obj >> HANDLE_INDEX_SHIFT);
    if (index >= obj_pool_num_objects + obj_pool_freed_indexes_size) {
        return false;
    }

    // Sequence mismatch means the slot was freed and possibly reused since this
    // handle was issued.
    hdr = element_hdr_at_index(index);
    if (sequence_from_hdr(hdr) != ((obj >> HANDLE_SEQ_SHIFT) & HANDLE_SEQ_MAX)) {
        return false;
    }

    // A `STATUS_RELEASED` entry with a matching sequence should not be
    // possible, but guard against it anyway.
    if (hdr->status != STATUS_HANDLE) {
        return false;
    }

    return true;
}

/**
 * Reserves a specific handle for the next call to `obj_pool_allocate`.
 *
 * 0x4E5530
 */
bool obj_handle_request(int64_t obj)
{
    int index;
    int num_indexes_to_recycle;
    int tmp_index;
    ObjPoolEntryHeader* hdr;

    // Quick sanity check: low 3 bits must match the handle marker.
    if ((obj & HANDLE_MARKER_MASK) != HANDLE_MARKER_VALUE) {
        tig_debug_println("Handle requested is not marked as a handle.");
        return false;
    }

    index = index_from_handle(obj);
    if (index < obj_pool_freed_indexes_size + obj_pool_num_objects) {
        // Index is already within the live pool range, it must be freed.
        hdr = element_hdr_at_index(index);
        if (hdr->status != STATUS_RELEASED) {
            tig_debug_println("Index of handle requested is not released.");
            return false;
        }

        obj_handle_requested = obj;

        return true;
    }

    if (index >= OBJ_POOL_CAP) {
        tig_debug_println("Index of handle requested is too large.");
        return false;
    }

    // The requested index lies beyond the current pool range. Calculate how
    // many intermediate slots need to be recycled as empty placeholders.
    num_indexes_to_recycle = index - obj_pool_freed_indexes_size - obj_pool_num_objects + 1;

    // Grow the pool until it is large enough to contain the requested index.
    while (index >= obj_pool_capacity) {
        if (!grow_pool()) {
            return false;
        }
    }

    // Add all intermediate slots (and the target slot itself) to the freed
    // indices stack marked as released, so they are treated as deallocated
    // placeholders.
    tmp_index = obj_pool_freed_indexes_size + obj_pool_num_objects;
    while (num_indexes_to_recycle != 0) {
        recycle_index(tmp_index);
        hdr = element_hdr_at_index(tmp_index);
        sequence_to_hdr(hdr, 0);
        hdr->status = STATUS_RELEASED;
        tmp_index++;
        num_indexes_to_recycle--;
    }

    // Keep a reference for subsequent `obj_pool_allocate` call.
    obj_handle_requested = obj;

    return true;
}

/**
 * Acquires the next available pool index for a new object.
 *
 * Prefers to reuse an index from the freed-index stack to keep the live range
 * compact. Falls back to appending to the end of the current live range,
 * growing the pool by one bucket if the range is full.
 *
 * 0x4E5640
 */
int acquire_index(void)
{
    int index;
    ObjPoolEntryHeader* hdr;

    if (obj_pool_freed_indexes_size > 0) {
        // Reuse a previously freed slot.
        index = obj_pool_freed_indexes[--obj_pool_freed_indexes_size];
        obj_pool_num_objects++;
        hdr = element_hdr_at_index(index);
        hdr->status = STATUS_HANDLE;
        return index;
    }

    // No freed slots available, proceed to allocating from the live range. As
    // a first step make sure there is an empty space in the pool or make grow.
    if (obj_pool_num_objects == obj_pool_capacity) {
        if (!grow_pool()) {
            return 0;
        }
    }

    index = obj_pool_num_objects++;
    hdr = element_hdr_at_index(index);
    hdr->status = STATUS_HANDLE;
    return index;
}

/**
 * Returns a pool index to the freed-index stack and marks the slot released.
 *
 * Zeroes the sequence number so that any outstanding handles with the old
 * sequence will fail validation.
 *
 * 0x4E56A0
 */
void release_index(int index)
{
    ObjPoolEntryHeader* hdr;

    hdr = element_hdr_at_index(index);
    recycle_index(index);
    obj_pool_num_objects--;
    sequence_to_hdr(hdr, 0);
    hdr->status = STATUS_RELEASED;
}

/**
 * Allocates one more bucket, extending the pool's capacity by
 * `OBJ_POOL_BUCKET_SIZE`.
 *
 * Returns `false` without allocating if the pool is already at `OBJ_POOL_CAP`.
 *
 * 0x4E56E0
 */
bool grow_pool(void)
{
    if (obj_pool_capacity < OBJ_POOL_CAP) {
        obj_pool_capacity += OBJ_POOL_BUCKET_SIZE;

        // Warn if we're approaching the hard limit.
        if (obj_pool_num_objects + 1 >= obj_pool_warn) {
            tig_debug_printf("WARNING: storing %d objects --", obj_pool_num_objects + 1);
            tig_debug_printf(" near %d, game will crash!\n", OBJ_POOL_CAP);
        }

        obj_pool_buckets[obj_pool_num_buckets++] = (uint8_t*)MALLOC(obj_pool_bucket_byte_size);

        return true;
    }

    // Bad, bad, bad, should never happen.
    if (obj_pool_capacity > OBJ_POOL_CAP) {
        tig_debug_println("Object pool element cap exceeded.  Capacity tracking out of sync");
    }

    return false;
}

/**
 * Pushes `index` onto the freed-index stack.
 *
 * 0x4E5770
 */
void recycle_index(int index)
{
    if (obj_pool_freed_indexes_size == obj_pool_freed_indexes_capacity) {
        obj_pool_freed_indexes_capacity += OBJ_POOL_FREED_INDEXES_GROW_STEP;
        obj_pool_freed_indexes = (int*)REALLOC(obj_pool_freed_indexes, sizeof(*obj_pool_freed_indexes) * obj_pool_freed_indexes_capacity);
    }

    obj_pool_freed_indexes[obj_pool_freed_indexes_size] = index;
    obj_pool_freed_indexes_size++;
}

/**
 * Binary-searches the permanent OID table for `oid`.
 *
 * On a hit writes the entry's index into `*index_ptr` and returns `true`.
 * On a miss writes the insertion point (the index at which `oid` should be
 * inserted to keep the table sorted) into `*index_ptr` and returns `false`.
 *
 * 0x4E57E0
 */
bool find_perm_oid(ObjectID oid, int* index_ptr)
{
    int l;
    int r;
    int m;

    l = 0;
    r = obj_pool_perm_oid_table_size - 1;
    while (l <= r) {
        m = (l + r) / 2;
        // FIXME: Unnecessary copying.
        if (objid_compare(obj_pool_perm_oid_table[m].oid, oid)) {
            l = m + 1;
        } else if (objid_compare(oid, obj_pool_perm_oid_table[m].oid)) {
            r = m - 1;
        } else {
            // Exact match.
            *index_ptr = m;
            return true;
        }
    }

    // Not found, `l` is the correct insertion point to maintain sort order.
    *index_ptr = l;
    return false;
}

/**
 * Encodes `index` and `seq` into a 64-bit handle.
 *
 * 0x4E58C0
 */
int64_t make_handle(int index, int seq)
{
    return ((int64_t)index << HANDLE_INDEX_SHIFT) + (seq << HANDLE_SEQ_SHIFT) + HANDLE_MARKER_VALUE;
}

/**
 * Extracts the pool index from a handle.
 *
 * 0x4E5900
 */
int index_from_handle(int64_t obj)
{
    return (int)(obj >> HANDLE_INDEX_SHIFT);
}

/**
 * Returns a pointer to the `ObjPoolEntryHeader` for the entry at `index`.
 *
 * 0x4E5920
 */
ObjPoolEntryHeader* element_hdr_at_index(int index)
{
    return (ObjPoolEntryHeader*)&(obj_pool_buckets[index / OBJ_POOL_BUCKET_SIZE][obj_pool_element_byte_size * (index % OBJ_POOL_BUCKET_SIZE)]);
}

/**
 * Returns a pointer to the user data portion of the entry at `index`.
 *
 * 0x4E5960
 */
void* element_data_at_index(int index)
{
    ObjPoolEntryHeader* hdr;

    hdr = element_hdr_at_index(index);

    return (uint8_t*)hdr + obj_pool_entry_header_byte_size;
}

/**
 * Stores `seq` into the header's seq field.
 *
 * 0x4E5980
 */
void sequence_to_hdr(ObjPoolEntryHeader* hdr, int seq)
{
    hdr->seq = seq;
}

/**
 * Retrieves the sequence number stored in the entry header.
 *
 * 0x4E59A0
 */
int sequence_from_hdr(ObjPoolEntryHeader* hdr)
{
    return hdr->seq;
}
