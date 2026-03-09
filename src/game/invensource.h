#ifndef ARCANUM_GAME_INVENSOURCE_H_
#define ARCANUM_GAME_INVENSOURCE_H_

#include "game/context.h"

#define INVEN_SOURCE_SET_SIZE 300

typedef struct InvenSourceSet {
    /* 0000 */ int buy_all;
    /* 0004 */ int cnt;
    /* 0008 */ int buy_cnt;
    /* 000C */ int min_coins;
    /* 0010 */ int max_coins;
    /* 0014 */ int rate[INVEN_SOURCE_SET_SIZE];
    /* 04C4 */ int basic_prototype[INVEN_SOURCE_SET_SIZE];
    /* 0974 */ int buy_basic_prototype[INVEN_SOURCE_SET_SIZE];
} InvenSourceSet;

bool invensource_init(GameInitInfo* init_info);
void invensource_exit(void);
void invensource_get_id_list(int id, InvenSourceSet* set);

#endif /* ARCANUM_GAME_INVENSOURCE_H_ */
