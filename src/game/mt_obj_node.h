#ifndef ARCANUM_GAME_MT_OBJ_NODE_H_
#define ARCANUM_GAME_MT_OBJ_NODE_H_

#include "game/context.h"
#include "game/object.h"

typedef struct MagicTechObjectNode {
    /* 0000 */ int64_t obj;
    /* 0008 */ Ryan field_8;
    /* 0030 */ int field_30;
    /* 0034 */ int aptitude;
    /* 0038 */ int type;
    union {
        /* 0040 */ struct MagicTechObjectNode* next;
        /* 0040 */ int64_t loc;
    };
} MagicTechObjectNode;

bool mt_obj_node_init(GameInitInfo* init_info);
void mt_obj_node_exit(void);
MagicTechObjectNode* mt_obj_node_create(void);
void mt_obj_node_destroy(MagicTechObjectNode* node);
bool mt_obj_node_save_list(MagicTechObjectNode** head_ptr, TigFile* stream);
bool mt_obj_node_load_list(MagicTechObjectNode** head_ptr, TigFile* stream);
bool mt_obj_node_save_detached(MagicTechObjectNode* node, TigFile* stream);
bool mt_obj_node_load_detached(MagicTechObjectNode* node, TigFile* stream);

#endif /* ARCANUM_GAME_MT_OBJ_NODE_H_ */
