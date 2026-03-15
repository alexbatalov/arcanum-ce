#include "game/sa.h"

#include "game/obj_private.h"

// 0x603738
static int sa_flat_copy_cnt;

// 0x603740
static void* sa_flat_copy_dst;

// 0x60373C
static SizeableArrayEnumerateCallback* sa_enumerate_cb;

// 0x603744
static SizeableArray* sa_enumerate_arr;

static void sa_insert_slot(SizeableArray** sa_ptr, int index);
static void sa_remove_slot(SizeableArray** sa_ptr, int index);
static void* sa_data(SizeableArray* sa);
static int sa_byte_size(SizeableArray* sa);
static bool sa_enumerate_worker(int key);
static bool sa_array_copy_to_flat_worker(void* entry, int key);

/**
 * Allocates a new `SizeableArray` with the given element size in bytes.
 *
 * 0x4E7430
 */
void sa_allocate(SizeableArray** sa_ptr, int size)
{
    if (*sa_ptr != NULL) {
        // FIXME: Should call `sa_deallocate` here to also destroy the
        // existing bitset.
        FREE(*sa_ptr);
    }

    *sa_ptr = (SizeableArray*)MALLOC(sizeof(SizeableArray));
    (*sa_ptr)->size = size;
    (*sa_ptr)->count = 0;
    (*sa_ptr)->bitset_id = bitset_alloc();
}

/**
 * Destroys a `SizeableArray`, freeing both the element storage and the
 * associated bitset.
 *
 * 0x4E7470
 */
void sa_deallocate(SizeableArray** sa_ptr)
{
    bitset_free((*sa_ptr)->bitset_id);
    FREE(*sa_ptr);
    *sa_ptr = NULL;
}

/**
 * Deep-copies `*src` into `*dst`.
 *
 * 0x4E74A0
 */
void sa_copy(SizeableArray** dst, SizeableArray** src)
{
    int size;

    if (*dst != NULL) {
        sa_deallocate(dst);
    }

    size = sa_byte_size(*src);
    *dst = (SizeableArray*)MALLOC(size);
    memcpy(*dst, *src, size);
    (*dst)->bitset_id = bitset_copy((*src)->bitset_id);
}

/**
 * Copies all elements from a `SizeableArray` into a flat (plain C) array.
 *
 * 0x4E7500
 */
void sa_array_copy_to_flat(void* dst, SizeableArray** sa_ptr, int cnt, int size)
{
    if (*sa_ptr == NULL) {
        memset(dst, 0, size * (cnt + 1));
        return;
    }

    // Sanity-check: the caller's element size must agree with the array's.
    if (size != (*sa_ptr)->size) {
        tig_debug_println("Element size differs in sa_array_copy_to_flat.");
        return;
    }

    // Zero the entire destination range so absent keys become zero elements.
    memset(dst, 0, size * (cnt + 1));

    // Store parameters as globals so that the `sa_array_copy_to_flat_worker`
    // callback can access them.
    sa_flat_copy_dst = dst;
    sa_flat_copy_cnt = cnt;
    sa_enumerate(sa_ptr, sa_array_copy_to_flat_worker);
}

/**
 * Writes a `SizeableArray` to a `TigFile` stream.
 *
 * Returns `true` on success, `false` otherwise.
 *
 * 0x4E7590
 */
bool sa_write_file(SizeableArray** sa_ptr, TigFile* stream)
{
    int size;

    // Write header and all element data as a flat blog.
    size = sa_byte_size(*sa_ptr);
    if (tig_file_fwrite(*sa_ptr, size, 1, stream) != 1) {
        return false;
    }

    // Write associated bitset.
    return bitset_write_file((*sa_ptr)->bitset_id, stream);
}

/**
 * Reads a `SizeableArray` from a `TigFile` stream.
 *
 * Returns `true` on success, `false` otherwise.
 *
 * 0x4E75E0
 */
bool sa_read_file(SizeableArray** sa_ptr, TigFile* stream)
{
    if (*sa_ptr != NULL) {
        sa_deallocate(sa_ptr);
    }

    return sa_read_no_dealloc(sa_ptr, stream);
}

/**
 * Iterates over every element in the array, invoking `callback` for each one.
 *
 * The callback receives a pointer to the element data and the element's
 * integer key. Iteration stops early if the callback returns `false`.
 *
 * 0x4E7610
 */
bool sa_enumerate(SizeableArray** sa_ptr, SizeableArrayEnumerateCallback* callback)
{
    sa_enumerate_cb = callback;
    sa_enumerate_arr = *sa_ptr;
    return bitset_enumerate(sa_enumerate_arr->bitset_id, sa_enumerate_worker);
}

/**
 * Inserts or replaces the element with the given key.
 *
 * If the key is new, a slot is inserted at the correct sorted position and the
 * element buffer is grown by one. The value is then copied into the slot.
 * If the key already exists its element is overwritten in-place.
 *
 * 0x4E7640
 */
void sa_set(SizeableArray** sa_ptr, int key, const void* value)
{
    int index;

    // Compute the rank (sorted position) of this key before potentially
    // adding it. If the key is already present, rank gives is existing
    // position. Otherwise rank is an insertion point.
    index = bitset_rank((*sa_ptr)->bitset_id, key);

    if (!bitset_test((*sa_ptr)->bitset_id, key)) {
        bitset_set((*sa_ptr)->bitset_id, key, true);
        sa_insert_slot(sa_ptr, index);
    }

    // Write the value into the slot at `index` (either the existing slot or
    // the freshly inserted one).
    memcpy((uint8_t*)sa_data(*sa_ptr) + (*sa_ptr)->size * index,
        value,
        (*sa_ptr)->size);
}

/**
 * Retrieves the element with the given key into `value`.
 *
 * 0x4E76C0
 */
void sa_get(SizeableArray** sa_ptr, int key, void* value)
{
    int index;

    if (!sa_has(sa_ptr, key)) {
        // Key is not present.
        memset(value, 0, (*sa_ptr)->size);
        return;
    }

    // Resolve key to its sorted position in the element buffer.
    index = bitset_rank((*sa_ptr)->bitset_id, key);

    // Copy associated value.
    memcpy(value,
        (uint8_t*)sa_data(*sa_ptr) + (*sa_ptr)->size * index,
        (*sa_ptr)->size);
}

/*
 * Returns `true` if the array contains an element with the given key.
 *
 * 0x4E7740
 */
bool sa_has(SizeableArray** sa_ptr, int key)
{
    if (*sa_ptr == NULL) {
        return false;
    }

    return bitset_test((*sa_ptr)->bitset_id, key);
}

/**
 * Removes the element with the given key.
 *
 * 0x4E7760
 */
void sa_remove(SizeableArray** sa_ptr, int key)
{
    int index;

    if (sa_has(sa_ptr, key)) {
        index = bitset_rank((*sa_ptr)->bitset_id, key);
        sa_remove_slot(sa_ptr, index);

        bitset_set((*sa_ptr)->bitset_id, key, false);
    }
}

/**
 * Returns the number of elements currently stored in the array.
 *
 * 0x4E77A0
 */
int sa_count(SizeableArray** sa_ptr)
{
    return (*sa_ptr)->count;
}

/**
 * Returns the total number of bytes required to serialize the array,
 * including both the element buffer and the associated bitst.
 *
 * 0x4E77B0
 */
int sa_total_byte_size(SizeableArray** sa_ptr)
{
    return bitset_serialized_size((*sa_ptr)->bitset_id) + sa_byte_size(*sa_ptr);
}

/**
 * Serializes the array to a memory buffer.
 *
 * 0x4E77E0
 */
void sa_write_mem(SizeableArray** src, SizeableArray* dst)
{
    int size;

    // Copy the header and element buffer.
    size = sa_byte_size(*src);
    memcpy(dst, *src, size);

    // Copy associated bitset.
    bitset_write_mem((*src)->bitset_id, (uint8_t*)dst + size);
}

/**
 * Deserializes an array from a memory buffer.
 *
 * 0x4E7820
 */
void sa_read_mem(SizeableArray** sa_ptr, uint8_t** data)
{
    SizeableArray sa;
    int size;

    if (*sa_ptr != NULL) {
        sa_deallocate(sa_ptr);
    }

    // Read the fixed-size header into a temporary array to we can compute
    // allocation size.
    mem_read_advance(&sa, sizeof(sa), data);

    size = sa_byte_size(&sa);

    // Allocate and populate the new array.
    *sa_ptr = (SizeableArray*)MALLOC(size);
    (*sa_ptr)->size = sa.size;
    (*sa_ptr)->count = sa.count;
    (*sa_ptr)->bitset_id = sa.bitset_id;

    // Read element data (if present).
    if (size - sizeof(SizeableArray) != 0) {
        mem_read_advance(sa_data(*sa_ptr), size - sizeof(SizeableArray), data);
    }

    // Read the associated bitset.
    bitset_read_mem(&((*sa_ptr)->bitset_id), data);
}

/**
 * Reads a `SizeableArray` from a `TigFile` stream without deallocating
 * `*sa_ptr` beforehand.
 *
 * 0x4E78F0
 */
bool sa_read_no_dealloc(SizeableArray** sa_ptr, TigFile* stream)
{
    SizeableArray sa;
    int size;

    if (tig_file_fread(&sa, sizeof(sa), 1, stream) != 1) {
        return false;
    }

    size = sa_byte_size(&sa);

    *sa_ptr = (SizeableArray*)MALLOC(size);
    (*sa_ptr)->size = sa.size;
    (*sa_ptr)->count = sa.count;
    (*sa_ptr)->bitset_id = sa.bitset_id;

    if (size - sizeof(SizeableArray) != 0) {
        if (tig_file_fread(sa_data(*sa_ptr), size - sizeof(SizeableArray), 1, stream) != 1) {
            return false;
        }
    }

    return bitset_read_file(&((*sa_ptr)->bitset_id), stream);
}

/**
 * Inserts a blank element slot at sorted position `index`, shifting all
 * existing elements at and above that position one slot to the right.
 *
 * 0x4E7990
 */
void sa_insert_slot(SizeableArray** sa_ptr, int index)
{
    int elements_to_shift;
    uint8_t* data;

    // Grow the allocation by one element.
    *sa_ptr = (SizeableArray*)REALLOC(*sa_ptr, sa_byte_size(*sa_ptr) + (*sa_ptr)->size);

    // Shift subsequent elements to the right.
    elements_to_shift = (*sa_ptr)->count - index;
    if (elements_to_shift != 0) {
        data = (uint8_t*)sa_data(*sa_ptr);
        memmove(data + (*sa_ptr)->size * (index + 1),
            data + (*sa_ptr)->size * index,
            (*sa_ptr)->size * elements_to_shift);
    }

    (*sa_ptr)->count++;
}

/**
 * Removes the element slot at sorted position `index`, shifting all subsequent
 * elements one slot to the left.
 *
 * 0x4E79F0
 */
void sa_remove_slot(SizeableArray** sa_ptr, int index)
{
    int elements_to_shift;
    uint8_t* data;

    // Shift subsequent elements to the left.
    elements_to_shift = (*sa_ptr)->count - index - 1;
    if (elements_to_shift != 0) {
        data = (uint8_t*)sa_data(*sa_ptr);
        memmove(data + (*sa_ptr)->size * index,
            data + (*sa_ptr)->size * (index + 1),
            (*sa_ptr)->size * elements_to_shift);
    }

    // Decrement element count so that subsequent call to `sa_byte_size` returns
    // reduced size and shrink the allocation.
    (*sa_ptr)->count--;
    *sa_ptr = (SizeableArray*)REALLOC(*sa_ptr, sa_byte_size(*sa_ptr));
}

/**
 * Returns a pointer to the element buffer, which begins immediately after the
 * `SizeableArray` header.
 *
 * 0x4E7A50
 */
void* sa_data(SizeableArray* sa)
{
    return sa + 1;
}

/**
 * Returns the byte size of the `SizeableArray` array (header + data).
 *
 * This value does not include the size of the associated bitset. See
 * `sa_total_byte_size`.
 *
 * 0x4E7A60
 */
int sa_byte_size(SizeableArray* sa)
{
    return sa->size * sa->count + sizeof(*sa);
}

/**
 * Internal callback forwarded to `bitset_enumerate` during `sa_enumerate`.
 *
 * Returns the value returned by the user callback (false stops enumeration).
 *
 * 0x4E7A70
 */
bool sa_enumerate_worker(int key)
{
    int index;
    void* entry;

    // Resolve key to its sorted position in the element buffer.
    index = bitset_rank(sa_enumerate_arr->bitset_id, key);

    // Proceed to the original callback supplied to `sa_enumerate`.
    entry = (uint8_t*)sa_data(sa_enumerate_arr) + sa_enumerate_arr->size * index;
    return sa_enumerate_cb(entry, key);
}

/**
 * Internal callback used by `sa_array_copy_to_flat`() to copy one element from
 * the sparse array into the flat destination array.
 *
 * 0x4E7AB0
 */
bool sa_array_copy_to_flat_worker(void* entry, int key)
{
    // Keys are enumerated in ascending order. Once we exceed the max we are
    // done - no further keys can be in range.
    if (key > sa_flat_copy_cnt) {
        return false;
    }

    // Place the element at the corresponding position in the flat array.
    memcpy((uint8_t*)sa_flat_copy_dst + sa_enumerate_arr->size * key,
        entry,
        sa_enumerate_arr->size);

    return true;
}
