#include "game/obj_find.h"

#include "game/obj.h"
#include "game/sector.h"

#define OBJ_FIND_BUCKET_SIZE 128
#define OBJ_FIND_SECTOR_GROW 32

typedef struct FindNode {
    /* 0000 */ unsigned int flags;
    /* 0008 */ int64_t obj;
    /* 0010 */ struct FindNode* prev;
    /* 0014 */ struct FindNode* next;
    /* 0018 */ int64_t sec;
} FindNode;

typedef struct FindSector {
    /* 0000 */ int64_t sec;
    /* 0008 */ FindNode* head;
} FindSector;

static void obj_find_node_reserve();
static void obj_find_node_clear();
static void obj_find_node_allocate(FindNode** obj_find_node);
static void obj_find_node_deallocate(FindNode* obj_find_node);
static void obj_find_node_attach(FindSector* find_sector, FindNode* find_node);
static void obj_find_node_detach(FindNode* find_node);
static void obj_find_sector_reserve();
static bool obj_find_sector_find(int64_t sec, int* index_ptr);
static void obj_find_sector_allocate(int64_t sec, FindSector** find_sector_ptr);
static void obj_find_sector_deallocate(FindSector* find_sector);

/**
 * A pool of the freed `FindNode` instances for reuse.
 *
 * 0x60368C
 */
static FindNode* free_find_node_head;

/**
 * Number of elements in `find_sectors` array.
 *
 * 0x603690
 */
static int find_sectors_size;

/**
 * 0x603694
 */
static FindNode** find_node_buckets;

/**
 * The maximum number of elements in `find_sectors` array.
 *
 * 0x603698
 */
static int find_sectors_capacity;

/**
 * 0x60369C
 */
static FindSector* find_sectors;

/**
 * Number of elements in `find_node_buckets` array.
 *
 * 0x6036A0
 */
static int find_node_buckets_cnt;

/**
 * Flag indicating whether the object find system is initialized.
 *
 * 0x6036A4
 */
static bool obj_find_initialized;

/**
 * Initializes the object find system.
 *
 * 0x4E3900
 */
void obj_find_init()
{
    if (obj_find_initialized) {
        return;
    }

    find_node_buckets_cnt = 0;
    find_node_buckets = NULL;
    free_find_node_head = NULL;
    obj_find_node_reserve();

    find_sectors_size = 0;
    find_sectors_capacity = 0;
    find_sectors = NULL;
    obj_find_sector_reserve();

    obj_find_initialized = true;
}

/**
 * Shuts down up the object find system.
 *
 * 0x4E3950
 */
void obj_find_exit()
{
    if (!obj_find_initialized) {
        return;
    }

    obj_find_node_clear();

    if (find_sectors != NULL) {
        FREE(find_sectors);
    }

    obj_find_initialized = false;
}

/**
 * Adds an object to the find system.
 *
 * 0x4E3980
 */
void obj_find_add(int64_t obj)
{
    int64_t loc;
    int64_t sec;
    FindNode* find_node;
    FindSector* find_sector;

    // Retrieve the object's location/sector.
    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    sec = sector_id_from_loc(loc);

    obj_find_node_allocate(&find_node);
    find_node->obj = obj;

    obj_find_sector_allocate(sec, &find_sector);
    obj_find_node_attach(find_sector, find_node);

    // Keep reference to the find node.
    obj_field_ptr_set(obj, OBJ_F_FIND_NODE, find_node);
}

/**
 * Removes an object from the find system.
 *
 * 0x4E39F0
 */
void obj_find_remove(int64_t obj)
{
    FindNode* find_node;

    // Retrieve the object's find node.
    find_node = (FindNode*)obj_field_ptr_get(obj, OBJ_F_FIND_NODE);

    // Validate that the node is active.
    if ((find_node->flags & 0x02) == 0) {
        tig_debug_println("Error: Node already released in obj_find_remove.");
        return;
    }

    // Validate that the node is assigned to the correct object.
    if (find_node->obj != obj) {
        tig_debug_println("Error: Node already reassigned to different handlein obj_find_remove.");
        return;
    }

    // Detach the node from sector and deallocate it (returning to the free
    // pool).
    obj_find_node_detach(find_node);
    obj_find_node_deallocate(find_node);

    // Nullify reference to the find node.
    obj_field_ptr_set(obj, OBJ_F_FIND_NODE, NULL);
}

/**
 * Notifies the object find system that object's location has changed.
 *
 * 0x4E3A70
 */
void obj_find_move(int64_t obj)
{
    int64_t loc;
    int64_t sec;
    FindNode* find_node;
    FindSector* find_sector;

    // Retrieve the new object's location/sector.
    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    sec = sector_id_from_loc(loc);

    // Retrieve the object's find node.
    find_node = (FindNode*)obj_field_ptr_get(obj, OBJ_F_FIND_NODE);
    if (find_node == NULL) {
        return;
    }

    // Validate that the node is active.
    if ((find_node->flags & 0x02) == 0) {
        tig_debug_println("Error: Node already released in obj_find_move.");
        return;
    }

    // Validate that the node is assigned to the correct object.
    if (find_node->obj != obj) {
        tig_debug_println("Error: Node already reassigned to different handlein obj_find_move");
        return;
    }

    // Move the node to a new sector if the sector has changed.
    if (find_node->sec != sec) {
        // Detach from old sector.
        obj_find_node_detach(find_node);

        // Attach to the new sector.
        obj_find_sector_allocate(sec, &find_sector);
        obj_find_node_attach(find_sector, find_node);
    }
}

/**
 * Initiates iteration over objects in a specified sector.
 *
 * 0x4E3B30
 */
bool obj_find_walk_first(int64_t sec, int64_t* obj_ptr, FindNode** iter_ptr)
{
    int index;
    FindNode* node;

    if (!obj_find_sector_find(sec, &index)) {
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    node = find_sectors[index].head;
    if (node == NULL) {
        tig_debug_println("Found empty sector in obj_find_walk_first.");
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    *obj_ptr = node->obj;
    *iter_ptr = node->next;

    return true;
}

/**
 * Continues iteration over objects in a sector.
 *
 * 0x4E3BA0
 */
bool obj_find_walk_next(int64_t* obj_ptr, FindNode** iter_ptr)
{
    if (*iter_ptr != NULL) {
        *obj_ptr = (*iter_ptr)->obj;
        *iter_ptr = (*iter_ptr)->next;
        return true;
    } else {
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }
}

/**
 * Allocates a new bucket of `FindNode` instances and adds them to the free
 * list.
 *
 * 0x4E3BE0
 */
void obj_find_node_reserve()
{
    int index;

    find_node_buckets_cnt++;

    // Allocate or grow list of buckets.
    if (find_node_buckets == NULL) {
        find_node_buckets = (FindNode**)MALLOC(sizeof(*find_node_buckets) * find_node_buckets_cnt);
    } else {
        find_node_buckets = (FindNode**)REALLOC(find_node_buckets, sizeof(*find_node_buckets) * find_node_buckets_cnt);
    }

    // Allocate a new bucket.
    find_node_buckets[find_node_buckets_cnt - 1] = (FindNode*)MALLOC(sizeof(FindNode) * OBJ_FIND_BUCKET_SIZE);

    // Add each node in the new bucket to the free list.
    for (index = 0; index < OBJ_FIND_BUCKET_SIZE; index++) {
        obj_find_node_deallocate(&(find_node_buckets[find_node_buckets_cnt - 1][index]));
    }
}

/**
 * Frees all allocated `FindNode` buckets and the list of buckets itself.
 *
 * 0x4E3C60
 */
void obj_find_node_clear()
{
    while (find_node_buckets_cnt != 0) {
        FREE(find_node_buckets[--find_node_buckets_cnt]);
    }

    FREE(find_node_buckets);
    find_node_buckets = NULL;
}

/**
 * Allocates a `FindNode` from the free list, reserving a new bucket if needed.
 *
 * 0x4E3CB0
 */
void obj_find_node_allocate(FindNode** obj_find_node)
{
    // Reserve a new bucket if the free list is empty.
    if (free_find_node_head == NULL) {
        obj_find_node_reserve();
    }

    *obj_find_node = free_find_node_head;

    // Check for double allocation.
    if (((*obj_find_node)->flags & 0x02) != 0) {
        tig_debug_println("Error: Allocating twice in obj_find_node_allocate.");
        tig_message_post_quit(0);
    }

    // Mark the node as active.
    (*obj_find_node)->flags |= 0x02;

    free_find_node_head = free_find_node_head->next;
}

/**
 * Deallocates a `FindNode` by returning it to the free list.
 *
 * 0x4E3D10
 */
void obj_find_node_deallocate(FindNode* obj_find_node)
{
    obj_find_node->flags = 0;

    obj_find_node->next = free_find_node_head;
    free_find_node_head = obj_find_node;
}

/**
 * Attaches a find node to the find sector.
 *
 * 0x4E3D30
 */
void obj_find_node_attach(FindSector* find_sector, FindNode* find_node)
{
    find_node->prev = NULL;
    find_node->next = find_sector->head;
    if (find_sector->head != NULL) {
        find_sector->head->prev = find_node;
    }
    find_sector->head = find_node;
    find_node->sec = find_sector->sec;
}

/**
 * Detaches a find node from a find sector.
 *
 * 0x4E3D60
 */
void obj_find_node_detach(FindNode* find_node)
{
    int index;
    FindSector* find_sector;

    if (!obj_find_sector_find(find_node->sec, &index)) {
        tig_debug_println("Error: Can't find sector in obj_find_node_detach.");
        return;
    }

    find_sector = &(find_sectors[index]);

    if (find_node->next != NULL) {
        find_node->next->prev = find_node->prev;
    }

    if (find_node->prev != NULL) {
        find_node->prev->next = find_node->next;
    } else {
        find_sector->head = find_node->next;

        // Deallocate the sector if it becomes empty.
        if (find_sector->head == NULL) {
            obj_find_sector_deallocate(find_sector);
        }
    }
}

/**
 * Allocates more `FindSector` instances.
 *
 * 0x4E3DD0
 */
void obj_find_sector_reserve()
{
    find_sectors_capacity += OBJ_FIND_SECTOR_GROW;

    if (find_sectors != NULL) {
        find_sectors = (FindSector*)REALLOC(find_sectors, sizeof(*find_sectors) * find_sectors_capacity);
    } else {
        find_sectors = (FindSector*)MALLOC(sizeof(*find_sectors) * find_sectors_capacity);
    }
}

/**
 * Searches for an index in `find_sectors` array for the specified sector.
 *
 * Returns `true` if the sector is found, `false` otherwise.
 *
 * 0x4E3E10.
 */
bool obj_find_sector_find(int64_t sec, int* index_ptr)
{
    int l;
    int r;
    int m;

    l = 0;
    r = find_sectors_size - 1;
    while (l <= r) {
        m = (l + r) / 2;
        if (find_sectors[m].sec == sec) {
            *index_ptr = m;
            return true;
        } else if (find_sectors[m].sec < sec) {
            l = m + 1;
        } else {
            r = m - 1;
        }
    }

    *index_ptr = l;
    return false;
}

/**
 * Allocates a new `FindSector` instance for a given sector.
 *
 * 0x4E3E90
 */
void obj_find_sector_allocate(int64_t sec, FindSector** find_sector_ptr)
{
    int index;

    // Check if the `FindSector` already exists.
    if (obj_find_sector_find(sec, &index)) {
        *find_sector_ptr = &(find_sectors[index]);
        return;
    }

    // Grow the find sector array.
    if (find_sectors_size == find_sectors_capacity) {
        obj_find_sector_reserve();
    }

    // Shift existing find sectors to make a room for a new instance.
    if (find_sectors_size - index != 0) {
        memmove(&(find_sectors[index + 1]),
            &(find_sectors[index]),
            sizeof(*find_sectors) * (find_sectors_size - index));
    }

    // Initialize the new find sector.
    *find_sector_ptr = &(find_sectors[index]);
    (*find_sector_ptr)->sec = sec;
    (*find_sector_ptr)->head = NULL;

    find_sectors_size++;
}

/**
 * Deallocates a `FindSector`.
 *
 * 0x4E3F40
 */
void obj_find_sector_deallocate(FindSector* find_sector)
{
    int index;

    // Calculate the index of the element in array.
    index = (int)(find_sector - find_sectors);

    // Shift remaining sectors to fill the gap.
    if (find_sectors_size - index != 1) {
        memmove(&(find_sectors[index]),
            &(find_sectors[index + 1]),
            sizeof(*find_sectors) * (find_sectors_size - index - 1));
    }

    find_sectors_size--;
}
