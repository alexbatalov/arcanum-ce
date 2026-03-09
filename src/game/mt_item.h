#ifndef ARCANUM_GAME_MT_ITEM_H_
#define ARCANUM_GAME_MT_ITEM_H_

#include "game/combat.h"
#include "game/context.h"

#define MT_ITEM_TRIG_COUNT 26

// clang-format off
#define MT_ITEM_TRIG_NONE                           0x00000000u
#define MT_ITEM_TRIG_RANDOM_CHANCE_FREQUENT         0x00000010u
#define MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON_MELEE   0x00000100u
#define MT_ITEM_TRIG_RANDOM_CHANCE_RARE             0x00000200u
#define MT_ITEM_TRIG_RANDOM_CHANCE_UNCOMMON         0x00000400u
#define MT_ITEM_TRIG_RANDOM_CHANCE_COMMON           0x00000800u
#define MT_ITEM_TRIG_ITEM_USED                      0x00001000u
#define MT_ITEM_TRIG_TARGET_ATTACKER                0x00002000u
#define MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON         0x00004000u
#define MT_ITEM_TRIG_TARGET_ATTACKER_ARMOR          0x00008000u
#define MT_ITEM_TRIG_PARENT_DMGS_OPPONENT           0x00010000u
#define MT_ITEM_TRIG_PARENT_DMGS_OPPONENT_W_ITEM    0x00020000u
#define MT_ITEM_TRIG_TARGET_HIT                     0x00040000u
#define MT_ITEM_TRIG_TARGET_GOING_UNCONSCIOUS       0x00080000u
#define MT_ITEM_TRIG_PARENT_DYING                   0x00100000u
#define MT_ITEM_TRIG_PARENT_HIT_BY_ATK_SPELL        0x00200000u
#define MT_ITEM_TRIG_PARENT_ATKS_OPPONENT           0x00400000u
#define MT_ITEM_TRIG_PARENT_ATKS_LOCATION           0x00800000u
#define MT_ITEM_TRIG_DROP                           0x01000000u
#define MT_ITEM_TRIG_PARENT_HIT                     0x02000000u
#define MT_ITEM_TRIG_PARENT_STUNNED                 0x04000000u
#define MT_ITEM_TRIG_PARENT_GOING_UNCONSCIOUS       0x08000000u
#define MT_ITEM_TRIG_USER_ACTIVATE                  0x10000000u
#define MT_ITEM_TRIG_WEAR                           0x20000000u
#define MT_ITEM_TRIG_UNWEAR                         0x40000000u
#define MT_ITEM_TRIG_PICKUP                         0x80000000u
// clang-format on

#define MT_ITEM_TRIG_TARGET_ATTACKER_ANY (MT_ITEM_TRIG_TARGET_ATTACKER \
    | MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON                              \
    | MT_ITEM_TRIG_TARGET_ATTACKER_ARMOR                               \
    | MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON_MELEE)

#define MT_ITEM_TRIG_RANDOM_CHANCE_ANY (MT_ITEM_TRIG_RANDOM_CHANCE_RARE \
    | MT_ITEM_TRIG_RANDOM_CHANCE_UNCOMMON                               \
    | MT_ITEM_TRIG_RANDOM_CHANCE_COMMON                                 \
    | MT_ITEM_TRIG_RANDOM_CHANCE_FREQUENT)

extern const char* mt_item_trig_keys[MT_ITEM_TRIG_COUNT];
extern unsigned int mt_item_trig_values[MT_ITEM_TRIG_COUNT];

bool mt_item_init(GameInitInfo* init_info);
void mt_item_exit(void);
unsigned int mt_item_triggers(int spell);
int mt_item_spell_to_magictech_spell(int spell);
int mt_item_spell(int64_t item_obj, int slot);
void mt_item_notify_wear(int64_t item_obj, int64_t parent_obj);
void mt_item_notify_pickup(int64_t item_obj, int64_t parent_obj);
void mt_item_notify_parent_hit(int64_t attacker_obj, CombatContext* combat, int64_t target_obj);
void mt_item_notify_parent_stunned(int64_t attacker_obj, int64_t target_obj);
void mt_item_notify_parent_going_unconscious(int64_t attacker_obj, int64_t target_obj);
void mt_item_notify_parent_dying(int64_t attacker_obj, int64_t target_obj);
void mt_item_notify_parent_attacks_obj(int64_t attacker_obj, int64_t target_obj);
void mt_item_notify_parent_attacks_loc(int64_t attacker_obj, int64_t weapon_obj, int64_t target_loc);
void mt_item_notify_target_going_unconscious(int64_t attacker_obj, int64_t target_obj);
void mt_item_notify_parent_dmgs_obj(int64_t attacker_obj, int64_t weapon_obj, int64_t target_obj);
void mt_item_notify_used(int64_t item_obj, int64_t target_obj);
void mt_item_notify_unwear(int64_t item_obj, int64_t parent_obj);
void mt_item_notify_drop(int64_t item_obj, int64_t parent_obj);
bool mt_item_valid_ai_action(int64_t item_obj);

#endif /* ARCANUM_GAME_MT_ITEM_H_ */
