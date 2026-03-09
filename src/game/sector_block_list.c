#include "game/sector_block_list.h"

static void sub_4F83A0(void);
static void sub_4F83D0(void);

// 0x603DE4
static SectorBlockListNode* sector_block_list_head;

// 0x4F81E0
bool sector_block_list_init(SectorBlockList* list)
{
    memset(list, 0, sizeof(*list));

    return true;
}

// 0x4F8200
bool sector_block_list_reset(SectorBlockList* list)
{
    memset(list, 0, sizeof(*list));

    return true;
}

// 0x4F8230
bool sector_block_list_exit(SectorBlockList* list)
{
    memset(list, 0, sizeof(*list));

    return true;
}

// 0x4F8250
bool sector_block_list_load(SectorBlockList* list, TigFile* stream)
{
    if (tig_file_fread(list->mask, sizeof(*list->mask), 128, stream) != 128) {
        return false;
    }

    list->modified = false;

    return true;
}

// 0x4F8290
bool sector_block_list_save(SectorBlockList* list, TigFile* stream)
{
    if (tig_file_fwrite(list->mask, sizeof(*list->mask), 128, stream) != 128) {
        return false;
    }

    list->modified = false;

    return true;
}

// 0x4F82D0
bool sector_block_list_load_with_dif(SectorBlockList* list, TigFile* stream)
{
    if (!sector_block_list_load(list, stream)) {
        return false;
    }

    list->modified = true;

    return true;
}

// 0x4F8300
bool sector_block_list_save_with_dif(SectorBlockList* list, TigFile* stream)
{
    if (!sector_block_list_save(list, stream)) {
        return false;
    }

    list->modified = true;

    return true;
}

// 0x4F8330
void sub_4F8330(void)
{
}

// 0x4F8340
void sub_4F8340(void)
{
    sub_4F83D0();
}

// 0x4F8350
SectorBlockListNode* sector_block_list_node_create(void)
{
    SectorBlockListNode* node;

    if (sector_block_list_head == NULL) {
        sub_4F83A0();
    }

    node = sector_block_list_head;
    sector_block_list_head = node->next;

    node->next = NULL;

    return node;
}

// 0x4F8380
void sector_block_list_node_destroy(SectorBlockListNode* node)
{
    node->next = sector_block_list_head;
    sector_block_list_head = node;
}

// 0x4F83A0
void sub_4F83A0(void)
{
    int index;
    SectorBlockListNode* node;

    if (sector_block_list_head == NULL) {
        for (index = 0; index < 64; index++) {
            node = (SectorBlockListNode*)MALLOC(sizeof(*node));
            node->next = sector_block_list_head;
            sector_block_list_head = node;
        }
    }
}

// 0x4F83D0
void sub_4F83D0(void)
{
    SectorBlockListNode* next;

    while (sector_block_list_head != NULL) {
        next = sector_block_list_head->next;
        FREE(sector_block_list_head);
        sector_block_list_head = next;
    }
}
