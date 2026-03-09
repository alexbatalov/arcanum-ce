#include "game/obj_pool.h"

#include "game/map.h"
#include "game/object.h"

#define OBJ_POOL_CAP 0x200000
#define OBJ_POOL_BUCKET_SIZE 0x2000

#define HANDLE_INDEX_SHIFT 29

#define HANDLE_SEQ_SHIFT 3
#define HANDLE_SEQ_MAX 0x7FFFFF

#define HANDLE_MARKER_MASK 0x7
#define HANDLE_MARKER_VALUE 2

#define STATUS_HANDLE 'H'
#define STATUS_RELEASED 'P'

typedef struct ObjPoolEntryHeader {
    int status : 8;
    int seq : 24;
} ObjPoolEntryHeader;

typedef struct PermOidLookupEntry {
    /* 0000 */ ObjectID oid;
    /* 0018 */ int64_t obj;
} PermOidLookupEntry;

static int acquire_index(void);
static void release_index(int index);
static bool grow_pool(void);
static void recycle_index(int index);
static bool sub_4E57E0(ObjectID oid, int* index_ptr);
static int64_t make_handle(int index, int seq);
static int index_from_handle(int64_t obj);
static ObjPoolEntryHeader* element_hdr_at_index(int index);
static void* element_data_at_index(int index);
static void sequence_to_hdr(ObjPoolEntryHeader* hdr, int seq);
static int sequence_from_hdr(ObjPoolEntryHeader* hdr);

// 0x5B9258
static int obj_pool_entry_header_byte_size = sizeof(ObjPoolEntryHeader);

// 0x6036B0
static int obj_pool_next_seq;

// 0x6036B4
static bool obj_pool_editor;

// 0x6036B8
static PermOidLookupEntry* dword_6036B8;

// 0x6036BC
static unsigned char** obj_pool_buckets;

// 0x6036C0
static int dword_6036C0;

// 0x6036C4
static int obj_pool_num_objects;

// 0x6036C8
static int obj_pool_capacity;

// 0x6036CC
static int obj_pool_num_buckets;

// 0x6036D0
static int obj_pool_freed_indexes_size;

// 0x6036D4
static int obj_pool_bucket_byte_size;

// 0x6036D8
static int* obj_pool_freed_indexes;

// 0x6036DC
static int dword_6036DC;

// 0x6036E0
static int obj_pool_freed_indexes_capacity;

// 0x6036E4
static int obj_pool_element_byte_size;

// 0x6036E8
static int obj_pool_warn;

// 0x6036F0
static int64_t obj_handle_requested;

// 0x6036F8
static bool obj_pool_initialized;

// 0x4E4CD0
void obj_pool_init(int size, bool editor)
{
    obj_pool_buckets = (unsigned char**)CALLOC(256, sizeof(*obj_pool_buckets));
    obj_pool_editor = editor;
    obj_pool_warn = OBJ_POOL_CAP / 4;
    obj_pool_capacity = OBJ_POOL_BUCKET_SIZE;
    obj_pool_next_seq = ((int)time(0) & (HANDLE_SEQ_MAX / 2)) * 2 + 1;
    obj_pool_element_byte_size = obj_pool_entry_header_byte_size + size;
    obj_pool_bucket_byte_size = obj_pool_element_byte_size * OBJ_POOL_BUCKET_SIZE;
    obj_pool_buckets[0] = (unsigned char*)MALLOC(obj_pool_bucket_byte_size);
    obj_pool_num_buckets = 1;
    obj_pool_num_objects = 0;
    obj_pool_freed_indexes_capacity = 4096;
    obj_pool_freed_indexes = (int*)MALLOC(sizeof(*obj_pool_freed_indexes) * obj_pool_freed_indexes_capacity);
    obj_pool_freed_indexes_size = 0;
    dword_6036C0 = 1024;
    dword_6036DC = 0;
    obj_handle_requested = OBJ_HANDLE_NULL;
    dword_6036B8 = (PermOidLookupEntry*)MALLOC(sizeof(*dword_6036B8) * dword_6036C0);
    obj_pool_initialized = true;
}

// 0x4E4DB0
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

    for (index = 0; index < obj_pool_num_buckets; index++) {
        FREE(obj_pool_buckets[index]);
    }

    FREE(obj_pool_freed_indexes);
    FREE(dword_6036B8);
    FREE(obj_pool_buckets);
    obj_pool_initialized = false;
}

// 0x4E4E60
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
        *obj_ptr = obj_handle_requested;

        index = index_from_handle(*obj_ptr);

        hdr = element_hdr_at_index(index);
        hdr->status = STATUS_HANDLE;

        sequence_to_hdr(hdr, (*obj_ptr >> HANDLE_SEQ_SHIFT) & HANDLE_SEQ_MAX);
        object = element_data_at_index(index);
        obj_pool_num_objects++;
        obj_pool_freed_indexes_size--;
        obj_handle_requested = OBJ_HANDLE_NULL;
    } else {
        index = acquire_index();
        object = element_data_at_index(index);
        *obj_ptr = make_handle(index, obj_pool_next_seq);

        hdr = element_hdr_at_index(index);
        sequence_to_hdr(hdr, obj_pool_next_seq);

        obj_pool_next_seq++;
        if (obj_pool_next_seq > HANDLE_SEQ_MAX) {
            obj_pool_next_seq = 1;
        }
    }

    return object;
}

// 0x4E4F80
void* obj_pool_lock(int64_t obj)
{
    return element_data_at_index(index_from_handle(obj));
}

// 0x4E4FA0
void obj_pool_unlock(int64_t obj)
{
    (void)obj;
}

// 0x4E4FB0
void obj_pool_deallocate(int64_t obj)
{
    release_index(index_from_handle(obj));
}

// 0x4E4FD0
void sub_4E4FD0(ObjectID oid, int64_t obj)
{
    int index;

    if (sub_4E57E0(oid, &index)) {
        dword_6036B8[index].obj = obj;
        return;
    }

    if (dword_6036DC == dword_6036C0) {
        dword_6036C0 += 0x200;
        if (dword_6036C0 > OBJ_POOL_CAP) {
            return;
        }

        dword_6036B8 = (PermOidLookupEntry*)REALLOC(dword_6036B8, sizeof(*dword_6036B8) * dword_6036C0);
    }

    if (index != dword_6036DC) {
        memmove(&(dword_6036B8[index + 1]),
            &(dword_6036B8[index]),
            sizeof(*dword_6036B8) * (dword_6036DC - index));
    }

    dword_6036B8[index].oid = oid;
    dword_6036B8[index].obj = obj;
    dword_6036DC++;
}

// 0x4E50E0
int64_t objp_perm_lookup(ObjectID oid)
{
    int idx;
    ObjectList objects;
    ObjectNode* node;

    if (sub_4E57E0(oid, &idx)) {
        if (obj_handle_is_valid(dword_6036B8[idx].obj)) {
            return dword_6036B8[idx].obj;
        }
    }

    if (oid.type != OID_TYPE_P) {
        return OBJ_HANDLE_NULL;
    }

    if (obj_pool_editor) {
        return OBJ_HANDLE_NULL;
    }

    if (map_current_map() != oid.d.p.map) {
        return OBJ_HANDLE_NULL;
    }

    object_list_location(oid.d.p.location, OBJ_TM_TRAP | OBJ_TM_WALL | OBJ_TM_PORTAL | OBJ_TM_SCENERY, &objects);

    if (objects.num_sectors != 1) {
        tig_debug_println("Warning: objp_perm_lookup found sectors != 1");
        object_list_destroy(&objects);
        return OBJ_HANDLE_NULL;
    }

    node = objects.head;
    while (node != NULL) {
        if (obj_field_int32_get(node->obj, OBJ_F_TEMP_ID) == oid.d.p.temp_id) {
            break;
        }
        node = node->next;
    }

    if (node != NULL) {
        sub_4E4FD0(oid, node->obj);
        object_list_destroy(&objects);

        if (!sub_4E57E0(oid, &idx)) {
            tig_debug_println("Error: objp_perm_lookup can't find handle just added with positional id.");
            return OBJ_HANDLE_NULL;
        }

        return dword_6036B8[idx].obj;
    }

    object_list_destroy(&objects);
    return OBJ_HANDLE_NULL;
}

// 0x4E5280
ObjectID sub_4E5280(int64_t obj)
{
    ObjectID oid;
    int index;

    for (index = 0; index < dword_6036DC; index++) {
        if (dword_6036B8[index].obj == obj) {
            return dword_6036B8[index].oid;
        }
    }

    oid.type = OID_TYPE_NULL;

    return oid;
}

// 0x4E52F0
void sub_4E52F0(ObjectID oid)
{
    (void)oid;
}

// 0x4E5300
void sub_4E5300(void)
{
    PermOidLookupEntry* v1;
    int cnt;
    int v2;
    int v3;

    cnt = 0;
    v1 = MALLOC(sizeof(*v1) * dword_6036DC);

    v2 = dword_6036DC - 1;
    v3 = v2;
    while (v2 >= 0) {
        if (dword_6036B8[v2].oid.type == OID_TYPE_A) {
            v1[v3] = dword_6036B8[v2];
            v3--;
            cnt++;
        }
        v2--;
    }

    memset(dword_6036B8, 0, sizeof(*dword_6036B8) * dword_6036DC);
    memcpy(dword_6036B8, &(v1[v3 + 1]), sizeof(*dword_6036B8) * cnt);
    FREE(v1);
    dword_6036DC = cnt;
}

// 0x4E53C0
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

// 0x4E5420
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

// 0x4E5470
bool obj_handle_is_valid(int64_t obj)
{
    int index;
    ObjPoolEntryHeader* hdr;

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if ((obj & HANDLE_MARKER_MASK) != HANDLE_MARKER_VALUE) {
        return false;
    }

    index = (int)(obj >> HANDLE_INDEX_SHIFT);
    if (index >= obj_pool_num_objects + obj_pool_freed_indexes_size) {
        return false;
    }

    hdr = element_hdr_at_index(index);
    if (sequence_from_hdr(hdr) != ((obj >> HANDLE_SEQ_SHIFT) & HANDLE_SEQ_MAX)) {
        return false;
    }

    if (hdr->status != STATUS_HANDLE) {
        return false;
    }

    return true;
}

// 0x4E5530
bool obj_handle_request(int64_t obj)
{
    int index;
    int num_indexes_to_recycle;
    int tmp_index;
    ObjPoolEntryHeader* hdr;

    if ((obj & HANDLE_MARKER_MASK) != HANDLE_MARKER_VALUE) {
        tig_debug_println("Handle requested is not marked as a handle.");
        return false;
    }

    index = index_from_handle(obj);
    if (index < obj_pool_freed_indexes_size + obj_pool_num_objects) {
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

    num_indexes_to_recycle = index - obj_pool_freed_indexes_size - obj_pool_num_objects + 1;

    while (index < obj_pool_capacity) {
        if (!grow_pool()) {
            return false;
        }
    }

    tmp_index = obj_pool_freed_indexes_size + obj_pool_num_objects;
    while (num_indexes_to_recycle != 0) {
        recycle_index(tmp_index);
        hdr = element_hdr_at_index(tmp_index);
        sequence_to_hdr(hdr, 0);
        hdr->status = STATUS_RELEASED;
        tmp_index++;
        num_indexes_to_recycle--;
    }

    obj_handle_requested = obj;

    return true;
}

// 0x4E5640
int acquire_index(void)
{
    int index;
    ObjPoolEntryHeader* hdr;

    if (obj_pool_freed_indexes_size > 0) {
        index = obj_pool_freed_indexes[--obj_pool_freed_indexes_size];
        obj_pool_num_objects++;
        hdr = element_hdr_at_index(index);
        hdr->status = STATUS_HANDLE;
        return index;
    }

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

// 0x4E56A0
void release_index(int index)
{
    ObjPoolEntryHeader* hdr;

    hdr = element_hdr_at_index(index);
    recycle_index(index);
    obj_pool_num_objects--;
    sequence_to_hdr(hdr, 0);
    hdr->status = STATUS_RELEASED;
}

// 0x4E56E0
bool grow_pool(void)
{
    if (obj_pool_capacity < OBJ_POOL_CAP) {
        obj_pool_capacity += OBJ_POOL_BUCKET_SIZE;
        if (obj_pool_num_objects + 1 >= obj_pool_warn) {
            tig_debug_printf("WARNING: storing %d objects --", obj_pool_num_objects + 1);
            tig_debug_printf(" near %d, game will crash!\n", OBJ_POOL_CAP);
        }

        obj_pool_buckets[obj_pool_num_buckets++] = (unsigned char*)MALLOC(obj_pool_bucket_byte_size);

        return true;
    }

    if (obj_pool_capacity > OBJ_POOL_CAP) {
        tig_debug_println("Object pool element cap exceeded.  Capacity tracking out of sync");
    }

    return false;
}

// 0x4E5770
void recycle_index(int index)
{
    if (obj_pool_freed_indexes_size == obj_pool_freed_indexes_capacity) {
        obj_pool_freed_indexes_capacity += 2048;
        obj_pool_freed_indexes = (int*)REALLOC(obj_pool_freed_indexes, sizeof(*obj_pool_freed_indexes) * obj_pool_freed_indexes_capacity);
    }

    obj_pool_freed_indexes[obj_pool_freed_indexes_size] = index;
    obj_pool_freed_indexes_size++;
}

// 0x4E57E0
bool sub_4E57E0(ObjectID oid, int* index_ptr)
{
    int l;
    int r;
    int m;

    l = 0;
    r = dword_6036DC - 1;
    while (l <= r) {
        m = (l + r) / 2;
        // FIXME: Unnecessary copying.
        if (objid_compare(dword_6036B8[m].oid, oid)) {
            l = m + 1;
        } else if (objid_compare(oid, dword_6036B8[m].oid)) {
            r = m - 1;
        } else {
            *index_ptr = m;
            return true;
        }
    }

    *index_ptr = l;
    return false;
}

// 0x4E58C0
int64_t make_handle(int index, int seq)
{
    return ((int64_t)index << HANDLE_INDEX_SHIFT) + (seq << HANDLE_SEQ_SHIFT) + HANDLE_MARKER_VALUE;
}

// 0x4E5900
int index_from_handle(int64_t obj)
{
    return (int)(obj >> HANDLE_INDEX_SHIFT);
}

// 0x4E5920
ObjPoolEntryHeader* element_hdr_at_index(int index)
{
    return (ObjPoolEntryHeader*)&(obj_pool_buckets[index / OBJ_POOL_BUCKET_SIZE][obj_pool_element_byte_size * (index % OBJ_POOL_BUCKET_SIZE)]);
}

// 0x4E5960
void* element_data_at_index(int index)
{
    ObjPoolEntryHeader* hdr;

    hdr = element_hdr_at_index(index);

    return (unsigned char*)hdr + obj_pool_entry_header_byte_size;
}

// 0x4E5980
void sequence_to_hdr(ObjPoolEntryHeader* hdr, int seq)
{
    hdr->seq = seq;
}

// 0x4E59A0
int sequence_from_hdr(ObjPoolEntryHeader* hdr)
{
    return hdr->seq;
}
