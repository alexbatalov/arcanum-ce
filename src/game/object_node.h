#ifndef ARCANUM_GAME_OBJECT_NODE_H_
#define ARCANUM_GAME_OBJECT_NODE_H_

#include "game/context.h"

typedef struct ObjectNode {
    /* 0000 */ int64_t obj;
    /* 0004 */ struct ObjectNode* next;
} ObjectNode;

bool object_node_init(GameInitInfo* init_info);
void object_node_exit(void);
ObjectNode* object_node_create(void);
void object_node_destroy(ObjectNode* node);

#endif /* ARCANUM_GAME_OBJECT_NODE_H_ */
