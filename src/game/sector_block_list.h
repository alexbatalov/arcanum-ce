#ifndef ARCANUM_GAME_SECTOR_BLOCK_LIST_H_
#define ARCANUM_GAME_SECTOR_BLOCK_LIST_H_

#include <tig/tig.h>

typedef struct SectorBlockList {
    /* 0000 */ bool modified;
    /* 0004 */ uint32_t mask[128];
} SectorBlockList;

typedef struct SectorBlockListNode {
    /* 0000 */ void* data;
    /* 0004 */ struct SectorBlockListNode* next;
} SectorBlockListNode;

bool sector_block_list_init(SectorBlockList* list);
bool sector_block_list_reset(SectorBlockList* list);
bool sector_block_list_exit(SectorBlockList* list);
bool sector_block_list_load(SectorBlockList* list, TigFile* stream);
bool sector_block_list_save(SectorBlockList* list, TigFile* stream);
bool sector_block_list_load_with_dif(SectorBlockList* list, TigFile* stream);
bool sector_block_list_save_with_dif(SectorBlockList* list, TigFile* stream);
void sub_4F8330(void);
void sub_4F8340(void);
SectorBlockListNode* sector_block_list_node_create(void);
void sector_block_list_node_destroy(SectorBlockListNode* node);

#endif /* ARCANUM_GAME_SECTOR_BLOCK_LIST_H_ */
