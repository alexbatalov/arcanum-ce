#ifndef ARCANUM_GAME_FATE_H_
#define ARCANUM_GAME_FATE_H_

#include "game/context.h"

typedef enum Fate {
    FATE_FULL_HEAL,
    FATE_FORCE_GOOD_REACTION,
    FATE_CRIT_HIT,
    FATE_CRIT_MISS,
    FATE_SAVE_AGAINST_MAGICK,
    FATE_SPELL_AT_MAXIMUM,
    FATE_CRIT_SUCCESS_GAMBLING,
    FATE_CRIT_SUCCESS_HEAL,
    FATE_CRIT_SUCCESS_PICK_POCKET,
    FATE_CRIT_SUCCESS_REPAIR,
    FATE_CRIT_SUCCESS_PICK_LOCKS,
    FATE_CRIT_SUCCESS_DISARM_TRAPS,
    FATE_COUNT,
} Fate;

typedef void (*FateCallback)(int64_t obj, int fate);

bool fate_is_activated(int64_t obj, int fate);
bool fate_activate(int64_t obj, int fate);
bool fate_deactivate(int64_t obj, int fate);
bool fate_resolve(int64_t obj, int fate);
void fate_set_callback(FateCallback cb);

#endif /* ARCANUM_GAME_FATE_H_ */
