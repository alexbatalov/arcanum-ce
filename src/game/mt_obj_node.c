#include "game/mt_obj_node.h"

#include "game/obj.h"

#define GROW 64

static void mt_obj_node_reserve(void);
static void mt_obj_node_clear(void);
static bool mt_obj_node_save_list_node(MagicTechObjectNode* node, TigFile* stream);
static bool mt_obj_node_load_list_node(MagicTechObjectNode* node, TigFile* stream);

/**
 * A pool of the free `MagicTechObjectNode` instances for reuse.
 *
 * 0x5FC368
 */
static MagicTechObjectNode* mt_obj_node_head;

/**
 * Called when the game is initialized.
 *
 * 0x4BAC10
 */
bool mt_obj_node_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4BAC20
 */
void mt_obj_node_exit(void)
{
    mt_obj_node_clear();
}

/**
 * Allocates a new `MagicTechObjectNode` instance.
 *
 * 0x4BAC30
 */
MagicTechObjectNode* mt_obj_node_create(void)
{
    MagicTechObjectNode* node;

    // Reserve a new batch if the free pool is empty.
    if (mt_obj_node_head == NULL) {
        mt_obj_node_reserve();
    }

    node = mt_obj_node_head;
    mt_obj_node_head = node->next;

    node->next = NULL;
    node->obj = OBJ_HANDLE_NULL;
    node->type = -1;
    node->aptitude = 0;
    node->field_30 = 0;

    return node;
}

/**
 * Deallocates a `MagicTechObjectNode` by returning it to the free pool.
 *
 * 0x4BAC80
 */
void mt_obj_node_destroy(MagicTechObjectNode* node)
{
    node->next = mt_obj_node_head;
    mt_obj_node_head = node;
}

/**
 * Allocates a batch of `MagicTechObjectNode` instances and place them into free
 * pool.
 *
 * 0x4BACA0
 */
void mt_obj_node_reserve(void)
{
    int index;
    MagicTechObjectNode* node;

    if (mt_obj_node_head == NULL) {
        for (index = 0; index < GROW; index++) {
            node = (MagicTechObjectNode*)MALLOC(sizeof(*node));
            node->next = mt_obj_node_head;
            mt_obj_node_head = node;
        }
    }
}

/**
 * Deallocates all `MagicTechObjectNode` instances in the free pool.
 *
 * 0x4BACD0
 */
void mt_obj_node_clear(void)
{
    MagicTechObjectNode* next;

    while (mt_obj_node_head != NULL) {
        next = mt_obj_node_head->next;
        FREE(mt_obj_node_head);
        mt_obj_node_head = next;
    }
}

/**
 * Saves a list of `MagicTechObjectNode` to a file.
 *
 * 0x4BAD00
 */
bool mt_obj_node_save_list(MagicTechObjectNode** head_ptr, TigFile* stream)
{
    int cnt = 0;
    MagicTechObjectNode* node;

    if (head_ptr == NULL) {
        return false;
    }

    // Count the nodes.
    node = *head_ptr;
    while (node != NULL) {
        cnt++;
        node = node->next;
    }

    // Write the count.
    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Write each node.
    node = *head_ptr;
    while (node != NULL) {
        if (!mt_obj_node_save_list_node(node, stream)) {
            return false;
        }
        node = node->next;
    }

    return true;
}

/**
 * Saves a single `MagicTechObjectNode` to a file.
 *
 * 0x4BAD70
 */
bool mt_obj_node_save_list_node(MagicTechObjectNode* node, TigFile* stream)
{
    if (node == NULL) {
        return false;
    }

    if (!object_save_obj_handle_safe(&(node->obj), &(node->field_8), stream)) {
        return false;
    }

    if (tig_file_fwrite(&(node->aptitude), sizeof(node->aptitude), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(node->field_30), sizeof(node->field_30), 1, stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Loads a list of `MagicTechObjectNode` from a file.
 *
 * 0x4BADD0
 */
bool mt_obj_node_load_list(MagicTechObjectNode** head_ptr, TigFile* stream)
{
    int cnt = 0;
    int index;
    MagicTechObjectNode* node;

    if (head_ptr == NULL) {
        return false;
    }

    // Read the number of nodes.
    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    // Load each node.
    for (index = 0; index < cnt; index++) {
        node = *head_ptr = mt_obj_node_create();

        if (!mt_obj_node_load_list_node(node, stream)) {
            return false;
        }

        head_ptr = &(node->next);
        node->next = NULL;
    }

    return true;
}

/**
 * Loads a single `MagicTechObjectNode` from a file.
 *
 * 0x4BAE50
 */
bool mt_obj_node_load_list_node(MagicTechObjectNode* node, TigFile* stream)
{
    if (node == NULL) {
        return false;
    }

    if (!object_load_obj_handle_safe(&(node->obj), &(node->field_8), stream)) {
        return false;
    }

    if (tig_file_fread(&(node->aptitude), sizeof(node->aptitude), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fread(&(node->field_30), sizeof(node->field_30), 1, stream) != 1) {
        return false;
    }

    if (node->obj != OBJ_HANDLE_NULL) {
        node->type = obj_field_int32_get(node->obj, OBJ_F_TYPE);
    } else {
        node->type = -1;
    }

    return true;
}

/**
 * Saves a detached `MagicTechObjectNode` to a file.
 *
 * 0x4BAEE0
 */
bool mt_obj_node_save_detached(MagicTechObjectNode* node, TigFile* stream)
{
    if (node == NULL) {
        return false;
    }

    if (!object_save_obj_handle_safe(&(node->obj), &(node->field_8), stream)) {
        return false;
    }

    if (tig_file_fwrite(&(node->aptitude), sizeof(node->aptitude), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(node->field_30), sizeof(node->field_30), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(node->loc), sizeof(node->loc), 1, stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Loads a detached `MagicTechObjectNode` from a file.
 *
 * 0x4BAF50
 */
bool mt_obj_node_load_detached(MagicTechObjectNode* node, TigFile* stream)
{
    if (node == NULL) {
        return false;
    }

    if (!object_load_obj_handle_safe(&(node->obj), &(node->field_8), stream)) {
        return false;
    }

    if (tig_file_fread(&(node->aptitude), sizeof(node->aptitude), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fread(&(node->field_30), sizeof(node->field_30), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fread(&(node->loc), sizeof(node->loc), 1, stream) != 1) {
        return false;
    }

    if (node->obj != OBJ_HANDLE_NULL) {
        node->type = obj_field_int32_get(node->obj, OBJ_F_TYPE);
    } else {
        node->type = -1;
    }

    return true;
}
