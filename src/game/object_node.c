#include "game/object_node.h"

#define OBJECT_NODE_LIST_GROW_SIZE 64

static void object_node_reserve(void);
static void object_node_remove_all(void);

/**
 * A pool of the free `ObjectNode` instances for reuse.
 *
 * 0x603AC0
 */
static ObjectNode* object_node_head;

/**
 * Called when the game is initialized.
 *
 * 0x4E9920
 */
bool object_node_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4E9930
 */
void object_node_exit(void)
{
    object_node_remove_all();
}

/**
 * Creates a new `ObjectNode` instance, reusing from the free pool if
 * available.
 *
 * 0x4E9940
 */
ObjectNode* object_node_create(void)
{
    ObjectNode* node;

    node = object_node_head;
    if (node == NULL) {
        object_node_reserve();
        node = object_node_head;
    }

    object_node_head = node->next;
    node->next = NULL;

    return node;
}

/**
 * Destroys an `ObjectNode` instance by returning it to the free pool.
 *
 * 0x4E9970
 */
void object_node_destroy(ObjectNode* node)
{
    node->next = object_node_head;
    object_node_head = node;
}

/**
 * Allocates a batch of `ObjectNode` instances and place them into free pool.
 *
 * 0x4E9990
 */
void object_node_reserve(void)
{
    int index;
    ObjectNode* node;

    if (object_node_head != NULL) {
        return;
    }

    for (index = 0; index < OBJECT_NODE_LIST_GROW_SIZE; index++) {
        node = (ObjectNode*)MALLOC(sizeof(*node));
        node->next = object_node_head;
        object_node_head = node;
    }
}

/**
 * Deallocates all `ObjectNode` instances in the free pool.
 *
 * 0x4E99C0
 */
void object_node_remove_all(void)
{
    ObjectNode* next;

    while (object_node_head != NULL) {
        next = object_node_head->next;
        FREE(object_node_head);
        object_node_head = next;
    }
}
