#include "game/tile_script_list.h"

// NOTE: This structure represents tile script list item data as it is stored on
// disk. Its memory layout is set in stone and should never change. The main
// difference from `TileScriptListNode` is the type of the `next` member.
// Originally, it is a pointer, which breaks the memory layout on x64 platforms.
// This value is always `0`.
typedef struct TileScriptListNodeSerializedData {
    /* 0000 */ unsigned int flags;
    /* 0004 */ unsigned int id;
    /* 0008 */ Script scr;
    /* 0014 */ int next;
} TileScriptListNodeSerializedData;

// Serializeable.
static_assert(sizeof(TileScriptListNodeSerializedData) == 0x18, "wrong size");

static void tile_script_node_reserve(void);
static void tile_script_node_clear(void);

/**
 * A pool of the free `TileScriptListNode` instances for reuse.
 *
 * 0x603DD0
 */
static TileScriptListNode* tile_script_node_head;

/**
 * Initializes the tile script node submodule.
 *
 * 0x4F6310
 */
void tile_script_node_init(void)
{
}

/**
 * Shuts down the tile script node submodule.
 *
 * 0x4F6320
 */
void tile_script_node_exit(void)
{
    tile_script_node_clear();
}

/**
 * Allocates a new `TileScriptListNode` instance.
 *
 * 0x4F6330
 */
TileScriptListNode* tile_script_node_create(void)
{
    TileScriptListNode* node;

    // Reserve a new batch if the free list is empty.
    if (tile_script_node_head == NULL) {
        tile_script_node_reserve();
    }

    node = tile_script_node_head;
    tile_script_node_head = node->next;

    node->next = NULL;

    return node;
}

/**
 * Deallocates a `TileScriptListNode` by returning it to the free pool.
 *
 * 0x4F6360
 */
void tile_script_node_destroy(TileScriptListNode* node)
{
    node->next = tile_script_node_head;
    tile_script_node_head = node;
}

/**
 * Allocates a batch of `TileScriptListNode` instances and place them into free
 * pool.
 *
 * 0x4F6380
 */
void tile_script_node_reserve(void)
{
    int index;
    TileScriptListNode* node;

    if (tile_script_node_head == NULL) {
        for (index = 0; index < 8; index++) {
            node = (TileScriptListNode*)MALLOC(sizeof(*node));
            node->next = tile_script_node_head;
            tile_script_node_head = node;
        }
    }
}

/**
 * Deallocates all `TileScriptListNode` instances in the free pool.
 *
 * 0x4F63B0
 */
void tile_script_node_clear(void)
{
    TileScriptListNode* next;

    while (tile_script_node_head != NULL) {
        next = tile_script_node_head->next;
        FREE(tile_script_node_head);
        tile_script_node_head = next;
    }
}

/**
 * Initializes a tile script list.
 *
 * 0x4F63E0
 */
bool tile_script_list_init(TileScriptList* list)
{
    list->flags = 0;
    list->head = NULL;

    return true;
}

/**
 * Resets a tile script list to its initial state.
 *
 * 0x4F6400
 */
bool tile_script_list_reset(TileScriptList* list)
{
    TileScriptListNode* node;
    TileScriptListNode* next;

    node = list->head;
    while (node != NULL) {
        next = node->next;
        tile_script_node_destroy(node);
        node = next;
    }

    list->flags = 0;
    list->head = NULL;

    return true;
}

/**
 * Cleans up a tile script list.
 *
 * 0x4F6460
 */
bool tile_script_list_exit(TileScriptList* list)
{
    return tile_script_list_reset(list);
}

/**
 * Sets a script for a specific tile ID in the list.
 *
 * 0x4F6470
 */
bool tile_script_list_set(TileScriptList* list, unsigned int id, Script* scr)
{
    TileScriptListNode* prev;
    TileScriptListNode* node;
    TileScriptListNode* new_node;

    prev = NULL;
    node = list->head;
    while (node != NULL) {
        if (node->id == id) {
            // // Update existing node.
            node->flags |= TILE_SCRIPT_LIST_NODE_MODIFIED;
            node->scr = *scr;

            // Mark list as modified.
            list->flags |= TILE_SCRIPT_LIST_MODIFIED;

            return true;
        }

        if (node->id > id) {
            break;
        }

        prev = node;
        node = node->next;
    }

    // Create and insert new node.
    new_node = tile_script_node_create();
    new_node->flags = TILE_SCRIPT_LIST_NODE_MODIFIED;
    new_node->id = id;
    new_node->scr = *scr;
    new_node->next = node;

    if (prev != NULL) {
        prev->next = new_node;
    } else {
        list->head = new_node;
    }

    // Mark list as modified.
    list->flags |= TILE_SCRIPT_LIST_MODIFIED;

    return true;
}

/**
 * Removes a script for a specific tile ID from the list.
 *
 * 0x4F6520
 */
bool tile_script_list_remove(TileScriptList* list, unsigned int id)
{
    TileScriptListNode* prev;
    TileScriptListNode* node;

    // Search for the node.
    prev = NULL;
    node = list->head;
    while (node != NULL) {
        if (node->id == id) {
            break;
        }
        prev = node;
        node = node->next;
    }

    if (node == NULL) {
        return false;
    }

    // Unlink the node.
    if (prev != NULL) {
        prev->next = node->next;
    } else {
        list->head = node->next;
    }

    // Free the node.
    tile_script_node_destroy(node);

    // Mark list as modified.
    list->flags |= TILE_SCRIPT_LIST_MODIFIED;

    return true;
}

/**
 * Retrieves a script for a specific tile ID from the list.
 *
 * 0x4F6570
 */
bool tile_script_list_get(TileScriptList* list, unsigned int id, Script* scr)
{
    TileScriptListNode* node;

    // Search for the node.
    node = list->head;
    while (node != NULL) {
        if (node->id == id) {
            *scr = node->scr;
            return true;
        }
        node = node->next;
    }

    return false;
}

/**
 * Loads a tile script list from a file.
 *
 * 0x4F65B0
 */
bool tile_script_list_load(TileScriptList* list, TigFile* stream)
{
    int cnt;
    int index;
    TileScriptListNode* prev;
    TileScriptListNode* node;
    TileScriptListNodeSerializedData serialized;

    prev = NULL;

    // Read the number of nodes.
    cnt = 0;
    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Load each node.
    for (index = 0; index < cnt; index++) {
        node = tile_script_node_create();

        if (tig_file_fread(&serialized, sizeof(serialized), 1, stream) != 1) {
            return false;
        }

        node->flags = serialized.flags;
        node->id = serialized.id;
        node->scr = serialized.scr;

        // Link the node.
        if (prev != NULL) {
            prev->next = node;
        } else {
            list->head = node;
        }
        prev = node;
    }

    return true;
}

/**
 * Saves a tile script list to a file.
 *
 * 0x4F6630
 */
bool tile_script_list_save(TileScriptList* list, TigFile* stream)
{
    int cnt;
    TileScriptListNode* node;
    TileScriptListNodeSerializedData serialized;

    // Count the nodes.
    cnt = 0;
    node = list->head;
    while (node != NULL) {
        cnt++;
        node = node->next;
    }

    // Write the count.
    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Write each node.
    node = list->head;
    while (node != NULL) {
        serialized.flags = node->flags;
        serialized.id = node->id;
        serialized.scr = node->scr;
        serialized.next = 0;

        if (tig_file_fwrite(&serialized, sizeof(serialized), 1, stream) != 1) {
            return false;
        }
        node = node->next;
    }

    // Clear the modified flag.
    list->flags &= ~TILE_SCRIPT_LIST_MODIFIED;

    return true;
}

/**
 * Checks if the tile script list has been modified.
 *
 * 0x4F66C0
 */
bool tile_script_list_is_modified(TileScriptList* list)
{
    return (list->flags & TILE_SCRIPT_LIST_MODIFIED) != 0;
}

/**
 * Loads a differential update for a tile script list from a file.
 *
 * 0x4F66D0
 */
bool tile_script_list_load_with_dif(TileScriptList* list, TigFile* stream)
{
    int cnt;
    int index;
    TileScriptListNodeSerializedData serialized;

    // Read the number of nodes.
    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Load and apply each node.
    for (index = 0; index < cnt; index++) {
        if (tig_file_fread(&serialized, sizeof(serialized), 1, stream) != 1) {
            return false;
        }

        tile_script_list_set(list, serialized.id, &(serialized.scr));
    }

    return true;
}

/**
 * Saves a differential update for a tile script list to a file.
 *
 * 0x4F6750
 */
bool tile_script_list_save_with_dif(TileScriptList* list, TigFile* stream)
{
    int cnt;
    TileScriptListNode* node;
    TileScriptListNodeSerializedData serialized;

    // Count modified nodes.
    cnt = 0;
    node = list->head;
    while (node != NULL) {
        if ((node->flags & TILE_SCRIPT_LIST_NODE_MODIFIED) != 0) {
            cnt++;
        }
        node = node->next;
    }

    // Write the count.
    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Write modified nodes.
    node = list->head;
    while (node != NULL) {
        if ((node->flags & TILE_SCRIPT_LIST_NODE_MODIFIED) != 0) {
            serialized.flags = node->flags;
            serialized.id = node->id;
            serialized.scr = node->scr;
            serialized.next = 0;

            if (tig_file_fwrite(&serialized, sizeof(serialized), 1, stream) != 1) {
                return false;
            }
        }
        node = node->next;
    }

    return true;
}
