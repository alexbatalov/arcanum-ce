#include "game/sa.h"

#include "game/obj_private.h"

// 0x603738
static int dword_603738;

// 0x603740
static void* dword_603740;

// 0x60373C
static SizeableArrayEnumerateCallback* dword_60373C;

// 0x603744
static SizeableArray* dword_603744;

static void sub_4E7990(SizeableArray** sa_ptr, int a2);
static void sub_4E79F0(SizeableArray** sa_ptr, int a2);
static void* sa_data(SizeableArray* sa);
static int sa_byte_size(SizeableArray* sa);
static bool sub_4E7A70(int a1);
static bool sub_4E7AB0(void* ptr, int a2);

// 0x4E7430
void sa_allocate(SizeableArray** sa_ptr, int size)
{
    if (*sa_ptr != NULL) {
        FREE(*sa_ptr);
    }

    *sa_ptr = (SizeableArray*)MALLOC(sizeof(SizeableArray));
    (*sa_ptr)->size = size;
    (*sa_ptr)->count = 0;
    (*sa_ptr)->field_8 = bitset_alloc();
}

// 0x4E7470
void sa_deallocate(SizeableArray** sa_ptr)
{
    bitset_free((*sa_ptr)->field_8);
    FREE(*sa_ptr);
    *sa_ptr = NULL;
}

// 0x4E74A0
void sub_4E74A0(SizeableArray** dst, SizeableArray** src)
{
    int size;

    if (*dst != NULL) {
        sa_deallocate(dst);
    }

    size = sa_byte_size(*src);
    *dst = (SizeableArray*)MALLOC(size);
    memcpy(*dst, *src, size);
    (*dst)->field_8 = bitset_copy((*src)->field_8);
}

// 0x4E7500
void sa_array_copy_to_flat(void* dst, SizeableArray** sa_ptr, int cnt, int size)
{
    if (*sa_ptr == NULL) {
        memset(dst, 0, size * (cnt + 1));
        return;
    }

    if (size != (*sa_ptr)->size) {
        tig_debug_println("Element size differs in sa_array_copy_to_flat.");
        return;
    }

    memset(dst, 0, size * (cnt + 1));
    dword_603740 = dst;
    dword_603738 = cnt;
    sa_enumerate(sa_ptr, sub_4E7AB0);
}

// 0x4E7590
bool sa_write(SizeableArray** sa_ptr, TigFile* stream)
{
    int size;

    size = sa_byte_size(*sa_ptr);
    if (tig_file_fwrite(*sa_ptr, size, 1, stream) != 1) {
        return false;
    }

    return bitset_write_file((*sa_ptr)->field_8, stream);
}

// 0x4E75E0
bool sa_read(SizeableArray** sa_ptr, TigFile* stream)
{
    if (*sa_ptr != NULL) {
        sa_deallocate(sa_ptr);
    }

    return sa_read_no_dealloc(sa_ptr, stream);
}

// 0x4E7610
bool sa_enumerate(SizeableArray** sa_ptr, SizeableArrayEnumerateCallback* callback)
{
    dword_60373C = callback;
    dword_603744 = *sa_ptr;
    return bitset_enumerate(dword_603744->field_8, sub_4E7A70);
}

// 0x4E7640
void sa_set(SizeableArray** sa_ptr, int a2, const void* value)
{
    int index;

    index = bitset_rank((*sa_ptr)->field_8, a2);
    if (!bitset_test((*sa_ptr)->field_8, a2)) {
        bitset_set((*sa_ptr)->field_8, a2, 1);
        sub_4E7990(sa_ptr, index);
    }

    memcpy((uint8_t*)sa_data(*sa_ptr) + (*sa_ptr)->size * index,
        value,
        (*sa_ptr)->size);
}

// 0x4E76C0
void sa_get(SizeableArray** sa_ptr, int a2, void* value)
{
    int index;

    if (!sa_has(sa_ptr, a2)) {
        memset(value, 0, (*sa_ptr)->size);
        return;
    }

    index = bitset_rank((*sa_ptr)->field_8, a2);
    memcpy(value,
        (uint8_t*)sa_data(*sa_ptr) + (*sa_ptr)->size * index,
        (*sa_ptr)->size);
}

// 0x4E7740
bool sa_has(SizeableArray** sa_ptr, int a2)
{
    if (*sa_ptr == NULL) {
        return false;
    }

    return bitset_test((*sa_ptr)->field_8, a2);
}

// 0x4E7760
void sub_4E7760(SizeableArray** sa_ptr, int a2)
{
    if (sa_has(sa_ptr, a2)) {
        sub_4E79F0(sa_ptr, bitset_rank((*sa_ptr)->field_8, a2));
        bitset_set((*sa_ptr)->field_8, a2, 0);
    }
}

// 0x4E77A0
int sa_count(SizeableArray** sa_ptr)
{
    return (*sa_ptr)->count;
}

// 0x4E77B0
int sub_4E77B0(SizeableArray** sa_ptr)
{
    return bitset_serialized_size((*sa_ptr)->field_8) + sa_byte_size(*sa_ptr);
}

// 0x4E77E0
void sub_4E77E0(SizeableArray** src, SizeableArray* dst)
{
    int size;

    size = sa_byte_size(*src);
    memcpy(dst, *src, size);

    bitset_write_mem((*src)->field_8, (uint8_t*)dst + size);
}

// 0x4E7820
void sub_4E7820(SizeableArray** sa_ptr, uint8_t** data)
{
    SizeableArray sa;
    int size;

    if (*sa_ptr != NULL) {
        sa_deallocate(sa_ptr);
    }

    sub_4E4C50(&sa, sizeof(sa), data);

    size = sa_byte_size(&sa);

    *sa_ptr = (SizeableArray*)MALLOC(size);
    (*sa_ptr)->size = sa.size;
    (*sa_ptr)->count = sa.count;
    (*sa_ptr)->field_8 = sa.field_8;

    if (size - sizeof(SizeableArray) != 0) {
        sub_4E4C50(sa_data(*sa_ptr), size - sizeof(SizeableArray), data);
    }

    bitset_read_mem(&((*sa_ptr)->field_8), data);
}

// 0x4E78F0
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
    (*sa_ptr)->field_8 = sa.field_8;

    if (size - sizeof(SizeableArray) != 0) {
        if (tig_file_fread(sa_data(*sa_ptr), size - sizeof(SizeableArray), 1, stream) != 1) {
            return false;
        }
    }

    return bitset_read_file(&((*sa_ptr)->field_8), stream);
}

// 0x4E7990
void sub_4E7990(SizeableArray** sa_ptr, int a2)
{
    int v1;
    uint8_t* data;

    *sa_ptr = (SizeableArray*)REALLOC(*sa_ptr, (*sa_ptr)->size + sa_byte_size(*sa_ptr));

    v1 = (*sa_ptr)->count - a2;
    if (v1 != 0) {
        data = (uint8_t*)sa_data(*sa_ptr);
        memmove(data + (*sa_ptr)->size * (a2 + 1),
            data + (*sa_ptr)->size * a2,
            (*sa_ptr)->size * v1);
    }

    (*sa_ptr)->count++;
}

// 0x4E79F0
void sub_4E79F0(SizeableArray** sa_ptr, int a2)
{
    int v1;
    uint8_t* data;

    v1 = (*sa_ptr)->count - a2 - 1;
    if (v1 != 0) {
        data = (uint8_t*)sa_data(*sa_ptr);
        memmove(data + (*sa_ptr)->size * a2,
            data + (*sa_ptr)->size * (a2 + 1),
            (*sa_ptr)->size * v1);
    }

    (*sa_ptr)->count--;
    *sa_ptr = (SizeableArray*)REALLOC(*sa_ptr, sa_byte_size(*sa_ptr));
}

// 0x4E7A50
void* sa_data(SizeableArray* sa)
{
    return sa + 1;
}

// 0x4E7A60
int sa_byte_size(SizeableArray* sa)
{
    return sa->size * sa->count + sizeof(*sa);
}

// 0x4E7A70
bool sub_4E7A70(int a1)
{
    int index;

    index = bitset_rank(dword_603744->field_8, a1);
    return dword_60373C((uint8_t*)sa_data(dword_603744) + dword_603744->size * index, a1);
}

// 0x4E7AB0
bool sub_4E7AB0(void* entry, int a2)
{
    if (a2 > dword_603738) {
        return false;
    }

    memcpy((uint8_t*)dword_603740 + dword_603744->size * a2,
        entry,
        dword_603744->size);

    return true;
}
