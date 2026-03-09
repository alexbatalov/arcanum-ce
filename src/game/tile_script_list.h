#ifndef ARCANUM_GAME_TILE_SCRIPT_LIST_H_
#define ARCANUM_GAME_TILE_SCRIPT_LIST_H_

#include "game/script.h"

#define TILE_SCRIPT_LIST_NODE_MODIFIED 0x0001

typedef struct TileScriptListNode {
    /* 0000 */ unsigned int flags;
    /* 0004 */ unsigned int id;
    /* 0008 */ Script scr;
    /* 0014 */ struct TileScriptListNode* next;
} TileScriptListNode;

#define TILE_SCRIPT_LIST_MODIFIED 0x0001

typedef struct TileScriptList {
    /* 0000 */ unsigned int flags;
    /* 0004 */ struct TileScriptListNode* head;
} TileScriptList;

void tile_script_node_init(void);
void tile_script_node_exit(void);
TileScriptListNode* tile_script_node_create(void);
void tile_script_node_destroy(TileScriptListNode* node);
bool tile_script_list_init(TileScriptList* list);
bool tile_script_list_reset(TileScriptList* list);
bool tile_script_list_exit(TileScriptList* list);
bool tile_script_list_set(TileScriptList* list, unsigned int id, Script* scr);
bool tile_script_list_remove(TileScriptList* list, unsigned int id);
bool tile_script_list_get(TileScriptList* list, unsigned int id, Script* scr);
bool tile_script_list_load(TileScriptList* list, TigFile* stream);
bool tile_script_list_save(TileScriptList* list, TigFile* stream);
bool tile_script_list_is_modified(TileScriptList* list);
bool tile_script_list_load_with_dif(TileScriptList* list, TigFile* stream);
bool tile_script_list_save_with_dif(TileScriptList* list, TigFile* stream);

#endif /* ARCANUM_GAME_TILE_SCRIPT_LIST_H_ */
