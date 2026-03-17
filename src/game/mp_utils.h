#ifndef ARCANUM_GAME_MP_UTILS_H_
#define ARCANUM_GAME_MP_UTILS_H_

#include "game/anim_private.h"
#include "game/broadcast.h"
#include "game/combat.h"
#include "game/context.h"
#include "game/dialog.h"
#include "game/magictech.h"
#include "game/obj.h"
#include "game/script.h"
#include "game/target.h"
#include "game/ui.h"

typedef struct PacketGamePlayerList {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oids[8];
} PacketGamePlayerList;

// Serializeable.
static_assert(sizeof(PacketGamePlayerList) == 0xC8, "wrong size");

typedef struct PacketGameTime {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ uint64_t game_time;
    /* 0010 */ uint64_t anim_time;
} PacketGameTime;

// Serializeable.
static_assert(sizeof(PacketGameTime) == 0x18, "wrong size");

typedef struct Packet4 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t loc;
} Packet4;

// Serializeable.
static_assert(sizeof(Packet4) == 0x28, "wrong size");

typedef struct Packet5 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ DateTime field_8;
    /* 0010 */ AnimGoalData field_10;
    /* 0188 */ int64_t loc;
    /* 0190 */ int offset_x;
    /* 0194 */ int offset_y;
    /* 0198 */ AnimID field_198;
    /* 01A4 */ int field_1A4;
} Packet5;

// Serializeable.
static_assert(sizeof(Packet5) == 0x1A8, "wrong size");

typedef struct Packet6 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ tig_art_id_t art_id;
    /* 000C */ int padding_C;
    /* 0010 */ ObjectID self_oid;
    /* 0028 */ ObjectID target_oid;
    /* 0040 */ int64_t loc;
    /* 0048 */ int64_t target_loc;
    /* 0050 */ int spell;
    /* 0054 */ AnimID anim_id;
    /* 0060 */ ObjectID obj_oid;
} Packet6;

// Serializeable.
static_assert(sizeof(Packet6) == 0x78, "wrong size");

typedef struct Packet7 {
    /* 0000 */ int type;
    /* 0004 */ AnimID anim_id;
    /* 0010 */ AnimGoalData goal_data;
    /* 0188 */ int64_t loc;
    /* 0190 */ int offset_x;
    /* 0194 */ int offset_y;
} Packet7;

// Serializeable.
static_assert(sizeof(Packet7) == 0x198, "wrong size");

typedef struct Packet8 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ AGModifyData modify_data;
    /* 0038 */ int offset_x;
    /* 003C */ int offset_y;
    /* 0040 */ int64_t field_40;
    /* 0048 */ int field_48;
    /* 004C */ int field_4C;
} Packet8;

// Serializeable.
static_assert(sizeof(Packet8) == 0x50, "wrong size");

typedef struct Packet9 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int priority_level;
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ FollowerInfo field_18;
    /* 0048 */ int field_48;
    /* 004C */ int field_4C;
    /* 0050 */ int64_t loc;
    /* 0058 */ int offset_x;
    /* 005C */ int offset_y;
    /* 0060 */ int art_id;
    /* 0064 */ int field_64;
} Packet9;

static_assert(sizeof(Packet9) == 0x68, "wrong size");

typedef struct Packet10 {
    /* 0000 */ int type;
    /* 0004 */ AnimID anim_id;
    /* 0010 */ ObjectID oid;
    /* 0028 */ int64_t loc;
    /* 0030 */ int offset_x;
    /* 0034 */ int offset_y;
    /* 0038 */ tig_art_id_t art_id;
    /* 003C */ int flags;
} Packet10;

// Serializeable.
static_assert(sizeof(Packet10) == 0x40, "wrong size");

typedef struct PacketCombatModeSet {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ bool active;
    /* 0024 */ int padding_24;
} PacketCombatModeSet;

// Serializeable.
static_assert(sizeof(PacketCombatModeSet) == 0x28, "wrong size");

typedef struct PacketCombatDmg {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ CombatContext combat;
    /* 0070 */ ObjectID attacker_oid;
    /* 0088 */ ObjectID weapon_oid;
    /* 00A0 */ ObjectID target_oid;
    /* 00B8 */ ObjectID field_B8;
    /* 00D0 */ ObjectID field_D0;
} PacketCombatDmg;

// Serializeable.
static_assert(sizeof(PacketCombatDmg) == 0xE8, "wrong size");

typedef struct PacketCombatHeal {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ CombatContext combat;
    /* 0070 */ ObjectID attacker_oid;
    /* 0088 */ ObjectID weapon_oid;
    /* 00A0 */ ObjectID target_oid;
    /* 00B8 */ ObjectID field_B8;
    /* 00D0 */ ObjectID field_D0;
} PacketCombatHeal;

// Serializeable.
static_assert(sizeof(PacketCombatHeal) == 0xE8, "wrong size");

typedef struct Packet26 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int field_20;
    /* 0024 */ int padding_24;
} Packet26;

// Serializeable.
static_assert(sizeof(Packet26) == 0x28, "wrong size");

typedef struct Packet27 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t loc;
} Packet27;

static_assert(sizeof(Packet27) == 0x28, "wrong size");

typedef struct Packet28 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID critter_oid;
    /* 0038 */ int inventory_location;
    /* 003C */ int padding_3C;
} Packet28;

// Serializeable.
static_assert(sizeof(Packet28) == 0x40, "wrong size");

typedef struct Packet29 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
} Packet29;

static_assert(sizeof(Packet29) == 0x20, "wrong size");

typedef struct Packet30 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int field_38;
    /* 003C */ int padding_3C;
} Packet30;

// Serializeable.
static_assert(sizeof(Packet30) == 0x40, "wrong size");

typedef struct Packet31 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ FollowerInfo field_8;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
    /* 0040 */ TargetDescriptor td;
    /* 0050 */ ObjectID field_50;
} Packet31;

// Serializeable.
static_assert(sizeof(Packet31) == 0x68, "wrong size");

typedef struct PacketDialog {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ ObjectID field_8;
        } b;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ ObjectID field_20;
            /* 0038 */ int field_38;
            /* 003C */ int field_3C;
            /* 0040 */ int field_40;
            /* 0044 */ char field_44[1000];
            /* 042C */ int padding_42C;
        } d;
        struct {
            /* 0008 */ int field_8;
            /* 000C */ int field_C;
            /* 0010 */ int field_10;
            /* 0014 */ int field_14;
            /* 0018 */ DialogSerializedData serialized_data;
        } e;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ int field_20;
            /* 0024 */ int field_24;
        } f;
    } d;
} PacketDialog;

// Serializeable.
static_assert(sizeof(PacketDialog) == 0x430, "wrong size");

typedef struct Packet46 {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int field_18;
    /* 001C */ int field_1C;
} Packet46;

static_assert(sizeof(Packet46) == 0x20, "wrong size");

typedef struct Packet51 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ int field_20;
    /* 0024 */ int padding_24;
} Packet51;

// Serializeable.
static_assert(sizeof(Packet51) == 0x28, "wrong size");

typedef struct Packet54 {
    /* 0000 */ int type;
    /* 0004 */ int magictech_id;
} Packet54;

// Serializeable.
static_assert(sizeof(Packet54) == 0x08, "wrong size");

typedef struct PacketPlayerRequestCastSpell {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ MagicTechInvocation invocation;
} PacketPlayerRequestCastSpell;

// Serializeable.
static_assert(sizeof(PacketPlayerRequestCastSpell) == 0xE8, "wrong size");

typedef struct PacketPlayerCastSpell {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ MagicTechInvocation invocation;
} PacketPlayerCastSpell;

static_assert(sizeof(PacketPlayerCastSpell) == 0xE8, "wrong size");

typedef struct PacketPlayerInterruptSpell {
    /* 0000 */ int type;
    /* 0004 */ int mt_id;
} PacketPlayerInterruptSpell;

// Serializeable.
static_assert(sizeof(PacketPlayerInterruptSpell) == 0x08, "wrong size");

typedef struct PacketPlayerSpellMaintainAdd {
    /* 0000 */ int type;
    /* 0004 */ int mt_id;
    /* 0008 */ int player;
} PacketPlayerSpellMaintainAdd;

// Serializeable.
static_assert(sizeof(PacketPlayerSpellMaintainAdd) == 0xC, "wrong size");

typedef struct PacketPlayerSpellMaintainEnd {
    /* 0000 */ int type;
    /* 0004 */ int mt_id;
    /* 0008 */ int player;
} PacketPlayerSpellMaintainEnd;

static_assert(sizeof(PacketPlayerSpellMaintainEnd) == 0xC, "wrong size");

typedef struct Packet64 {
    /* 0000 */ int type;
    /* 0004 */ int player;
    /* 0008 */ int map;
    /* 000C */ char name[TIG_MAX_PATH];
    /* 0110 */ int field_110;
    /* 0114 */ int field_114;
    /* 0118 */ int field_118;
    /* 011C */ int field_11C;
} Packet64;

// Serializeable.
static_assert(sizeof(Packet64) == 0x120, "wrong size");

typedef struct Packet67 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
} Packet67;

static_assert(sizeof(Packet67) == 0x08, "wrong size");

typedef struct PacketNotifyPlayerLagging {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ FollowerInfo field_8;
} PacketNotifyPlayerLagging;

static_assert(sizeof(PacketNotifyPlayerLagging) == 0x38, "wrong size");

typedef struct PacketNotifyPlayerRecovered {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ FollowerInfo field_8;
} PacketNotifyPlayerRecovered;

// Serializeable.
static_assert(sizeof(PacketNotifyPlayerRecovered) == 0x38, "wrong size");

typedef struct Packet70 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ int name;
            /* 000C */ int field_C;
            /* 0010 */ ObjectID oid;
            /* 0028 */ int64_t loc;
            /* 0030 */ int field_30;
            /* 0034 */ int field_34;
            /* 0038 */ ObjectID field_38;
            /* 0050 */ int field_50;
            /* 0054 */ int field_54;
            /* 0058 */ int field_58;
            /* 005C */ int field_5C;
            /* 0060 */ int field_60;
            /* 0064 */ int field_64;
        } s0;
        struct {
            /* 0008 */ ObjectID oid;
            /* 0020 */ int64_t loc;
            /* 0028 */ int offset_x;
            /* 002C */ int offset_y;
        } s5;
    };
} Packet70;

// Serializeable.
static_assert(sizeof(Packet70) == 0x68, "wrong size");

typedef struct PacketPartyUpdate {
    int type;
    int party[8];
} PacketPartyUpdate;

// Serializeable.
static_assert(sizeof(PacketPartyUpdate) == 0x24, "wrong size");

typedef struct PacketObjectDestroy {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
} PacketObjectDestroy;

// Serializeable.
static_assert(sizeof(PacketObjectDestroy) == 0x20, "wrong size");

typedef struct PacketSummon {
    /* 0000 */ int type;
    /* 0008 */ MagicTechSummonInfo summon_info;
} PacketSummon;

// TODO: Wrong size on x64 (network only).
#if defined(_WIN32) && !defined(_WIN64)
// Serializeable.
static_assert(sizeof(PacketSummon) == 0xD8, "wrong size");
#endif

typedef struct Packet74 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    union {
        /* 0020 */ int mt_id;
        struct {
            /* 0020 */ MagicTechComponentTrait trait;
            /* 0038 */ int obj_type;
            /* 003C */ int padding_3C;
        };
    };
} Packet74;

// Serializeable.
static_assert(sizeof(Packet74) == 0x40, "wrong size");

typedef struct PacketMagicTechObjFlag {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID self_oid;
    /* 0038 */ int fld;
    /* 003C */ int value;
    /* 0040 */ int state;
    /* 0044 */ int padding_44;
    /* 0048 */ ObjectID parent_oid;
    /* 0060 */ ObjectID source_oid;
} PacketMagicTechObjFlag;

// Serializeable.
static_assert(sizeof(PacketMagicTechObjFlag) == 0x78, "wrong size");

typedef struct Packet76 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int cost;
    /* 0024 */ bool field_24;
    /* 0028 */ int magictech;
    /* 002C */ int padding_2C;
} Packet76;

// Serializeable.
static_assert(sizeof(Packet76) == 0x30, "wrong size");

typedef struct PacketMagicTechEyeCandy {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int fx_id;
    /* 0024 */ int mt_id;
} PacketMagicTechEyeCandy;

// Serializeable.
static_assert(sizeof(PacketMagicTechEyeCandy) == 0x28, "wrong size");

typedef struct Packet80 {
    /* 0000 */ int type;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID parent_oid;
    /* 0038 */ int idx;
    /* 003C */ int field_3C;
} Packet80;

typedef struct Packet81 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ ObjectID field_10;
    /* 0028 */ ObjectID field_28;
    /* 0040 */ ObjectID field_40;
    /* 0058 */ int field_58;
    /* 005C */ int field_5C;
    /* 0060 */ int field_60;
    /* 0064 */ int field_64;
    /* 0068 */ int field_68;
    /* 006C */ int field_6C;
    /* 0070 */ ObjectID field_70;
    /* 0088 */ ObjectID field_88;
    /* 00A0 */ ObjectID field_A0;
} Packet81;

// Serializeable.
static_assert(sizeof(Packet81) == 0xB8, "wrong size");

typedef struct Packet82 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
    /* 0040 */ ObjectID field_40;
} Packet82;

static_assert(sizeof(Packet82) == 0x58, "wrong size");

typedef struct Packet83 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ char field_C[128];
} Packet83;

static_assert(sizeof(Packet83) == 0x8C, "wrong size");

typedef struct Packet84 {
    /* 0000 */ int type;
    /* 0004 */ int extra_length;
    /* 0008 */ int field_8;
    /* 0010 */ UiMessage ui_message;
} Packet84;

// TODO: Wrong size on x64 (network only).
#if defined(_WIN32) && !defined(_WIN64)
// Serializeable.
static_assert(sizeof(Packet84) == 0x28, "wrong size");
#endif

typedef struct PacketTrapMarkKnown {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID pc_oid;
    /* 0020 */ ObjectID trap_oid;
    /* 0038 */ int reason;
    /* 003C */ int padding_3C;
} PacketTrapMarkKnown;

// Serializeable.
static_assert(sizeof(PacketTrapMarkKnown) == 0x40, "wrong size");

typedef struct Packet92 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ tig_art_id_t art_id;
    /* 0024 */ int padding_24;
} Packet92;

// Serializeable.
static_assert(sizeof(Packet92) == 0x28, "wrong size");

typedef struct Packet93 {
    /* 0000 */ int type;
    /* 0000 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int field_20;
    /* 0024 */ int padding_24;
} Packet93;

// Serializeable.
static_assert(sizeof(Packet93) == 0x28, "wrong size");

typedef struct PacketUpdateInven {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
} PacketUpdateInven;

// Serializeable.
static_assert(sizeof(PacketUpdateInven) == 0x20, "wrong size");

typedef struct Packet97 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
} Packet97;

// Serializeable.
static_assert(sizeof(Packet97) == 0x38, "wrong size");

typedef struct PacketMultiplayerFlagsChange {
    /* 0000 */ int type;
    /* 0004 */ int client_id;
    /* 0008 */ unsigned int flags;
} PacketMultiplayerFlagsChange;

// Serializeable.
static_assert(sizeof(PacketMultiplayerFlagsChange) == 0xC, "wrong size");

// typedef struct Packet100 {
//     /* 0000 */ int type;
//     /* 0004 */ int subtype;
//     /* 0008 */ ObjectID field_8;
//     /* 0020 */ ObjectID field_20;
//     /* 0038 */ int field_38;
//     /* 003C */ int field_3C;
// } Packet100;

// static_assert(sizeof(Packet100) == 0x40, "wrong size");

typedef struct Packet99 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int64_t location;
    /* 0028 */ int dx;
    /* 002C */ int dy;
    /* 0030 */ int field_30;
    /* 0034 */ int padding_34;
} Packet99;

// Serializeable.
static_assert(sizeof(Packet99) == 0x38, "wrong size");

typedef struct Packet100 {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ int field_8;
            /* 000C */ int field_C;
        } a;
        struct {
            /* 0008 */ ObjectID field_8;
        } b;
        struct {
            /* 0008 */ int field_8;
            /* 0010 */ ObjectID field_10;
            /* 0028 */ ObjectID field_28;
        } c;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ ObjectID field_20;
        } s;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ int field_20;
        } x;
        struct {
            /* 0008 */ ObjectID field_8;
            /* 0020 */ ObjectID field_20;
            /* 0038 */ int field_38;
            /* 003C */ int field_3C;
        } z;
    } d;
} Packet100;

// Serializeable.
static_assert(sizeof(Packet100) == 0x40, "wrong size");

typedef struct Packet104 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID oid;
} Packet104;

// Serializeable.
static_assert(sizeof(Packet104) == 0x20, "wrong size");

typedef struct PacketPlaySound {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    union {
        struct {
            /* 0008 */ int sound_id;
            /* 000C */ int loops;
            /* 0010 */ ObjectID oid;
        };
        struct {
            /* 0008 */ int music_scheme_idx;
            /* 000C */ int ambient_scheme_idx;
        };
    };
} PacketPlaySound;

// Serializeable.
static_assert(sizeof(PacketPlaySound) == 0x28, "wrong size");

typedef struct Packet118 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
} Packet118;

// Serializeable.
static_assert(sizeof(Packet118) == 0x38, "wrong size");

typedef struct Packet121 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int field_20;
    /* 0024 */ int padding_24;
} Packet121;

// Serializeable.
static_assert(sizeof(Packet121) == 0x28, "wrong size");

typedef struct Packet122 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
} Packet122;

// Serializeable.
static_assert(sizeof(Packet122) == 0x20, "wrong size");

typedef struct Packet123 {
    /* 0000 */ int type;
    /* 0004 */ int total_size;
    /* 0008 */ int player;
    /* 000C */ int rule_length;
    /* 0010 */ int name_length;
} Packet123;

// Serializeable.
static_assert(sizeof(Packet123) == 0x14, "wrong size");

typedef struct PacketPerformIdentifyService {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID npc_oid;
    /* 0038 */ ObjectID pc_oid;
    /* 0050 */ int cost;
    /* 0054 */ int padding_54;
} PacketPerformIdentifyService;

// Serializeable.
static_assert(sizeof(PacketPerformIdentifyService) == 0x58, "wrong size");

typedef struct PacketPerformRepairService {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID item_oid;
    /* 0020 */ ObjectID npc_oid;
    /* 0038 */ ObjectID pc_oid;
    /* 0050 */ int cost;
    /* 0054 */ int padding_54;
} PacketPerformRepairService;

// Serializeable.
static_assert(sizeof(PacketPerformRepairService) == 0x58, "wrong size");

typedef enum PacketChareditTraitChangeSubtype {
    PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_CACHE,
    PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC,
    PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC,
} PacketChareditTraitChangeSubtype;

typedef struct PacketChareditTraitChange {
    /* 0000 */ int type;
    /* 0004 */ int subtype;
    /* 0008 */ int trait;
    /* 000C */ int param;
} PacketChareditTraitChange;

// Serializeable.
static_assert(sizeof(PacketChareditTraitChange) == 0x10, "wrong size");

typedef struct Packet128 {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ ObjectID target_oid;
    /* 0020 */ ObjectID item_oid;
} Packet128;

// Serializeable.
static_assert(sizeof(Packet128) == 0x38, "wrong size");

typedef struct Packet130 {
    /* 0000 */ int type;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID oid;
    /* 0020 */ int fld;
    /* 0024 */ int length;
} Packet130;

// Serializeable.
static_assert(sizeof(Packet130) == 0x28, "wrong size");

bool sub_4ED6C0(int64_t obj);
bool mp_object_create(int name, int64_t loc, int64_t* obj_ptr);
void mp_object_destroy(int64_t obj);
void sub_4EDA60(UiMessage* ui_message, int player, int a3);
void mp_trap_mark_known(int64_t pc_obj, int64_t trap_obj, int reason);
void sub_4EDCE0(int64_t obj, tig_art_id_t art_id);
void mp_ui_update_inven(int64_t obj);
void sub_4EDF20(int64_t obj, int64_t location, int dx, int dy, bool a7);
void mp_item_activate(int64_t owner_obj, int64_t item_obj);
void mp_ui_written_start_type(int64_t obj, WrittenType written_type, int num);
void mp_ui_show_inven_loot(int64_t obj, int64_t a2);
void sub_4EE3A0(int64_t obj, int64_t a2);
void mp_ui_show_inven_identify(int64_t pc_obj, int64_t target_obj);
void sub_4EE4C0(int64_t obj, int64_t a2);
void sub_4EF830(int64_t a1, int64_t a2);
void sub_4EFAE0(int64_t obj, int a2);
void sub_4EFBA0(int64_t obj);
void sub_4EFC30(int64_t pc_obj, const char* name, const char* rule);
void mp_gsound_play_sfx(int sound_id);
void sub_4EED00(int64_t obj, int sound_id);
void mp_gsound_play_sfx_on_obj(int sound_id, int loops, int64_t obj);
void mp_gsound_play_scheme(int music_scheme_idx, int ambient_scheme_idx);
void sub_4F0500(int64_t obj, int fld);
void sub_4F0640(int64_t obj, ObjectID* oid_ptr);
void sub_4F0690(ObjectID oid, int64_t* obj_ptr);

#endif /* ARCANUM_GAME_MP_UTILS_H_ */
