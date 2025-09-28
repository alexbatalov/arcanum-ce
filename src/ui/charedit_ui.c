#include "ui/charedit_ui.h"

#include <stdio.h>

#include "game/anim.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/effect.h"
#include "game/gamelib.h"
#include "game/hrp.h"
#include "game/level.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/object.h"
#include "game/player.h"
#include "game/portrait.h"
#include "game/resistance.h"
#include "game/skill.h"
#include "game/snd.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/tech.h"
#include "game/ui.h"
#include "ui/intgame.h"
#include "ui/schematic_ui.h"
#include "ui/scrollbar_ui.h"
#include "ui/skill_ui.h"
#include "ui/spell_ui.h"
#include "ui/tech_ui.h"

typedef enum ChareditStat {
    CHAREDIT_STAT_UNSPENT_POINTS,
    CHAREDIT_STAT_LEVEL,
    CHAREDIT_STAT_XP_TO_NEXT_LEVEL,
    CHAREDIT_STAT_HP_PTS_CURRENT,
    CHAREDIT_STAT_HP_PTS_MAXIMUM,
    CHAREDIT_STAT_FATIGUE_PTS_CURRENT,
    CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM,
    CHAREDIT_STAT_STRENGTH_BASE,
    CHAREDIT_STAT_STRENGTH_LEVEL,
    CHAREDIT_STAT_CONSTITUTION_BASE,
    CHAREDIT_STAT_CONSTITUTION_LEVEL,
    CHAREDIT_STAT_DEXTERITY_BASE,
    CHAREDIT_STAT_DEXTERITY_LEVEL,
    CHAREDIT_STAT_BEAUTY_BASE,
    CHAREDIT_STAT_BEAUTY_LEVEL,
    CHAREDIT_STAT_INTELLIGENCE_BASE,
    CHAREDIT_STAT_INTELLIGENCE_LEVEL,
    CHAREDIT_STAT_WILLPOWER_BASE,
    CHAREDIT_STAT_WILLPOWER_LEVEL,
    CHAREDIT_STAT_PERCEPTION_BASE,
    CHAREDIT_STAT_PERCEPTION_LEVEL,
    CHAREDIT_STAT_CHARISMA_BASE,
    CHAREDIT_STAT_CHARISMA_LEVEL,
    CHAREDIT_STAT_COUNT,
} ChareditStat;

typedef enum ChareditSkillGroup {
    CHAREDIT_SKILL_GROUP_COMBAT,
    CHAREDIT_SKILL_GROUP_THIEVING,
    CHAREDIT_SKILL_GROUP_SOCIAL,
    CHAREDIT_SKILL_GROUP_TECHNOLOGICAL,
    CHAREDIT_SKILL_GROUP_COUNT,
} ChareditSkillGroup;

#define CHAREDIT_SKILLS_PER_GROUP 4

typedef enum ChareditError {
    CHAREDIT_ERR_STAT_AT_MAX, // "Stat has reached its maximum value."
    CHAREDIT_ERR_NOT_ENOUGH_CHARACTER_POINTS, // "You haven't enough character points."
    CHAREDIT_ERR_STAT_AT_ACCEPTED_LEVEL, // "You cannot reduce this stat below its last accepted level."
    CHAREDIT_ERR_STAT_AT_MIN, // "Stat has reached its minimum value."
    CHAREDIT_ERR_SKILL_AT_MAX, // "Skill has reached its maximum value."
    CHAREDIT_ERR_NOT_ENOUGH_STATS, // "You must raise your stats first."
    CHAREDIT_ERR_SKILL_AT_ACCEPTED_LEVEL, // "You cannot reduce this skill below its last accepted level."
    CHAREDIT_ERR_NO_POINTS_IN_SKILL, // "You have no points in this skill."
    CHAREDIT_ERR_SKILL_AT_MIN, // "Skill has reached its minimum value."
    CHAREDIT_ERR_NOT_ENOUGH_INTELLIGENCE, // "You must raise your Intelligence first."
    CHAREDIT_ERR_SPELL_AT_ACCEPTED_LEVEL, // "You cannot sell back an accepted spell."
    CHAREDIT_ERR_STAT_SKILL_PREREQUISITE, // "The stat cannot be lowered because the current level is a prerequisite for a skill."
    CHAREDIT_ERR_INTELLIGENCE_SPELL_PREREQUISITE, // "Your Intelligence cannot be lowered because the current level is a prerequisite for a spell."
    CHAREDIT_ERR_NOT_ENOUGH_LEVEL, // "You are not high enough level to buy this spell."
    CHAREDIT_ERR_INTELLIGENCE_TECH_PREREQUISITE, // "Your Intelligence cannot be lowered because the current level is a prerequisite for a tech discipline."
    CHAREDIT_ERR_NOT_ENOUGH_STRENGTH, // "You must raise your Strength first."
    CHAREDIT_ERR_NOT_ENOUGH_DEXTERITY, // "You must raise your Dexterity first."
    CHAREDIT_ERR_NOT_ENOUGH_CONSTITUTION, // "You must raise your Constitution first."
    CHAREDIT_ERR_NOT_ENOUGH_BEAUTY, // "You must raise your Beauty first."
    CHAREDIT_ERR_NOT_ENOUGH_WILLPOWER, // "You must raise your Willpower first."
    CHAREDIT_ERR_NOT_ENOUGH_PERCEPTION, // "You must raise your Perception first."
    CHAREDIT_ERR_NOT_ENOUGH_CHARISMA, // "You must raise your Charisma first."
    CHAREDIT_ERR_WILLPOWER_SPELL_PREREQUISITE, // "Your Willpower cannot be lowered because the current level is a prerequisite for a spell."
    CHAREDIT_ERR_COUNT,
} ChareditError;

typedef enum MpChareditTrait {
    MP_CHAREDIT_TRAIT_STAT,
    MP_CHAREDIT_TRAIT_BASIC_SKILL,
    MP_CHAREDIT_TRAIT_TECH_SKILL,
    MP_CHAREDIT_TRAIT_DEGREE,
    MP_CHAREDIT_TRAIT_COLLEGE,
} MpChareditTrait;

typedef struct S5C8150 {
    const char* str;
    int x;
    int y;
    int value;
} S5C8150;

typedef struct S5C87D0 {
    int x;
    int y;
    tig_button_handle_t button_handle;
    int art_num;
} S5C87D0;

typedef struct S5C8CA8 {
    int x;
    int y;
    int width;
    int height;
    int field_10;
    tig_button_handle_t button_handle;
} S5C8CA8;

static void charedit_refresh_basic_info();
static bool charedit_window_message_filter(TigMessage* msg);
static void charedit_show_hint(int hint);
static void charedit_cycle_obj(bool next);
static void charedit_refresh_internal();
static void charedit_refresh_secondary_stats();
static void charedit_refresh_stats();
static void charedit_refresh_stat(int stat);
static int charedit_stat_map_to_critter_stat(int stat);
static int charedit_stat_value_get(int64_t obj, int stat);
static void charedit_stat_value_set(int64_t obj, int stat, int value);
static void sub_55B880(tig_window_handle_t window_handle, tig_font_handle_t font, S5C8150* a3, const char** list, int a5, int a6);
static bool charedit_create_skills_win();
static void sub_55BD10(int group);
static void charedit_refresh_skills_win();
static bool charedit_create_tech_win();
static void charedit_draw_tech_degree_icon(int index);
static void charedit_refresh_tech_win();
static bool charedit_create_spells_win();
static void sub_55CA70(int a1, int a2);
static void charedit_refresh_spells_win();
static bool charedit_scheme_win_create();
static void charedit_scheme_win_refresh();
static bool charedit_skills_win_message_filter(TigMessage* msg);
static bool sub_55D6F0(TigMessage* msg);
static bool charedit_tech_win_message_filter(TigMessage* msg);
static bool charedit_spells_win_message_filter(TigMessage* msg);
static bool charedit_scheme_win_message_filter(TigMessage* msg);
static bool charedit_labels_init();
static void charedit_labels_exit();
static void charedit_refresh_alignment_aptitude_bars();
static void sub_55EFB0();
static void sub_55EFE0();
static void sub_55EFF0();
static void sub_55F0D0();
static void charedit_scheme_scrollbar_value_changed(int value);
static void charedit_scheme_scrollbar_refresh_rect(TigRect* rect);

// 0x5C7E70
static struct {
    int x;
    int y;
    int field_8;
} stru_5C7E70[CHAREDIT_STAT_COUNT] = {
    { 337, 94, 3 },
    { 337, 75, 2 },
    { 337, 113, 6 },
    { 0, 0, 3 },
    { -447, 184, 3 },
    { 0, 0, 3 },
    { -447, 254, 3 },
    { 0, 0, 2 },
    { -211, 156, 2 },
    { 0, 0, 2 },
    { -211, 196, 2 },
    { 0, 0, 2 },
    { -211, 236, 2 },
    { 0, 0, 2 },
    { -211, 276, 2 },
    { 0, 0, 2 },
    { -350, 156, 2 },
    { 0, 0, 2 },
    { -350, 196, 2 },
    { 0, 0, 2 },
    { -350, 236, 2 },
    { 0, 0, 2 },
    { -350, 276, 2 },
};

// 0x5C7F88
static S5C87D0 stru_5C7F88[10] = {
    { 465, 144, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_HP_PTS_MAXIMUM },
    { 465, 214, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM },
    { 224, 117, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_STRENGTH_BASE },
    { 224, 157, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_CONSTITUTION_BASE },
    { 224, 197, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_DEXTERITY_BASE },
    { 224, 237, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_BEAUTY_BASE },
    { 367, 117, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_INTELLIGENCE_BASE },
    { 367, 157, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_WILLPOWER_BASE },
    { 367, 197, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_PERCEPTION_BASE },
    { 367, 237, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_CHARISMA_BASE },
};

// 0x5C8028
static S5C87D0 stru_5C8028[10] = {
    { 409, 144, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_HP_PTS_MAXIMUM },
    { 409, 214, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM },
    { 170, 117, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_STRENGTH_BASE },
    { 170, 157, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_CONSTITUTION_BASE },
    { 170, 197, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_DEXTERITY_BASE },
    { 170, 237, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_BEAUTY_BASE },
    { 313, 117, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_INTELLIGENCE_BASE },
    { 313, 157, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_WILLPOWER_BASE },
    { 313, 197, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_PERCEPTION_BASE },
    { 313, 237, TIG_BUTTON_HANDLE_INVALID, CHAREDIT_STAT_CHARISMA_BASE },
};

// 0x5C80C8
static int dword_5C80C8[] = {
    106,
    107,
    109,
    110,
    111,
    112,
    113,
    114,
    115,
    116,
};

// 0x5C8124
static int dword_5C8124[] = {
    -1,
    -1,
    STAT_STRENGTH,
    STAT_CONSTITUTION,
    STAT_DEXTERITY,
    STAT_BEAUTY,
    STAT_INTELLIGENCE,
    STAT_WILLPOWER,
    STAT_PERCEPTION,
    STAT_CHARISMA,
};

// 0x5C8150
static S5C8150 stru_5C8150[] = {
    { NULL, 212, 94, 0 },
    { NULL, 212, 75, 0 },
    { NULL, 212, 113, 0 },
    { NULL, 212, 56, 0 },
    { NULL, 337, 75, 0 },
    { NULL, 337, 94, 0 },
    { NULL, 337, 113, 0 },
    { NULL, -178, 323, 0 },
    { NULL, -383, 323, 0 },
};

// 0x5C81E0
static S5C8150 stru_5C81E0[] = {
    { NULL, 49, 349, STAT_CARRY_WEIGHT },
    { NULL, 49, 367, STAT_DAMAGE_BONUS },
    { NULL, 49, 385, STAT_AC_ADJUSTMENT },
    { NULL, 49, 403, STAT_SPEED },
    { NULL, 178, 349, STAT_HEAL_RATE },
    { NULL, 178, 367, STAT_POISON_RECOVERY },
    { NULL, 178, 385, STAT_REACTION_MODIFIER },
    { NULL, 178, 403, STAT_MAX_FOLLOWERS },
    { NULL, 322, 342, RESISTANCE_TYPE_NORMAL },
    { NULL, 322, 358, RESISTANCE_TYPE_MAGIC },
    { NULL, 322, 374, RESISTANCE_TYPE_FIRE },
    { NULL, 322, 390, RESISTANCE_TYPE_POISON },
    { NULL, 322, 406, RESISTANCE_TYPE_ELECTRICAL },
};

// 0x5C82B0
static S5C87D0 charedit_skill_group_buttons[CHAREDIT_SKILL_GROUP_COUNT] = {
    { 21, 17, TIG_BUTTON_HANDLE_INVALID, 302 },
    { 75, 17, TIG_BUTTON_HANDLE_INVALID, 303 },
    { 129, 17, TIG_BUTTON_HANDLE_INVALID, 304 },
    { 185, 17, TIG_BUTTON_HANDLE_INVALID, 305 },
};

// 0x5C82F0
static S5C8150 stru_5C82F0[SKILL_COUNT] = {
    { NULL, 520, 167, BASIC_SKILL_BOW },
    { NULL, 520, 233, BASIC_SKILL_DODGE },
    { NULL, 520, 299, BASIC_SKILL_MELEE },
    { NULL, 520, 365, BASIC_SKILL_THROWING },
    { NULL, 520, 167, BASIC_SKILL_BACKSTAB },
    { NULL, 520, 233, BASIC_SKILL_PICK_POCKET },
    { NULL, 520, 299, BASIC_SKILL_PROWLING },
    { NULL, 520, 365, BASIC_SKILL_SPOT_TRAP },
    { NULL, 520, 167, BASIC_SKILL_GAMBLING },
    { NULL, 520, 233, BASIC_SKILL_HAGGLE },
    { NULL, 520, 299, BASIC_SKILL_HEAL },
    { NULL, 520, 365, BASIC_SKILL_PERSUATION },
    { NULL, 520, 167, TECH_SKILL_REPAIR },
    { NULL, 520, 233, TECH_SKILL_FIREARMS },
    { NULL, 520, 299, TECH_SKILL_PICK_LOCKS },
    { NULL, 520, 365, TECH_SKILL_DISARM_TRAPS },
};

// 0x5C83F0
static S5C8150 stru_5C83F0[4] = {
    { NULL, 517, 206, -1 },
    { NULL, 517, 261, -1 },
    { NULL, 517, 316, -1 },
    { NULL, 517, 371, -1 },
};

// 0x5C8430
static S5C87D0 charedit_skills_plus_buttons[SKILL_COUNT] = {
    { 192, 84, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_BOW },
    { 192, 150, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_DODGE },
    { 192, 216, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_MELEE },
    { 192, 282, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_THROWING },
    { 192, 84, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_BACKSTAB },
    { 192, 150, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_PICK_POCKET },
    { 192, 216, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_PROWLING },
    { 192, 282, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_SPOT_TRAP },
    { 192, 84, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_GAMBLING },
    { 192, 150, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_HAGGLE },
    { 192, 216, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_HEAL },
    { 192, 282, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_PERSUATION },
    { 192, 84, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_REPAIR },
    { 192, 150, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_FIREARMS },
    { 192, 216, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_PICK_LOCKS },
    { 192, 282, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_DISARM_TRAPS },
};

// 0x5C8530
static S5C87D0 charedit_skills_minus_buttons[SKILL_COUNT] = {
    { 18, 84, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_BOW },
    { 18, 150, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_DODGE },
    { 18, 216, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_MELEE },
    { 18, 282, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_THROWING },
    { 18, 84, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_BACKSTAB },
    { 18, 150, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_PICK_POCKET },
    { 18, 216, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_PROWLING },
    { 18, 282, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_SPOT_TRAP },
    { 18, 84, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_GAMBLING },
    { 18, 150, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_HAGGLE },
    { 18, 216, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_HEAL },
    { 18, 282, TIG_BUTTON_HANDLE_INVALID, BASIC_SKILL_PERSUATION },
    { 18, 84, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_REPAIR },
    { 18, 150, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_FIREARMS },
    { 18, 216, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_PICK_LOCKS },
    { 18, 282, TIG_BUTTON_HANDLE_INVALID, TECH_SKILL_DISARM_TRAPS },
};

// 0x5C8630
static S5C87D0 charedit_college_buttons[COLLEGE_COUNT] = {
    { 516, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_CONVEYANCE },
    { 541, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_DIVINATION },
    { 566, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_AIR },
    { 591, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_EARTH },
    { 616, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_FIRE },
    { 641, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_WATER },
    { 665, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_FORCE },
    { 690, 119, TIG_BUTTON_HANDLE_INVALID, COLLEGE_MENTAL },
    { 528, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_META },
    { 553, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_MORPH },
    { 578, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_NATURE },
    { 603, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_NECROMANTIC_BLACK },
    { 627, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_NECROMANTIC_WHITE },
    { 652, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_PHANTASM },
    { 677, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_SUMMONING },
    { 702, 144, TIG_BUTTON_HANDLE_INVALID, COLLEGE_TEMPORAL },
};

// 0x5C8730
static S5C8150 charedit_spell_title_labels[5] = {
    { NULL, 562, 180, -1 },
    { NULL, 562, 228, -1 },
    { NULL, 562, 275, -1 },
    { NULL, 562, 323, -1 },
    { NULL, 562, 370, -1 },
};

// 0x5C8780
static S5C8150 charedit_spell_minimum_level_labels[5] = {
    { NULL, 562, 201, -1 },
    { NULL, 562, 249, -1 },
    { NULL, 562, 296, -1 },
    { NULL, 562, 344, -1 },
    { NULL, 562, 391, -1 },
};

// 0x5C87D0
static S5C87D0 charedit_tech_buttons[TECH_COUNT] = {
    /*    TECH_HERBOLOGY */ { 8, 7, TIG_BUTTON_HANDLE_INVALID, TECH_HERBOLOGY },
    /*    TECH_CHEMISTRY */ { 34, 29, TIG_BUTTON_HANDLE_INVALID, TECH_CHEMISTRY },
    /*     TECH_ELECTRIC */ { 61, 7, TIG_BUTTON_HANDLE_INVALID, TECH_ELECTRIC },
    /*   TECH_EXPLOSIVES */ { 88, 28, TIG_BUTTON_HANDLE_INVALID, TECH_EXPLOSIVES },
    /*          TECH_GUN */ { 116, 7, TIG_BUTTON_HANDLE_INVALID, TECH_GUN },
    /*   TECH_MECHANICAL */ { 142, 29, TIG_BUTTON_HANDLE_INVALID, TECH_MECHANICAL },
    /*       TECH_SMITHY */ { 171, 7, TIG_BUTTON_HANDLE_INVALID, TECH_SMITHY },
    /* TECH_THERAPEUTICS */ { 198, 29, TIG_BUTTON_HANDLE_INVALID, TECH_THERAPEUTICS },
};

// 0x5C8850
static S5C8150 stru_5C8850[7] = {
    { NULL, 549, 198, -1 },
    { NULL, 549, 231, -1 },
    { NULL, 549, 264, -1 },
    { NULL, 549, 297, -1 },
    { NULL, 549, 330, -1 },
    { NULL, 549, 363, -1 },
    { NULL, 549, 396, -1 },
};

// 0x5C88C0
static S5C8150 stru_5C88C0[7] = {
    { NULL, 549, 213, -1 },
    { NULL, 549, 246, -1 },
    { NULL, 549, 279, -1 },
    { NULL, 549, 312, -1 },
    { NULL, 549, 345, -1 },
    { NULL, 549, 378, -1 },
    { NULL, 549, 411, -1 },
};

// 0x5C8930
static TigRect stru_5C8930 = { 12, 10, 89, 89 };

// 0x5C8940
static S5C8150 stru_5C8940[3] = {
    { NULL, -57, 239, 0 },
    { NULL, -767, 107, 0 },
    { NULL, -767, 406, 0 },
};

// 0x5C8970
static TigRect stru_5C8970 = { 756, 68, 30, 16 };

// 0x5C8980
static TigRect stru_5C8980 = { 756, 367, 30, 16 };

// 0x5C8E40
static S5C8150 stru_5C8E40 = { 0, -620, 125, 0 };

// 0x5C8E50
static S5C8150 stru_5C8E50[15] = {
    { NULL, 526, 170, 0 },
    { NULL, 526, 186, 0 },
    { NULL, 526, 202, 0 },
    { NULL, 526, 218, 0 },
    { NULL, 526, 234, 0 },
    { NULL, 526, 250, 0 },
    { NULL, 526, 266, 0 },
    { NULL, 526, 282, 0 },
    { NULL, 526, 298, 0 },
    { NULL, 526, 314, 0 },
    { NULL, 526, 330, 0 },
    { NULL, 526, 346, 0 },
    { NULL, 526, 362, 0 },
    { NULL, 526, 378, 0 },
    { NULL, 526, 394, 0 },
};

// 0x5C8990
static UiMessage charedit_error_msg = { UI_MSG_TYPE_EXCLAMATION, 0, 0, 0, 0 };

static S5C8CA8 stru_5C89A8[32] = {
    { 141, 21, 66, 66, 0x64, TIG_BUTTON_HANDLE_INVALID },
    { 212, 15, 243, 75, 0x65, TIG_BUTTON_HANDLE_INVALID },
    { 15, 138, 85, 85, 0x66, TIG_BUTTON_HANDLE_INVALID },
    { 337, 34, 123, 18, 0x67, TIG_BUTTON_HANDLE_INVALID },
    { 337, 34, 123, 18, 0x68, TIG_BUTTON_HANDLE_INVALID },
    { 337, 72, 123, 18, 0x69, TIG_BUTTON_HANDLE_INVALID },
    { 410, 134, 74, 37, 0x6A, TIG_BUTTON_HANDLE_INVALID },
    { 410, 204, 74, 37, 0x6B, TIG_BUTTON_HANDLE_INVALID },
    { 751, 88, 38, 274, 0x6C, TIG_BUTTON_HANDLE_INVALID },
    { 124, 110, 126, 31, 0x6D, TIG_BUTTON_HANDLE_INVALID },
    { 124, 150, 126, 31, 0x6E, TIG_BUTTON_HANDLE_INVALID },
    { 124, 190, 126, 31, 0x6F, TIG_BUTTON_HANDLE_INVALID },
    { 124, 230, 126, 31, 0x70, TIG_BUTTON_HANDLE_INVALID },
    { 266, 110, 126, 31, 0x71, TIG_BUTTON_HANDLE_INVALID },
    { 266, 150, 126, 31, 0x72, TIG_BUTTON_HANDLE_INVALID },
    { 266, 190, 126, 31, 0x73, TIG_BUTTON_HANDLE_INVALID },
    { 266, 230, 126, 31, 0x74, TIG_BUTTON_HANDLE_INVALID },
    { 51, 282, 253, 18, 0x75, TIG_BUTTON_HANDLE_INVALID },
    { 49, 308, 125, 14, 0x76, TIG_BUTTON_HANDLE_INVALID },
    { 49, 326, 125, 14, 0x77, TIG_BUTTON_HANDLE_INVALID },
    { 49, 344, 125, 14, 0x78, TIG_BUTTON_HANDLE_INVALID },
    { 49, 362, 125, 14, 0x79, TIG_BUTTON_HANDLE_INVALID },
    { 178, 308, 125, 14, 0x7A, TIG_BUTTON_HANDLE_INVALID },
    { 178, 326, 125, 14, 0x7B, TIG_BUTTON_HANDLE_INVALID },
    { 178, 344, 125, 14, 0x7C, TIG_BUTTON_HANDLE_INVALID },
    { 178, 362, 125, 14, 0x7D, TIG_BUTTON_HANDLE_INVALID },
    { 315, 282, 136, 18, 0x7E, TIG_BUTTON_HANDLE_INVALID },
    { 322, 301, 105, 14, 0x7F, TIG_BUTTON_HANDLE_INVALID },
    { 322, 317, 105, 14, 0x80, TIG_BUTTON_HANDLE_INVALID },
    { 322, 333, 105, 14, 0x81, TIG_BUTTON_HANDLE_INVALID },
    { 322, 349, 105, 14, 0x82, TIG_BUTTON_HANDLE_INVALID },
    { 322, 365, 105, 14, 0x83, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C8CA8
static S5C8CA8 charedit_skills_hover_areas[CHAREDIT_SKILLS_PER_GROUP] = {
    { 520, 126, 200, 60, 0, TIG_BUTTON_HANDLE_INVALID },
    { 520, 192, 200, 60, 0, TIG_BUTTON_HANDLE_INVALID },
    { 520, 258, 200, 60, 0, TIG_BUTTON_HANDLE_INVALID },
    { 520, 324, 200, 60, 0, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C8D08
static S5C8CA8 charedit_tech_degree_buttons[DEGREE_COUNT] = {
    /*     DEGREE_LAYMAN */ { 512, 129, 219, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /*     DEGREE_NOVICE */ { 535, 162, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /*  DEGREE_ASSISTANT */ { 535, 194, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /*  DEGREE_ASSOCIATE */ { 535, 226, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /* DEGREE_TECHNICIAN */ { 535, 258, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /*   DEGREE_ENGINEER */ { 535, 290, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /*  DEGREE_PROFESSOR */ { 535, 322, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
    /*  DEGREE_DOCTORATE */ { 535, 354, 177, 27, 0, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C8DC8
static S5C8CA8 stru_5C8DC8[5] = {
    { 521, 142, 196, 35, 0, TIG_BUTTON_HANDLE_INVALID },
    { 521, 190, 196, 35, 0, TIG_BUTTON_HANDLE_INVALID },
    { 521, 237, 196, 35, 0, TIG_BUTTON_HANDLE_INVALID },
    { 521, 285, 196, 35, 0, TIG_BUTTON_HANDLE_INVALID },
    { 521, 332, 196, 35, 0, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C8F40
static TigRect stru_5C8F40 = { 209, 60, 17, 255 };

// 0x5C8F50
static S5C8150 stru_5C8F50 = { NULL, 0, 0, -1 };

// 0x5C8F70
static int charedit_tech_degree_icons_x[DEGREE_COUNT - 1] = {
    /*     DEGREE_NOVICE */ 8,
    /*  DEGREE_ASSISTANT */ 8,
    /*  DEGREE_ASSOCIATE */ 8,
    /* DEGREE_TECHNICIAN */ 8,
    /*   DEGREE_ENGINEER */ 8,
    /*  DEGREE_PROFESSOR */ 8,
    /*  DEGREE_DOCTORATE */ 8,
};

// 0x5C8F8C
static int charedit_tech_degree_icons_y[DEGREE_COUNT - 1] = {
    /*     DEGREE_NOVICE */ 96,
    /*  DEGREE_ASSISTANT */ 129,
    /*  DEGREE_ASSOCIATE */ 162,
    /* DEGREE_TECHNICIAN */ 195,
    /*   DEGREE_ENGINEER */ 228,
    /*  DEGREE_PROFESSOR */ 261,
    /*  DEGREE_DOCTORATE */ 294,
};

// 0x5C8FA8
static int charedit_tech_degree_icons[DEGREE_COUNT] = {
    /*     DEGREE_LAYMAN */ 653,
    /*     DEGREE_NOVICE */ 649,
    /*  DEGREE_ASSISTANT */ 650,
    /*  DEGREE_ASSOCIATE */ 651,
    /* DEGREE_TECHNICIAN */ 652,
    /*   DEGREE_ENGINEER */ 654,
    /*  DEGREE_PROFESSOR */ 655,
    /*  DEGREE_DOCTORATE */ 656,
};

// 0x5C8FC8
static S5C8150 stru_5C8FC8[3] = {
    { 0, 520, 174, -1 },
    { 0, -621, 173, -1 },
    { 0, 707, 176, -1 },
};

// 0x5C8FF8
static int dword_5C8FF8[5] = {
    521,
    521,
    521,
    521,
    521,
};

// 0x5C900C
static int dword_5C900C[11] = {
    186,
    234,
    281,
    329,
    376,
    621,
    200,
    248,
    296,
    344,
    392,
};

// 0x64C7A0
static tig_font_handle_t charedit_morph15_green_font;

// 0x64C7A8
static ScrollbarId charedit_scheme_scrollbar;

// 0x64C7B0
static tig_window_handle_t charedit_spells_win;

// 0x64C7B4
static tig_button_handle_t dword_64C7B4;

// 0x64C7B8
static int dword_64C7B8[BASIC_SKILL_COUNT];

// 0x64C7E8
static tig_button_handle_t dword_64C7E8[15];

// 0x64C824
static tig_button_handle_t dword_64C824;

// 0x64C828
static tig_font_handle_t charedit_morph15_gray_font;

// 0x64C82C
static int dword_64C82C[TECH_SKILL_COUNT];

// 0x64C83C
static tig_button_handle_t dword_64C83C;

// 0x64C840
static tig_font_handle_t charedit_garmond9_white_font;

// 0x64C844
static const char* charedit_fatigue_str;

// 0x64C848
static tig_font_handle_t charedit_cloister18_white_font;

// 0x64C84C
static int charedit_player_basic_skills_tbl[8][BASIC_SKILL_COUNT];

// 0x64C9CC
static int charedit_hint;

// 0x64C9D0
static tig_font_handle_t charedit_flare12_white_font;

// 0x64C9D4
static int charedit_player_tech_skills_tbl[8][TECH_SKILL_COUNT];

// 0x64CA54
static tig_button_handle_t dword_64CA54;

// 0x64CA58
static int charedit_skills_hint;

// 0x64CA5C
static tig_button_handle_t dword_64CA5C;

// 0x64CA60
static tig_window_handle_t charedit_scheme_win;

// 0x64CA64
static tig_window_handle_t charedit_window_handle;

// 0x64CA68
static tig_font_handle_t charedit_flare12_blue_font;

// 0x64CA6C
static tig_window_handle_t charedit_skills_win;

// 0x64CA70
static mes_file_handle_t charedit_mes_file;

// 0x64CA74
static const char* dword_64CA74[TRAINING_COUNT];

// 0x64CA84
static const char* charedit_seconds_str;

// 0x64CA88
static const char* charedit_second_str;

// 0x64CA8C
static tig_window_handle_t charedit_tech_win;

// 0x64CA90
static const char* charedit_scheme_names[200];

// 0x64CDB0
static tig_font_handle_t charedit_pork12_gray_font;

// 0x64CDB4
static int charedit_tech_hint;

// 0x64CDB8
static tig_font_handle_t charedit_garmond9_yellow_font;

// 0x64CDBC
static tig_font_handle_t charedit_arial10_white_font;

// 0x64CDC0
static tig_font_handle_t charedit_nick16_yellow_font;

// 0x64CDC4
static tig_button_handle_t charedit_dec_tech_degree_btn;

// 0x64CDC8
static int charedit_num_schemes;

// 0x64CDCC
static ChareditMode charedit_mode;

// 0x64CDD0
static tig_font_handle_t charedit_morph15_yellow_font;

// 0x64CDD4
static tig_button_handle_t spell_plus_bid;

// 0x64CDD8
static tig_button_handle_t dword_64CDD8;

// 0x64CDDC
static int charedit_player_spell_college_levels_tbl[8][COLLEGE_COUNT];

// 0x64CFDC
static tig_button_handle_t dword_64CFDC;

// 0x64CFE0
static tig_font_handle_t charedit_nick16_white_font;

// 0x64CFE4
static int charedit_scheme_ids[200];

// 0x64D304
static int dword_64D304[23];

// 0x64D360
static int charedit_spells_hint;

// 0x64D364
static int dword_64D364[COLLEGE_COUNT];

// 0x64D3A4
static tig_font_handle_t charedit_icons17_white_font;

// 0x64D3A8
static tig_font_handle_t charedit_morph15_white_font;

// 0x64D3AC
static tig_button_handle_t dword_64D3AC;

// 0x64D3B0
static tig_font_handle_t charedit_nick16_gray_font;

// 0x64D3B4
static tig_button_handle_t charedit_inc_tech_degree_btn;

// 0x64D3B8
static tig_button_handle_t dword_64D3B8;

// 0x64D3BC
static tig_font_handle_t charedit_morph15_red_font;

// 0x64D3C0
static const char* charedit_quest_str;

// 0x64D3C4
static char* charedit_errors[CHAREDIT_ERR_COUNT];

// 0x64D420
static tig_font_handle_t charedit_pork12_yellow_font;

// 0x64D424
static int charedit_top_scheme_index;

// 0x64D428
static tig_button_handle_t dword_64D428;

// 0x64D42C
static tig_font_handle_t charedit_garmond9_gray_font;

// 0x64D430
static tig_button_handle_t spell_minus_bid;

// 0x64D434
static int charedit_player_stats_tbl[8][CHAREDIT_STAT_COUNT];

// 0x64D714
static char byte_64D714[2000];

// 0x64DEE4
static bool charedit_scheme_scrollbar_initialized;

// 0x64DEE8
static const char* charedit_minimum_level_str;

// 0x64DEEC
static int dword_64DEEC[TECH_COUNT];

// 0x64DF0C
static tig_font_handle_t charedit_pork12_white_font;

// 0x64DF10
static int charedit_player_tech_degrees_tbl[8][TECH_COUNT];

// 0x64E010
static int64_t charedit_obj;

// 0x64E018
static bool charedit_created;

// 0x64E01C
static int dword_64E01C;

// 0x64E020
static int dword_64E020;

// 0x64E024
static int dword_64E024;

// 0x64E028
static int charedit_selected_tech;

// 0x64E02C
static bool dword_64E02C;

// 0x559690
bool charedit_init(GameInitInfo* init_info)
{
    (void)init_info;

    if (!charedit_labels_init()) {
        return false;
    }

    if (!charedit_scheme_win_create()) {
        charedit_labels_exit();
        return false;
    }

    if (!charedit_create_spells_win()) {
        charedit_labels_exit();
        tig_window_destroy(charedit_scheme_win);
        return false;
    }

    if (!charedit_create_tech_win()) {
        charedit_labels_exit();
        tig_window_destroy(charedit_spells_win);
        tig_window_destroy(charedit_scheme_win);
        return false;
    }

    if (!charedit_create_skills_win()) {
        charedit_labels_exit();
        tig_window_destroy(charedit_spells_win);
        tig_window_destroy(charedit_tech_win);
        tig_window_destroy(charedit_scheme_win);
        return false;
    }

    tig_window_hide(charedit_skills_win);
    tig_window_hide(charedit_spells_win);
    tig_window_hide(charedit_tech_win);
    tig_window_hide(charedit_scheme_win);

    return true;
}

// 0x559770
void charedit_exit()
{
    charedit_labels_exit();
    tig_window_destroy(charedit_skills_win);
    tig_window_destroy(charedit_spells_win);
    tig_window_destroy(charedit_tech_win);
    tig_window_destroy(charedit_scheme_win);
}

// 0x5597B0
void charedit_reset()
{
    if (charedit_is_created()) {
        charedit_close();
    }
}

// 0x5597C0
bool charedit_open(int64_t obj, ChareditMode mode)
{
    int64_t pc_obj;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigWindowData window_data;
    TigButtonData button_data;
    int index;
    int portrait;
    PcLens pc_lens;
    PacketChareditTraitChange pkt;

    if (charedit_created) {
        charedit_close();
        return true;
    }

    pc_obj = player_get_local_pc_obj();
    if (critter_is_dead(pc_obj) || !intgame_mode_set(INTGAME_MODE_MAIN) || !intgame_mode_set(INTGAME_MODE_CHAREDIT)) {
        return false;
    }

    charedit_obj = obj;
    charedit_mode = mode;

    if (charedit_mode == CHAREDIT_MODE_CREATE) {
        object_hp_damage_set(charedit_obj, 0);
        critter_fatigue_damage_set(charedit_obj, 0);
    }

    tig_art_interface_id_create(22, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    if (!intgame_big_window_lock(charedit_window_message_filter, &charedit_window_handle)) {
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    tig_window_data(charedit_window_handle, &window_data);

    window_data.rect.x = 0;
    window_data.rect.y = 0;

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = window_data.rect.width;
    dst_rect.height = window_data.rect.height;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &(window_data.rect);
    art_blit_info.dst_rect = &dst_rect;
    if (tig_window_blit_art(charedit_window_handle, &art_blit_info) != TIG_OK) {
        intgame_big_window_unlock();
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_window_handle;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    tig_art_interface_id_create(24, 0, 0, 0, &button_data.art_id);
    for (index = 0; index < 10; index++) {
        if (charedit_mode == CHAREDIT_MODE_PASSIVE) {
            stru_5C7F88[index].button_handle = TIG_BUTTON_HANDLE_INVALID;
        } else {
            button_data.x = stru_5C7F88[index].x;
            button_data.y = stru_5C7F88[index].y;
            if (tig_button_create(&button_data, &(stru_5C7F88[index].button_handle))) {
                intgame_big_window_unlock();
                charedit_obj = OBJ_HANDLE_NULL;
                return false;
            }
        }
    }

    tig_art_interface_id_create(23, 0, 0, 0, &button_data.art_id);
    for (index = 0; index < 10; index++) {
        if (charedit_mode == CHAREDIT_MODE_PASSIVE) {
            stru_5C8028[index].button_handle = TIG_BUTTON_HANDLE_INVALID;
        } else {
            button_data.x = stru_5C8028[index].x;
            button_data.y = stru_5C8028[index].y;
            if (tig_button_create(&button_data, &(stru_5C8028[index].button_handle))) {
                intgame_big_window_unlock();
                charedit_obj = OBJ_HANDLE_NULL;
                return false;
            }
        }
    }

    charedit_refresh_basic_info();

    if (intgame_examine_portrait(pc_obj, charedit_obj, &portrait)) {
        portrait_draw_native(charedit_obj, portrait, charedit_window_handle, stru_5C89A8[0].x, stru_5C89A8[0].y);
    } else {
        tig_art_interface_id_create(portrait, 0, 0, 0, &art_id);
        tig_art_frame_data(art_id, &art_frame_data);

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.x = stru_5C89A8[0].x;
        dst_rect.y = stru_5C89A8[0].y;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.flags = 0;
        art_blit_info.art_id = art_id;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(charedit_window_handle, &art_blit_info);
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_window_handle;
    tig_art_interface_id_create(34, 0, 0, 0, &(button_data.art_id));
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    button_data.x = 527;
    button_data.y = 11;
    if (tig_button_create(&button_data, &dword_64C7B4) != TIG_OK) {
        intgame_big_window_unlock();
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    button_data.x = 595;
    tig_art_interface_id_create(35, 0, 0, 0, &(button_data.art_id));
    if (tig_button_create(&button_data, &dword_64C83C) != TIG_OK) {
        intgame_big_window_unlock();
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    button_data.x = 663;
    tig_art_interface_id_create(36, 0, 0, 0, &(button_data.art_id));
    if (tig_button_create(&button_data, &dword_64D3B8) != TIG_OK) {
        intgame_big_window_unlock();
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    button_data.x = 754;
    button_data.y = 21;
    tig_art_interface_id_create(568, 0, 0, 0, &(button_data.art_id));
    if (tig_button_create(&button_data, &dword_64CA54) != TIG_OK) {
        intgame_big_window_unlock();
        charedit_obj = OBJ_HANDLE_NULL;
        return false;
    }

    tig_window_show(charedit_skills_win);
    tig_window_show(charedit_spells_win);
    tig_window_show(charedit_tech_win);
    tig_window_show(charedit_scheme_win);
    sub_51E850(charedit_skills_win);

    if (charedit_mode == CHAREDIT_MODE_PASSIVE) {
        for (index = 0; index < 15; index++) {
            tig_button_hide(dword_64C7E8[index]);
        }
    } else {
        for (index = 0; index < 15; index++) {
            tig_button_show(dword_64C7E8[index]);
        }
    }

    dword_64E01C = 0;
    sub_55BD10(dword_64E020);

    if (charedit_obj != OBJ_HANDLE_NULL) {
        dword_64D428 = TIG_BUTTON_HANDLE_INVALID;
        dword_64CFDC = TIG_BUTTON_HANDLE_INVALID;
    } else {
        button_data.flags = 1;
        button_data.window_handle = charedit_window_handle;
        button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
        button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
        button_data.mouse_enter_snd_id = -1;
        button_data.mouse_exit_snd_id = -1;

        tig_art_interface_id_create(24, 0, 0, 0, &(button_data.art_id));
        button_data.x = 82;
        button_data.y = 196;
        tig_button_create(&button_data, &dword_64D428);

        button_data.y += 12;
        tig_art_interface_id_create(23, 0, 0, 0, &(button_data.art_id));
        tig_button_create(&button_data, &dword_64CFDC);
    }

    charedit_refresh_internal();
    location_origin_set(obj_field_int64_get(charedit_obj, OBJ_F_LOCATION));

    pc_lens.window_handle = charedit_window_handle;
    pc_lens.rect = &stru_5C8930;

    // FIX: Use 222 ("char_pcc.art") instead of 198 ("pcwincvr.art") to match
    // character editor background.
    tig_art_interface_id_create(222, 0, 0, 0, &(pc_lens.art_id));

    if (charedit_mode == CHAREDIT_MODE_ACTIVE || charedit_mode == CHAREDIT_MODE_PASSIVE) {
        intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
    }

    if (charedit_mode != CHAREDIT_MODE_3) {
        if (!tig_net_is_active()
            || tig_net_is_host()
            || multiplayer_is_locked()) {
            for (index = 0; index < 23; index++) {
                dword_64D304[index] = charedit_stat_value_get(charedit_obj, index);
            }

            for (index = 0; index < TECH_COUNT; index++) {
                dword_64DEEC[index] = tech_degree_get(charedit_obj, index);
            }

            for (index = 0; index < COLLEGE_COUNT; index++) {
                dword_64D364[index] = spell_college_level_get(charedit_obj, index);
            }

            for (index = 0; index < BASIC_SKILL_COUNT; index++) {
                dword_64C7B8[index] = basic_skill_points_get(charedit_obj, index);
            }

            for (index = 0; index < TECH_SKILL_COUNT; index++) {
                dword_64C82C[index] = tech_skill_points_get(charedit_obj, index);
            }
        } else {
            pkt.type = 127;
            pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_CACHE;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_window_handle;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.art_id = TIG_ART_ID_INVALID;

    for (index = 0; index < 32; index++) {
        button_data.x = stru_5C89A8[index].x;
        button_data.y = stru_5C89A8[index].y;
        button_data.width = stru_5C89A8[index].width;
        button_data.height = stru_5C89A8[index].height;
        tig_button_create(&button_data, &stru_5C89A8[index].button_handle);
    }

    dword_64C824 = TIG_BUTTON_HANDLE_INVALID;
    dword_64CA5C = TIG_BUTTON_HANDLE_INVALID;

    if (charedit_mode == CHAREDIT_MODE_PASSIVE
        && critter_leader_get(charedit_obj) == pc_obj
        && critter_follower_next(charedit_obj) != charedit_obj) {
        button_data.flags = TIG_BUTTON_MOMENTARY;
        button_data.window_handle = charedit_window_handle;

        tig_art_interface_id_create(827, 0, 0, 0, &(button_data.art_id));
        button_data.x = 126;
        button_data.y = 44;
        tig_button_create(&button_data, &dword_64C824);

        tig_art_interface_id_create(828, 0, 0, 0, &(button_data.art_id));
        button_data.x = 461;
        button_data.y = 44;
        tig_button_create(&button_data, &dword_64CA5C);
    }

    charedit_hint = -1;
    charedit_skills_hint = -1;
    charedit_tech_hint = -1;
    charedit_spells_hint = -1;

    if (player_is_local_pc_obj(obj)) {
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_CHAR, false);
    }

    sub_424070(obj, 4, 0, 1);

    charedit_created = true;

    if (combat_auto_attack_get(pc_obj)) {
        dword_64E02C = true;
        combat_auto_attack_set(0);
    }

    return true;
}

// 0x55A150
void charedit_close()
{
    if (charedit_created) {
        charedit_created = false;
        if (charedit_mode == CHAREDIT_MODE_ACTIVE || charedit_mode == CHAREDIT_MODE_PASSIVE) {
            intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
        }
        if (charedit_mode == CHAREDIT_MODE_CREATE) {
            object_hp_damage_set(charedit_obj, 0);
            critter_fatigue_damage_set(charedit_obj, 0);
        }
        intgame_big_window_unlock();
        tig_window_hide(charedit_skills_win);
        tig_window_hide(charedit_spells_win);
        tig_window_hide(charedit_tech_win);
        tig_window_hide(charedit_scheme_win);
        charedit_obj = OBJ_HANDLE_NULL;
        iso_interface_refresh();
        intgame_mode_set(INTGAME_MODE_MAIN);
        sub_55EFB0();
        if (dword_64E02C) {
            combat_auto_attack_set(true);
            dword_64E02C = false;
        }
    }
}

// 0x55A220
bool charedit_is_created()
{
    return charedit_created;
}

// 0x55A230
void charedit_refresh()
{
    if (charedit_created) {
        charedit_refresh_internal();
    }
}

// 0x55A240
void charedit_refresh_basic_info()
{
    TigFont font_desc;
    char* pch;
    char buffers[7][40];
    const char* labels[7];
    int index;

    sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[0]), NULL, -1, 3);
    sub_55B880(charedit_window_handle, charedit_morph15_yellow_font, &(stru_5C8150[3]), NULL, -1, 1);

    if (obj_field_int32_get(charedit_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[4]), NULL, -1, 5);
    } else if (critter_is_monstrous(charedit_obj)) {
        sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[7]), NULL, -1, 2);
    } else {
        sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[4]), NULL, -1, 2);
        sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[7]), NULL, -1, 2);
    }

    sub_55B880(charedit_window_handle, charedit_flare12_white_font, stru_5C81E0, NULL, -1, 13);

    object_examine(charedit_obj, charedit_obj, buffers[3]);
    tig_font_push(charedit_morph15_yellow_font);
    font_desc.str = buffers[3];
    pch = &(buffers[3][strlen(buffers[3])]);
    do {
        *pch-- = '\0';
        font_desc.width = 0;
        tig_font_measure(&font_desc);
    } while (font_desc.width > 243);
    tig_font_pop();

    sprintf(buffers[0], ": %d  ", charedit_stat_value_get(charedit_obj, CHAREDIT_STAT_UNSPENT_POINTS));
    sprintf(buffers[1], ": %d  ", charedit_stat_value_get(charedit_obj, CHAREDIT_STAT_LEVEL));
    sprintf(buffers[2], ": %d  ", charedit_stat_value_get(charedit_obj, CHAREDIT_STAT_XP_TO_NEXT_LEVEL));
    sprintf(buffers[4], ": %s", race_name(stat_level_get(charedit_obj, STAT_RACE)));
    sprintf(buffers[5], ": %s", gender_name(stat_level_get(charedit_obj, STAT_GENDER)));
    sprintf(buffers[6], ": %d ", stat_level_get(charedit_obj, STAT_AGE));

    for (index = 0; index < 7; index++) {
        labels[index] = buffers[index];
    }

    sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[0]), &(labels[0]), -1, 3);
    sub_55B880(charedit_window_handle, charedit_morph15_yellow_font, &(stru_5C8150[3]), &(labels[3]), -1, 1);

    if (obj_field_int32_get(charedit_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[4]), &(labels[4]), -1, 3);
    } else if (!critter_is_monstrous(charedit_obj)) {
        sub_55B880(charedit_window_handle, charedit_morph15_white_font, &(stru_5C8150[4]), &(labels[4]), -1, 2);
    }
}

// 0x55A5C0
bool charedit_window_message_filter(TigMessage* msg)
{
    int index;
    int param;
    int value;
    int cost;
    int unspent_points;
    int stat;

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            if (msg->data.button.button_handle == dword_64C7B4) {
                charedit_hint = 132;
                return true;
            }

            if (msg->data.button.button_handle == dword_64C83C) {
                charedit_hint = 133;
                return true;
            }

            if (msg->data.button.button_handle == dword_64D3B8) {
                charedit_hint = 134;
                return true;
            }

            if (msg->data.button.button_handle == dword_64CA54) {
                charedit_hint = 140;
                return true;
            }

            for (index = 0; index < 32; index++) {
                if (msg->data.button.button_handle == stru_5C89A8[index].button_handle) {
                    charedit_hint = stru_5C89A8[index].field_10;
                    return true;
                }
            }

            for (index = 0; index < 10; index++) {
                if (msg->data.button.button_handle == stru_5C7F88[index].button_handle) {
                    charedit_hint = dword_5C80C8[index];
                    return true;
                }

                if (msg->data.button.button_handle == stru_5C8028[index].button_handle) {
                    charedit_hint = dword_5C80C8[index];
                    return true;
                }
            }

            return false;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            if (msg->data.button.button_handle == dword_64C7B4
                || msg->data.button.button_handle == dword_64C83C
                || msg->data.button.button_handle == dword_64D3B8
                || msg->data.button.button_handle == dword_64CA54) {
                charedit_hint = -1;
                sub_550720();
                return true;
            }

            for (index = 0; index < 32; index++) {
                if (msg->data.button.button_handle == stru_5C89A8[index].button_handle) {
                    charedit_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            for (index = 0; index < 10; index++) {
                if (msg->data.button.button_handle == stru_5C7F88[index].button_handle
                    || msg->data.button.button_handle == stru_5C8028[index].button_handle) {
                    charedit_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            return false;
        case TIG_BUTTON_STATE_RELEASED:
            if (msg->data.button.button_handle == dword_64C824) {
                charedit_cycle_obj(false);
                return true;
            }

            if (msg->data.button.button_handle == dword_64CA5C) {
                charedit_cycle_obj(true);
                return true;
            }

            return false;
        case TIG_BUTTON_STATE_PRESSED:
            for (index = 0; index < 10; index++) {
                if (msg->data.button.button_handle == stru_5C7F88[index].button_handle) {
                    param = stru_5C7F88[index].art_num;
                    value = charedit_stat_value_get(charedit_obj, param) + 1;
                    if (param == CHAREDIT_STAT_HP_PTS_MAXIMUM || param == CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM) {
                        cost = 1;
                    } else {
                        stat = charedit_stat_map_to_critter_stat(param);
                        if (stat_level_get(charedit_obj, stat) >= stat_level_max(charedit_obj, stat)) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_MAX];
                            sub_550750(&charedit_error_msg);
                            return true;
                        }
                        cost = stat_cost(value);
                    }

                    unspent_points = stat_level_get(charedit_obj, STAT_UNSPENT_POINTS);
                    if (cost > unspent_points) {
                        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_CHARACTER_POINTS];
                        sub_550750(&charedit_error_msg);
                        return true;
                    }

                    if (tig_net_is_active()
                        && !tig_net_is_host()
                        && !multiplayer_is_locked()) {
                        PacketChareditTraitChange pkt;

                        pkt.type = 127;
                        pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC;
                        pkt.trait = MP_CHAREDIT_TRAIT_STAT;
                        pkt.param = param;
                        tig_net_send_app_all(&pkt, sizeof(pkt));

                        return true;
                    }

                    charedit_stat_value_set(charedit_obj, param, value);

                    if (charedit_stat_value_get(charedit_obj, param) < value) {
                        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_MAX];
                        sub_550750(&charedit_error_msg);
                        return true;
                    }

                    stat_base_set(charedit_obj, STAT_UNSPENT_POINTS, unspent_points - cost);
                    charedit_refresh_internal();
                    sub_550720();
                    return true;
                }

                if (msg->data.button.button_handle == stru_5C8028[index].button_handle) {
                    param = stru_5C8028[index].art_num;
                    value = charedit_stat_value_get(charedit_obj, param);
                    if (value == dword_64D304[param]) {
                        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_ACCEPTED_LEVEL];
                        sub_550750(&charedit_error_msg);
                        return true;
                    }

                    if (param == CHAREDIT_STAT_HP_PTS_MAXIMUM || param == CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM) {
                        cost = 1;
                    } else {
                        cost = stat_cost(value);
                    }

                    value--;

                    if (tig_net_is_active()
                        && !tig_net_is_host()
                        && !multiplayer_is_locked()) {
                        PacketChareditTraitChange pkt;

                        pkt.type = 127;
                        pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC;
                        pkt.trait = MP_CHAREDIT_TRAIT_STAT;
                        pkt.param = param;
                        tig_net_send_app_all(&pkt, sizeof(pkt));

                        return true;
                    }

                    charedit_stat_value_set(charedit_obj, param, value);

                    if (charedit_stat_value_get(charedit_obj, param) > value) {
                        if (value == 0) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_MIN];
                            sub_550750(&charedit_error_msg);
                            return true;
                        }

                        if (dword_5C8124[index] != -1
                            && !skill_check_stat(charedit_obj, dword_5C8124[index], value - 1)) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_SKILL_PREREQUISITE];
                            sub_550750(&charedit_error_msg);
                            return true;
                        }

                        if (param == CHAREDIT_STAT_INTELLIGENCE_BASE
                            && !spell_check_intelligence(charedit_obj, value - 1)) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_INTELLIGENCE_SPELL_PREREQUISITE];
                            sub_550750(&charedit_error_msg);
                            return true;
                        }

                        if (param == CHAREDIT_STAT_INTELLIGENCE_BASE
                            && !tech_check_intelligence(charedit_obj, value - 1)) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_INTELLIGENCE_TECH_PREREQUISITE];
                            sub_550750(&charedit_error_msg);
                            return true;
                        }

                        if (param == CHAREDIT_STAT_WILLPOWER_BASE
                            && !spell_check_willpower(charedit_obj, value - 1)) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_WILLPOWER_SPELL_PREREQUISITE];
                            sub_550750(&charedit_error_msg);
                            return true;
                        }

                        // Something else is wrong.
                        return true;
                    }

                    unspent_points = stat_level_get(charedit_obj, STAT_UNSPENT_POINTS);
                    stat_base_set(charedit_obj, STAT_UNSPENT_POINTS, unspent_points + cost);
                    charedit_refresh_internal();
                    sub_550720();
                    return true;
                }
            }

            if (msg->data.button.button_handle == dword_64C7B4) {
                if (dword_64E01C == 3) {
                    sub_55F0D0();
                }
                sub_51E850(charedit_skills_win);
                dword_64E01C = 0;
                charedit_refresh_internal();
                return true;
            }

            if (msg->data.button.button_handle == dword_64C83C) {
                if (dword_64E01C == 3) {
                    sub_55F0D0();
                }
                sub_51E850(charedit_tech_win);
                dword_64E01C = 1;
                charedit_refresh_internal();
                return true;
            }

            if (msg->data.button.button_handle == dword_64D3B8) {
                if (dword_64E01C == 3) {
                    sub_55F0D0();
                }
                sub_51E850(charedit_spells_win);
                dword_64E01C = 2;
                charedit_refresh_internal();
                return true;
            }

            if (msg->data.button.button_handle == dword_64CA54) {
                sub_51E850(charedit_scheme_win);
                if (dword_64E01C != 3) {
                    sub_55EFE0();
                }
                dword_64E01C = 3;
                charedit_refresh_internal();
                return true;
            }

            if (msg->data.button.button_handle == dword_64D428) {
                value = stat_base_get(charedit_obj, STAT_LEVEL) + 1;
                if (stat_base_set(charedit_obj, STAT_LEVEL, value) == value) {
                    unspent_points = stat_base_get(charedit_obj, STAT_UNSPENT_POINTS);
                    stat_base_set(charedit_obj, STAT_UNSPENT_POINTS, unspent_points + 1);
                    charedit_refresh_internal();
                }
                return true;
            }

            if (msg->data.button.button_handle == dword_64CFDC) {
                unspent_points = stat_base_get(charedit_obj, STAT_UNSPENT_POINTS);
                if (unspent_points >= 1) {
                    value = stat_base_get(charedit_obj, STAT_LEVEL) - 1;
                    if (stat_base_set(charedit_obj, STAT_LEVEL, value) == value) {
                        stat_base_set(charedit_obj, STAT_UNSPENT_POINTS, unspent_points - 1);
                    }
                    charedit_refresh_internal();
                }
                return true;
            }

            return false;
        }
        return false;
    case TIG_MESSAGE_MOUSE:
        switch (msg->data.mouse.event) {
        case TIG_MESSAGE_MOUSE_IDLE:
            charedit_show_hint(charedit_hint);
            return false;
        case TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP:
            if (charedit_mode != CHAREDIT_MODE_CREATE
                && charedit_mode != CHAREDIT_MODE_3
                && intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
                charedit_close();
                return true;
            }
            return false;
        default:
            return false;
        }
    case TIG_MESSAGE_KEYBOARD:
        if (msg->data.keyboard.key == SDLK_X
            && msg->data.keyboard.pressed
            && !tig_net_is_active()
            && charedit_mode == CHAREDIT_MODE_ACTIVE
            && (!combat_turn_based_is_active()
                || combat_turn_based_whos_turn_get() == player_get_local_pc_obj())) {
            gamelib_save("ExportSave", "Export-Save");
            sub_4A6470(player_get_local_pc_obj());
            gamelib_load("ExportSave");
            gamelib_delete("ExportSave");
            return true;
        }
        return false;
    default:
        return false;
    }
}

// 0x55AE70
void charedit_show_hint(int hint)
{
    UiMessage ui_message;
    MesFileEntry mes_file_entry;
    int value;

    if (hint == -1) {
        return;
    }

    if (hint >= 109 && hint <= 116) {
        ui_message.type = UI_MSG_TYPE_STAT;
        ui_message.field_8 = dword_5C8124[hint - 109 + 2];
        value = stat_base_get(charedit_obj, ui_message.field_8);
        if (value < stat_level_max(charedit_obj, ui_message.field_8)) {
            ui_message.field_C = stat_cost(value + 1);
        } else {
            ui_message.field_C = 0;
        }
        mes_file_entry.num = hint;
        if (mes_search(charedit_mes_file, &mes_file_entry)) {
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
        }
    } else if (hint >= 1000 && hint < 1999) {
        ui_message.type = UI_MSG_TYPE_SKILL;
        ui_message.field_8 = hint - 1000;
        if (IS_TECH_SKILL(ui_message.field_8)) {
            ui_message.field_C = tech_skill_min_stat_level_required(tech_skill_base(charedit_obj, GET_TECH_SKILL(ui_message.field_8)) + 4);
        } else {
            ui_message.field_C = basic_skill_min_stat_level_required(basic_skill_base(charedit_obj, GET_BASIC_SKILL(ui_message.field_8)) + 4);
        }
        sub_550750(&ui_message);
    } else if (hint >= 2000 && hint < 2999) {
        ui_message.type = UI_MSG_TYPE_SPELL;
        ui_message.field_8 = hint - 2000;
        ui_message.field_C = spell_cost(hint - 2000);
        ui_message.field_10 = charedit_obj;
        sub_550750(&ui_message);
    } else if (hint >= 3000 && hint < 3999) {
        ui_message.type = UI_MSG_TYPE_COLLEGE;
        ui_message.field_8 = hint - 3000;
        sub_550750(&ui_message);
    } else if (hint >= 4000 && hint < 4999) {
        ui_message.type = UI_MSG_TYPE_TECH;
        ui_message.field_8 = hint - 4000;
        sub_550750(&ui_message);
    } else if (hint >= 5000 && hint < 5999) {
        ui_message.type = UI_MSG_TYPE_DEGREE;
        ui_message.field_8 = hint - 5000;
        ui_message.field_C = tech_degree_cost_get(ui_message.field_8 % DEGREE_COUNT);
        sub_550750(&ui_message);
    } else {
        ui_message.type = UI_MSG_TYPE_FEEDBACK;
        mes_file_entry.num = hint;
        if (mes_search(charedit_mes_file, &mes_file_entry)) {
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
        }
    }
}

// 0x55B0E0
void charedit_cycle_obj(bool next)
{
    int64_t obj;
    ChareditMode mode;

    if (next) {
        obj = critter_follower_next(charedit_obj);
    } else {
        obj = critter_follower_prev(charedit_obj);
    }

    if (obj != charedit_obj) {
        mode = charedit_mode;
        charedit_close();
        charedit_open(obj, mode);
    }
}

// 0x55B150
void charedit_refresh_internal()
{
    charedit_refresh_stats();
    charedit_refresh_secondary_stats();
    switch (dword_64E01C) {
    case 0:
        charedit_refresh_skills_win();
        break;
    case 1:
        charedit_refresh_tech_win();
        break;
    case 2:
        charedit_refresh_spells_win();
        break;
    default:
        charedit_scheme_win_refresh();
        break;
    }
    charedit_refresh_alignment_aptitude_bars();
    charedit_refresh_basic_info();
    iso_interface_refresh();
    intgame_draw_bar(INTGAME_BAR_HEALTH);
    intgame_draw_bar(INTGAME_BAR_FATIGUE);
}

// 0x55B1B0
void charedit_refresh_secondary_stats()
{
    char buffers[13][10];
    const char* labels[13];
    int index;

    for (index = 0; index < 13; index++) {
        labels[index] = buffers[index];
    }

    for (index = 0; index < 8; index++) {
        sprintf(buffers[index],
            ": %d  ",
            stat_level_get(charedit_obj, stru_5C81E0[index].value));
    }

    for (index = 8; index < 13; index++) {
        sprintf(buffers[index],
            ": %d  ",
            object_get_resistance(charedit_obj, stru_5C81E0[index].value, true));
    }

    sub_55B880(charedit_window_handle, charedit_flare12_white_font, stru_5C81E0, labels, -1, 13);
}

// 0x55B280
void charedit_refresh_stats()
{
    int stat;

    for (stat = 0; stat < CHAREDIT_STAT_COUNT; stat++) {
        charedit_refresh_stat(stat);
    }
}

// 0x55B2A0
void charedit_refresh_stat(int stat)
{
    tig_font_handle_t font;
    int value;
    int base_value;
    int effective_value;
    char str[160];

    switch (stat) {
    case CHAREDIT_STAT_UNSPENT_POINTS:
    case CHAREDIT_STAT_LEVEL:
    case CHAREDIT_STAT_XP_TO_NEXT_LEVEL:
    case CHAREDIT_STAT_STRENGTH_BASE:
    case CHAREDIT_STAT_CONSTITUTION_BASE:
    case CHAREDIT_STAT_DEXTERITY_BASE:
    case CHAREDIT_STAT_BEAUTY_BASE:
    case CHAREDIT_STAT_INTELLIGENCE_BASE:
    case CHAREDIT_STAT_WILLPOWER_BASE:
    case CHAREDIT_STAT_PERCEPTION_BASE:
    case CHAREDIT_STAT_CHARISMA_BASE:
        return;
    }

    font = charedit_morph15_white_font;
    switch (stat) {
    case CHAREDIT_STAT_HP_PTS_MAXIMUM:
        value = object_hp_max(charedit_obj);
        break;
    case CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM:
        value = critter_fatigue_max(charedit_obj);
        break;
    default:
        value = charedit_stat_value_get(charedit_obj, stat);
        base_value = charedit_stat_value_get(charedit_obj, stat - 1);
        effective_value = effect_adjust_stat_level_no_innate(charedit_obj, charedit_stat_map_to_critter_stat(stat), base_value);
        if (effective_value > base_value) {
            font = charedit_morph15_green_font;
        } else if (effective_value < base_value) {
            font = charedit_morph15_red_font;
        }
    }

    sprintf(str, " %d ", value);
    stru_5C8F50.str = str;
    stru_5C8F50.x = stru_5C7E70[stat].x;
    stru_5C8F50.y = stru_5C7E70[stat].y;
    sub_55B880(charedit_window_handle, font, &stru_5C8F50, NULL, -1, 1);
}

// 0x55B410
int charedit_stat_map_to_critter_stat(int stat)
{
    switch (stat) {
    case CHAREDIT_STAT_UNSPENT_POINTS:
        return STAT_UNSPENT_POINTS;
    case CHAREDIT_STAT_LEVEL:
        return STAT_LEVEL;
    case CHAREDIT_STAT_HP_PTS_CURRENT:
    case CHAREDIT_STAT_HP_PTS_MAXIMUM:
        return -3;
    case CHAREDIT_STAT_FATIGUE_PTS_CURRENT:
    case CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM:
        return -2;
    case CHAREDIT_STAT_STRENGTH_BASE:
    case CHAREDIT_STAT_STRENGTH_LEVEL:
        return STAT_STRENGTH;
    case CHAREDIT_STAT_CONSTITUTION_BASE:
    case CHAREDIT_STAT_CONSTITUTION_LEVEL:
        return STAT_CONSTITUTION;
    case CHAREDIT_STAT_DEXTERITY_BASE:
    case CHAREDIT_STAT_DEXTERITY_LEVEL:
        return STAT_DEXTERITY;
    case CHAREDIT_STAT_BEAUTY_BASE:
    case CHAREDIT_STAT_BEAUTY_LEVEL:
        return STAT_BEAUTY;
    case CHAREDIT_STAT_INTELLIGENCE_BASE:
    case CHAREDIT_STAT_INTELLIGENCE_LEVEL:
        return STAT_INTELLIGENCE;
    case CHAREDIT_STAT_WILLPOWER_BASE:
    case CHAREDIT_STAT_WILLPOWER_LEVEL:
        return STAT_WILLPOWER;
    case CHAREDIT_STAT_PERCEPTION_BASE:
    case CHAREDIT_STAT_PERCEPTION_LEVEL:
        return STAT_PERCEPTION;
    case CHAREDIT_STAT_CHARISMA_BASE:
    case CHAREDIT_STAT_CHARISMA_LEVEL:
        return STAT_CHARISMA;
    default:
        // FIXME: Ideally should return something else to denote error.
        return 0;
    }
}

// 0x55B4D0
int charedit_stat_value_get(int64_t obj, int stat)
{
    switch (stat) {
    case CHAREDIT_STAT_UNSPENT_POINTS:
        return stat_base_get(obj, STAT_UNSPENT_POINTS);
    case CHAREDIT_STAT_LEVEL:
        return stat_level_get(obj, STAT_LEVEL);
    case CHAREDIT_STAT_XP_TO_NEXT_LEVEL:
        return level_experience_points_to_next_level(obj);
    case CHAREDIT_STAT_HP_PTS_CURRENT:
        return object_hp_current(obj);
    case CHAREDIT_STAT_HP_PTS_MAXIMUM:
        return object_hp_pts_get(obj);
    case CHAREDIT_STAT_FATIGUE_PTS_CURRENT:
        return critter_fatigue_current(obj);
    case CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM:
        return critter_fatigue_pts_get(obj);
    case CHAREDIT_STAT_STRENGTH_BASE:
        return stat_base_get(obj, STAT_STRENGTH);
    case CHAREDIT_STAT_STRENGTH_LEVEL:
        return stat_level_get(obj, STAT_STRENGTH);
    case CHAREDIT_STAT_CONSTITUTION_BASE:
        return stat_base_get(obj, STAT_CONSTITUTION);
    case CHAREDIT_STAT_CONSTITUTION_LEVEL:
        return stat_level_get(obj, STAT_CONSTITUTION);
    case CHAREDIT_STAT_DEXTERITY_BASE:
        return stat_base_get(obj, STAT_DEXTERITY);
    case CHAREDIT_STAT_DEXTERITY_LEVEL:
        return stat_level_get(obj, STAT_DEXTERITY);
    case CHAREDIT_STAT_BEAUTY_BASE:
        return stat_base_get(obj, STAT_BEAUTY);
    case CHAREDIT_STAT_BEAUTY_LEVEL:
        return stat_level_get(obj, STAT_BEAUTY);
    case CHAREDIT_STAT_INTELLIGENCE_BASE:
        return stat_base_get(obj, STAT_INTELLIGENCE);
    case CHAREDIT_STAT_INTELLIGENCE_LEVEL:
        return stat_level_get(obj, STAT_INTELLIGENCE);
    case CHAREDIT_STAT_WILLPOWER_BASE:
        return stat_base_get(obj, STAT_WILLPOWER);
    case CHAREDIT_STAT_WILLPOWER_LEVEL:
        return stat_level_get(obj, STAT_WILLPOWER);
    case CHAREDIT_STAT_PERCEPTION_BASE:
        return stat_base_get(obj, STAT_PERCEPTION);
    case CHAREDIT_STAT_PERCEPTION_LEVEL:
        return stat_level_get(obj, STAT_PERCEPTION);
    case CHAREDIT_STAT_CHARISMA_BASE:
        return stat_base_get(obj, STAT_CHARISMA);
    case CHAREDIT_STAT_CHARISMA_LEVEL:
        return stat_level_get(obj, STAT_CHARISMA);
    default:
        return 0;
    }
}

// 0x55B720
void charedit_stat_value_set(int64_t obj, int stat, int value)
{
    switch (stat) {
    case CHAREDIT_STAT_HP_PTS_MAXIMUM:
        object_hp_pts_set(obj, value);
        break;
    case CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM:
        critter_fatigue_pts_set(obj, value);
        break;
    case CHAREDIT_STAT_STRENGTH_BASE:
        stat_base_set(obj, STAT_STRENGTH, value);
        break;
    case CHAREDIT_STAT_CONSTITUTION_BASE:
        stat_base_set(obj, STAT_CONSTITUTION, value);
        break;
    case CHAREDIT_STAT_DEXTERITY_BASE:
        stat_base_set(obj, STAT_DEXTERITY, value);
        break;
    case CHAREDIT_STAT_BEAUTY_BASE:
        stat_base_set(obj, STAT_BEAUTY, value);
        break;
    case CHAREDIT_STAT_INTELLIGENCE_BASE:
        stat_base_set(obj, STAT_INTELLIGENCE, value);
        break;
    case CHAREDIT_STAT_WILLPOWER_BASE:
        stat_base_set(obj, STAT_WILLPOWER, value);
        break;
    case CHAREDIT_STAT_PERCEPTION_BASE:
        stat_base_set(obj, STAT_PERCEPTION, value);
        break;
    case CHAREDIT_STAT_CHARISMA_BASE:
        stat_base_set(obj, STAT_CHARISMA, value);
        break;
    }
}

// 0x55B880
void sub_55B880(tig_window_handle_t window_handle, tig_font_handle_t font, S5C8150* a3, const char** list, int a5, int a6)
{
    TigWindowData window_data;
    TigArtBlitInfo art_blit_info;
    TigRect rect;
    TigFont font_desc;
    int num;
    int index;
    int x;

    tig_window_data(window_handle, &window_data);

    if (window_handle == charedit_skills_win) {
        num = 29;
    } else if (window_handle == charedit_tech_win) {
        num = 30;
    } else if (window_handle == charedit_spells_win) {
        num = 31;
    } else if (window_handle == charedit_scheme_win) {
        num = 567;
    } else {
        num = 22;
    }

    tig_art_interface_id_create(num, 0, 0, 0, &(art_blit_info.art_id));
    art_blit_info.flags = 0;

    // TODO: The loop below does not look good, review. For now initialize some
    // vars to silence compiler warnings.
    rect.x = 0;
    font_desc.width = 0;
    font_desc.str = NULL;

    tig_font_push(font);
    for (index = 0; index < a6; index++) {
        if (a5 == -1) {
            font_desc.str = a3[index].str;
            font_desc.width = 0;
            tig_font_measure(&font_desc);

            x = a3[index].x;
            if (x < 0) {
                x = -x - font_desc.width / 2;
            }
            rect.x = x - window_data.rect.x;
            rect.width = font_desc.width;
            rect.height = font_desc.height;
        }
        rect.y = a3[index].y - window_data.rect.y;

        if (list != NULL) {
            font_desc.str = list[index];
            if (a5 == -1) {
                if (a3[index].str != NULL) {
                    rect.x += font_desc.width;
                }
            } else if (a5 < 0) {
                font_desc.width = 0;
                tig_font_measure(&font_desc);
                rect.x = -window_data.rect.x - font_desc.width / 2 - a5;
            } else {
                rect.x = a5 - window_data.rect.x;
            }

            font_desc.width = 0;
            tig_font_measure(&font_desc);

            if (a5 == -1 && a3[index].str == NULL) {
                x = a3[index].x;
                if (x < 0) {
                    x = -x - font_desc.width / 2;
                }
                rect.x = x - window_data.rect.x;
            }

            rect.width = font_desc.width;
            rect.height = font_desc.height;
        }

        hrp_apply(&rect, GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

        art_blit_info.src_rect = &rect;
        art_blit_info.dst_rect = &rect;
        tig_window_blit_art(window_handle, &art_blit_info);
        tig_window_text_write(window_handle, font_desc.str, &rect);
    }
    tig_font_pop();
}

// 0x55BAB0
bool charedit_create_skills_win()
{
    tig_art_id_t art_id;
    TigWindowData window_data;
    TigButtonData button_data;
    TigArtFrameData art_frame_data;
    int index;
    tig_button_handle_t button_handles[CHAREDIT_SKILL_GROUP_COUNT];

    tig_art_interface_id_create(29, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect.x = 503;
    window_data.rect.y = 104;
    window_data.rect.width = art_frame_data.width;
    window_data.rect.height = art_frame_data.height;
    window_data.background_color = 0;
    window_data.message_filter = charedit_skills_win_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

    if (tig_window_create(&window_data, &charedit_skills_win) != TIG_OK) {
        return false;
    }

    button_data.flags = TIG_BUTTON_MOMENTARY | TIG_BUTTON_HIDDEN;
    button_data.window_handle = charedit_skills_win;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    tig_art_interface_id_create(624, 0, 0, 0, &(button_data.art_id));
    for (index = 0; index < SKILL_COUNT; index++) {
        button_data.x = charedit_skills_plus_buttons[index].x;
        button_data.y = charedit_skills_plus_buttons[index].y;
        if (tig_button_create(&button_data, &(charedit_skills_plus_buttons[index].button_handle)) != TIG_OK) {
            tig_window_destroy(charedit_skills_win);
            return false;
        }
    }

    tig_art_interface_id_create(625, 0, 0, 0, &(button_data.art_id));
    for (index = 0; index < SKILL_COUNT; index++) {
        button_data.x = charedit_skills_minus_buttons[index].x;
        button_data.y = charedit_skills_minus_buttons[index].y;
        if (tig_button_create(&button_data, &(charedit_skills_minus_buttons[index].button_handle)) != TIG_OK) {
            tig_window_destroy(charedit_skills_win);
            return false;
        }
    }

    button_data.flags = TIG_BUTTON_TOGGLE | TIG_BUTTON_LATCH;
    for (index = 0; index < CHAREDIT_SKILL_GROUP_COUNT; index++) {
        button_data.x = charedit_skill_group_buttons[index].x;
        button_data.y = charedit_skill_group_buttons[index].y;
        tig_art_interface_id_create(charedit_skill_group_buttons[index].art_num, 0, 0, 0, &(button_data.art_id));
        if (tig_button_create(&button_data, &(charedit_skill_group_buttons[index].button_handle)) != TIG_OK) {
            tig_window_destroy(charedit_skills_win);
            return false;
        }

        button_handles[index] = charedit_skill_group_buttons[index].button_handle;
    }

    tig_button_radio_group_create(CHAREDIT_SKILL_GROUP_COUNT, button_handles, dword_64E020);

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.art_id = TIG_ART_ID_INVALID;

    for (index = 0; index < CHAREDIT_SKILLS_PER_GROUP; index++) {
        button_data.x = charedit_skills_hover_areas[index].x - 503;
        button_data.y = charedit_skills_hover_areas[index].y - 63;
        button_data.width = charedit_skills_hover_areas[index].width;
        button_data.height = charedit_skills_hover_areas[index].height;
        tig_button_create(&button_data, &(charedit_skills_hover_areas[index].button_handle));
        // FIXME: No error checking as seen above.
    }

    return true;
}

// 0x55BD10
void sub_55BD10(int group)
{
    int index;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect rect;
    int training;
    char v1[4][80];
    const char* v2[4];
    int trainings[4];

    for (index = 0; index < SKILL_COUNT; index++) {
        tig_button_hide(charedit_skills_plus_buttons[index].button_handle);
        tig_button_hide(charedit_skills_minus_buttons[index].button_handle);
    }

    dword_64E020 = group;

    if (charedit_mode != CHAREDIT_MODE_PASSIVE) {
        for (index = 0; index < 4; index++) {
            tig_button_show(charedit_skills_plus_buttons[4 * dword_64E020 + index].button_handle);
            tig_button_show(charedit_skills_minus_buttons[4 * dword_64E020 + index].button_handle);
        }
    }

    tig_art_interface_id_create(29, 0, 0, 0, &art_id);

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = art_frame_data.width;
    rect.height = art_frame_data.height;

    art_blit_info.art_id = art_id;
    art_blit_info.flags = 0;
    art_blit_info.src_rect = &rect;
    art_blit_info.dst_rect = &rect;
    tig_window_blit_art(charedit_skills_win, &art_blit_info);

    sub_55B880(charedit_skills_win, charedit_morph15_white_font, &stru_5C82F0[4 * dword_64E020], 0, -1, 4);

    for (index = 0; index < 4; index++) {
        if (dword_64E020 < 3) {
            training = basic_skill_training_get(charedit_obj,
                stru_5C82F0[4 * dword_64E020 + index].value);
        } else {
            training = tech_skill_training_get(charedit_obj,
                stru_5C82F0[4 * dword_64E020 + index].value);
        }

        v2[index] = dword_64CA74[index];
        trainings[index] = training;
    }

    for (index = 0; index < 4; index++) {
        if (trainings[index] != 0) {
            sprintf(v1[index], " (%s)", v2[index]);
        } else {
            v1[index][0] = '\0';
        }

        v2[index] = v1[index];
    }

    sub_55B880(charedit_skills_win, charedit_morph15_white_font, &stru_5C82F0[4 * dword_64E020], v2, -1, 4);
    charedit_refresh_skills_win();
}

// 0x55BF20
void charedit_refresh_skills_win()
{
    tig_art_id_t skills_win_art_id;
    TigArtFrameData skills_win_art_frame_data;
    tig_art_id_t gauge_art_id;
    TigArtFrameData gauge_art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int y;
    int skill_level;
    int index;

    tig_art_interface_id_create(29, 0, 0, 0, &skills_win_art_id);
    if (tig_art_frame_data(skills_win_art_id, &skills_win_art_frame_data) != TIG_OK) {
        return;
    }

    art_blit_info.flags = 0;
    art_blit_info.dst_rect = &dst_rect;
    art_blit_info.src_rect = &src_rect;

    if (tig_art_interface_id_create(623, 0, 0, 0, &gauge_art_id) != TIG_OK) {
        return;
    }

    if (tig_art_frame_data(gauge_art_id, &gauge_art_frame_data) != TIG_OK) {
        return;
    }

    index = 0;
    for (y = 87; y < 351; y += 66) {
        if (dword_64E020 < 3) {
            skill_level = basic_skill_level(charedit_obj,
                stru_5C82F0[dword_64E020 * 4 + index].value);
            basic_skill_cost_inc(charedit_obj,
                stru_5C82F0[dword_64E020 * 4 + index].value);
        } else {
            skill_level = tech_skill_level(charedit_obj,
                stru_5C82F0[dword_64E020 * 4 + index].value);
            tech_skill_cost_inc(charedit_obj,
                stru_5C82F0[dword_64E020 * 4 + index].value);
        }

        src_rect.x = 59;
        src_rect.y = y;
        src_rect.width = gauge_art_frame_data.width;
        src_rect.height = gauge_art_frame_data.height;

        dst_rect.x = 59;
        dst_rect.y = y;
        dst_rect.width = gauge_art_frame_data.width;
        dst_rect.height = gauge_art_frame_data.height;

        art_blit_info.art_id = skills_win_art_id;
        tig_window_blit_art(charedit_skills_win, &art_blit_info);

        if (skill_level != 0) {
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = skill_level * gauge_art_frame_data.width / 20;
            src_rect.height = gauge_art_frame_data.height;

            dst_rect.x = 59;
            dst_rect.y = y;
            dst_rect.width = src_rect.width;
            dst_rect.height = src_rect.height;

            art_blit_info.art_id = gauge_art_id;
            tig_window_blit_art(charedit_skills_win, &art_blit_info);
        }

        index++;
    }
}

// 0x55C110
bool charedit_create_tech_win()
{
    tig_art_id_t art_id;
    TigWindowData window_data;
    TigButtonData button_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    tig_button_handle_t button_handles[TECH_COUNT];
    int index;

    tig_art_interface_id_create(30, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect.x = 503;
    window_data.rect.y = 104;
    window_data.rect.width = art_frame_data.width;
    window_data.rect.height = art_frame_data.height;
    window_data.background_color = 0;
    window_data.message_filter = charedit_tech_win_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

    if (tig_window_create(&window_data, &charedit_tech_win) != TIG_OK) {
        return false;
    }

    window_data.rect.x = 0;
    window_data.rect.y = 0;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &(window_data.rect);
    art_blit_info.dst_rect = &(window_data.rect);
    if (tig_window_blit_art(charedit_tech_win, &art_blit_info) != TIG_OK) {
        tig_window_destroy(charedit_tech_win);
        return false;
    }

    button_data.flags = TIG_BUTTON_TOGGLE | TIG_BUTTON_LATCH;
    button_data.window_handle = charedit_tech_win;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    for (index = 0; index < TECH_COUNT; index++) {
        tig_art_interface_id_create(306 + index, 0, 0, 0, &(button_data.art_id));
        button_data.x = charedit_tech_buttons[index].x;
        button_data.y = charedit_tech_buttons[index].y;
        if (tig_button_create(&button_data, &(charedit_tech_buttons[index].button_handle)) != TIG_OK) {
            tig_window_destroy(charedit_tech_win);
            return false;
        }

        button_handles[index] = charedit_tech_buttons[index].button_handle;
    }

    tig_button_radio_group_create(TECH_COUNT, button_handles, charedit_selected_tech);

    charedit_inc_tech_degree_btn = TIG_BUTTON_HANDLE_INVALID;
    charedit_dec_tech_degree_btn = TIG_BUTTON_HANDLE_INVALID;

    return true;
}

// 0x55C2E0
void charedit_draw_tech_degree_icon(int index)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;

    tig_art_interface_id_create(charedit_tech_degree_icons[charedit_selected_tech], 0, 0, 0, &art_id);
    tig_art_frame_data(art_id, &art_frame_data);

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    dst_rect.x = charedit_tech_degree_icons_x[index];
    dst_rect.y = charedit_tech_degree_icons_y[index];
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(charedit_tech_win, &art_blit_info);
}

// 0x55C3A0
void charedit_refresh_tech_win()
{
    TigButtonData button_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect rect;
    char str[7][80];
    char components[2][80];
    int index;
    tig_art_id_t art_id;
    int degree;
    int next_degree;

    tig_art_interface_id_create(30, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    // TODO: What's the purpose of 25px offset?
    rect.x = 0;
    rect.y = 25;
    rect.width = art_frame_data.width;
    rect.height = art_frame_data.height - 25;

    art_blit_info.art_id = art_id;
    art_blit_info.flags = 0;
    art_blit_info.src_rect = &rect;
    art_blit_info.dst_rect = &rect;

    if (charedit_inc_tech_degree_btn != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_destroy(charedit_inc_tech_degree_btn);
    }

    if (charedit_dec_tech_degree_btn != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_destroy(charedit_dec_tech_degree_btn);
    }

    charedit_inc_tech_degree_btn = TIG_BUTTON_HANDLE_INVALID;
    charedit_dec_tech_degree_btn = TIG_BUTTON_HANDLE_INVALID;

    for (index = 0; index < DEGREE_COUNT; index++) {
        if (charedit_tech_degree_buttons[index].button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_destroy(charedit_tech_degree_buttons[index].button_handle);
            charedit_tech_degree_buttons[index].button_handle = TIG_BUTTON_HANDLE_INVALID;
        }
    }

    tig_window_blit_art(charedit_tech_win, &art_blit_info);

    for (index = 0; index < 3; index++) {
        stru_5C8FC8[index].str = str[0];
    }

    str[0][0] = (char)('1' + charedit_selected_tech);
    str[0][1] = '\0';
    sub_55B880(charedit_tech_win, charedit_icons17_white_font, &(stru_5C8FC8[0]), 0, -1, 1);

    sprintf(str[0],
        "%s %s",
        tech_discipline_name_get(charedit_selected_tech),
        tech_degree_name_get(tech_degree_get(charedit_obj, charedit_selected_tech)));
    sub_55B880(charedit_tech_win, charedit_nick16_white_font, &(stru_5C8FC8[1]), 0, -1, 1);

    sprintf(str[0], "%d", tech_degree_level_get(charedit_obj, charedit_selected_tech));
    sub_55B880(charedit_tech_win, charedit_flare12_blue_font, &(stru_5C8FC8[2]), 0, -1, 1);

    degree = tech_degree_get(charedit_obj, charedit_selected_tech);
    next_degree = degree < 7 ? degree + 1 : degree;

    for (index = 0; index < degree; index++) {
        charedit_draw_tech_degree_icon(index);
    }

    for (index = 1; index - 1 < 7; index++) {
        stru_5C8850[index - 1].str = schematic_ui_learned_schematic_product_name(charedit_selected_tech, index);
        schematic_ui_learned_schematic_component_names(charedit_selected_tech, index, components[0], components[1]);
        sprintf(str[index - 1], "[%s]+[%s]", components[0], components[1]);
        stru_5C88C0[index - 1].str = str[index - 1];
    }

    if (next_degree > degree) {
        if (degree > 0) {
            sub_55B880(charedit_tech_win, charedit_nick16_white_font, stru_5C8850, 0, -1, degree);
            sub_55B880(charedit_tech_win, charedit_garmond9_white_font, stru_5C88C0, 0, -1, degree);
        }

        sub_55B880(charedit_tech_win, charedit_nick16_yellow_font, &(stru_5C8850[degree]), 0, -1, 1);
        sub_55B880(charedit_tech_win, charedit_garmond9_yellow_font, &(stru_5C88C0[degree]), 0, -1, 1);

        if (degree + 1 < 7) {
            sub_55B880(charedit_tech_win, charedit_nick16_gray_font, &stru_5C8850[degree + 1], 0, -1, 6 - degree);
            sub_55B880(charedit_tech_win, charedit_garmond9_gray_font, &stru_5C88C0[degree + 1], 0, -1, 6 - degree);
        }
    } else {
        sub_55B880(charedit_tech_win, charedit_nick16_white_font, stru_5C8850, 0, -1, 7);
        sub_55B880(charedit_tech_win, charedit_garmond9_white_font, stru_5C88C0, 0, -1, 7);
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_tech_win;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    if (next_degree > degree
        && charedit_mode != CHAREDIT_MODE_PASSIVE) {
        tig_art_interface_id_create(647, 0, 0, 0, &(button_data.art_id));
        button_data.x = charedit_tech_degree_icons_x[next_degree - 1];
        button_data.y = charedit_tech_degree_icons_y[next_degree - 1];
        tig_button_create(&button_data, &charedit_inc_tech_degree_btn);
    }

    if (degree > 0
        && charedit_mode != CHAREDIT_MODE_PASSIVE
        && (charedit_mode == CHAREDIT_MODE_CREATE
            || charedit_mode == CHAREDIT_MODE_3
            || dword_64DEEC[charedit_selected_tech] < degree)) {
        tig_art_interface_id_create(648, 0, 0, 0, &(button_data.art_id));
        button_data.x = charedit_tech_degree_icons_x[degree - 1];
        button_data.y = charedit_tech_degree_icons_y[degree - 1];
        tig_button_create(&button_data, &charedit_dec_tech_degree_btn);
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.art_id = TIG_ART_ID_INVALID;

    for (index = 0; index < DEGREE_COUNT; index++) {
        button_data.x = charedit_tech_degree_buttons[index].x - 503;
        button_data.y = charedit_tech_degree_buttons[index].y - 63;
        button_data.width = charedit_tech_degree_buttons[index].width;
        button_data.height = charedit_tech_degree_buttons[index].height;
        tig_button_create(&button_data, &(charedit_tech_degree_buttons[index].button_handle));
    }
}

// 0x55C890
bool charedit_create_spells_win()
{
    tig_art_id_t art_id;
    TigWindowData window_data;
    TigButtonData button_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    int index;
    int art_num;
    tig_button_handle_t button_handles[COLLEGE_COUNT];

    tig_art_interface_id_create(31, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect.x = 503;
    window_data.rect.y = 104;
    window_data.rect.width = art_frame_data.width;
    window_data.rect.height = art_frame_data.height;
    window_data.background_color = 0;
    window_data.message_filter = charedit_spells_win_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

    if (tig_window_create(&window_data, &charedit_spells_win) != TIG_OK) {
        return false;
    }

    window_data.rect.x = 0;
    window_data.rect.y = 0;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &(window_data.rect);
    art_blit_info.dst_rect = &(window_data.rect);
    if (tig_window_blit_art(charedit_spells_win, &art_blit_info) != TIG_OK) {
        tig_window_destroy(charedit_spells_win);
        return false;
    }

    button_data.flags = TIG_BUTTON_TOGGLE | TIG_BUTTON_LATCH;
    button_data.window_handle = charedit_spells_win;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    for (index = 0; index < COLLEGE_COUNT; index++) {
        art_num = spell_college_small_icon(index);
        if (art_num != -1) {
            tig_art_interface_id_create(art_num, 0, 0, 0, &(button_data.art_id));
            button_data.x = charedit_college_buttons[index].x - 503;
            button_data.y = charedit_college_buttons[index].y - 104;
            if (tig_button_create(&button_data, &(charedit_college_buttons[index].button_handle)) != TIG_OK) {
                tig_window_destroy(charedit_spells_win);
                return false;
            }

            button_handles[index] = charedit_college_buttons[index].button_handle;
        }
    }

    tig_button_radio_group_create(COLLEGE_COUNT, button_handles, dword_64E024);

    spell_plus_bid = TIG_BUTTON_HANDLE_INVALID;
    spell_minus_bid = TIG_BUTTON_HANDLE_INVALID;

    return true;
}

// 0x55CA70
void sub_55CA70(int a1, int a2)
{
    int art_num;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigPaletteModifyInfo palette_modify_info;
    TigPalette tmp_palette;

    art_num = spell_icon(a1 + 5 * dword_64E024);
    if (art_num != -1) {
        tig_art_interface_id_create(art_num, 0, 0, 0, &art_id);
        tig_art_frame_data(art_id, &art_frame_data);

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        art_blit_info.art_id = art_id;
        art_blit_info.src_rect = &src_rect;

        if (a1 < a2) {
            art_blit_info.flags = 0;
            tmp_palette = NULL;
        } else {
            tig_art_anim_data(art_id, &art_anim_data);

            tmp_palette = tig_palette_create();
            palette_modify_info.flags = TIG_PALETTE_MODIFY_GRAYSCALE;
            palette_modify_info.src_palette = art_anim_data.palette1;
            palette_modify_info.dst_palette = tmp_palette;
            tig_palette_modify(&palette_modify_info);

            art_blit_info.flags = TIG_ART_BLT_PALETTE_OVERRIDE;
            art_blit_info.palette = tmp_palette;
        }

        dst_rect.x = dword_5C8FF8[a1] - 503;
        dst_rect.y = dword_5C900C[a1] - 104;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(charedit_spells_win, &art_blit_info);

        if (tmp_palette != NULL) {
            tig_palette_destroy(tmp_palette);
        }
    }
}

// 0x55CBC0
void charedit_refresh_spells_win()
{
    tig_art_id_t art_id;
    TigButtonData button_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect rect;
    int index;
    int cnt;
    int v1;
    int spells[5];
    char spell_minimum_levels[5][80];
    int rc;

    tig_art_interface_id_create(31, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    // TODO: What's the purpose of 25px offset?
    rect.x = 0;
    rect.y = 25;
    rect.width = art_frame_data.width;
    rect.height = art_frame_data.height - 25;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &rect;
    art_blit_info.dst_rect = &rect;

    if (spell_plus_bid != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_destroy(spell_plus_bid);
    }

    if (spell_minus_bid != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_destroy(spell_minus_bid);
    }

    spell_plus_bid = TIG_BUTTON_HANDLE_INVALID;
    spell_minus_bid = TIG_BUTTON_HANDLE_INVALID;

    for (index = 0; index < 5; index++) {
        if (stru_5C8DC8[index].button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_destroy(stru_5C8DC8[index].button_handle);
        }
    }

    tig_window_blit_art(charedit_spells_win, &art_blit_info);

    cnt = spell_college_level_get(charedit_obj, dword_64E024);

    // TODO: Refactor v1.
    v1 = cnt;
    if (v1 < 5) {
        v1++;
    }

    for (index = 0; index < 5; index++) {
        sub_55CA70(index, cnt);
    }

    for (index = 0; index < 5; index++) {
        spells[index] = index + 5 * dword_64E024;
        charedit_spell_title_labels[index].str = spell_name(spells[index]);
    }

    if (v1 > cnt) {
        if (cnt > 0) {
            sub_55B880(charedit_spells_win, charedit_morph15_white_font, &(charedit_spell_title_labels[0]), NULL, -1, cnt);
        }

        sub_55B880(charedit_spells_win,
            charedit_mode == CHAREDIT_MODE_PASSIVE ? charedit_morph15_gray_font : charedit_morph15_yellow_font,
            &charedit_spell_title_labels[cnt], NULL, -1, 1);

        if (cnt + 1 < 5) {
            sub_55B880(charedit_spells_win, charedit_morph15_gray_font, &(charedit_spell_title_labels[cnt + 1]), NULL, -1, 4 - cnt);
        }
    } else {
        sub_55B880(charedit_spells_win, charedit_morph15_white_font, &(charedit_spell_title_labels[0]), 0, -1, 5);
    }

    for (index = 0; index < 5; index++) {
        sprintf(spell_minimum_levels[index],
            "%s: %d",
            charedit_minimum_level_str,
            spell_min_level(spells[index]));
        charedit_spell_minimum_level_labels[index].str = spell_minimum_levels[index];
    }

    if (v1 > cnt) {
        if (cnt > 0) {
            sub_55B880(charedit_spells_win, charedit_pork12_white_font, &(charedit_spell_minimum_level_labels[0]), 0, -1, cnt);
        }

        sub_55B880(charedit_spells_win,
            charedit_mode == CHAREDIT_MODE_PASSIVE ? charedit_pork12_gray_font : charedit_pork12_yellow_font,
            &(charedit_spell_minimum_level_labels[cnt]), NULL, -1, 1);

        if (cnt + 1 < 5) {
            sub_55B880(charedit_spells_win, charedit_pork12_gray_font, &(charedit_spell_minimum_level_labels[cnt + 1]), NULL, -1, 4 - cnt);
        }
    } else {
        sub_55B880(charedit_spells_win, charedit_pork12_white_font, &(charedit_spell_minimum_level_labels[0]), 0, -1, 5);
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_spells_win;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    if (v1 > cnt) {
        if (charedit_mode != CHAREDIT_MODE_PASSIVE) {
            tig_art_interface_id_create(647, 0, 0, 0, &(button_data.art_id));
            button_data.x = dword_5C8FF8[v1 - 1] - 503;
            button_data.y = dword_5C900C[v1 - 1] - 104;

            rc = tig_button_create(&button_data, &spell_plus_bid);
            if (rc != TIG_OK) {
                tig_debug_printf("spells_print_all: ERROR: failed to create spell_plus_bid: %d!\n", rc);
            }
        }
    }

    if (cnt > 0
        && charedit_mode != CHAREDIT_MODE_PASSIVE
        && (charedit_mode == CHAREDIT_MODE_CREATE
            || charedit_mode == CHAREDIT_MODE_3
            || dword_64D364[dword_64E024] < cnt)) {
        tig_art_interface_id_create(648, 0, 0, 0, &(button_data.art_id));
        button_data.x = dword_5C8FF8[cnt - 1] - 503;
        button_data.y = dword_5C900C[cnt - 1] - 104;

        rc = tig_button_create(&button_data, &spell_minus_bid);
        if (rc != TIG_OK) {
            tig_debug_printf("spells_print_all: ERROR: failed to create spell_minus_bid: %d!\n", rc);
        }
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_spells_win;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.art_id = TIG_ART_ID_INVALID;

    for (index = 0; index < 5; index++) {
        button_data.x = stru_5C8DC8[index].x - 503;
        button_data.y = stru_5C8DC8[index].y - 63;
        button_data.width = stru_5C8DC8[index].width;
        button_data.height = stru_5C8DC8[index].height;

        rc = tig_button_create(&button_data, &(stru_5C8DC8[index].button_handle));
        if (rc != TIG_OK) {
            tig_debug_printf("spells_print_all: ERROR: failed to create Hover Help Spell Start: %d!\n", rc);
        }
    }
}

// 0x55D060
bool charedit_scheme_win_create()
{
    tig_art_id_t art_id;
    TigWindowData window_data;
    TigButtonData button_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    int index;

    tig_art_interface_id_create(567, 0, 0, 0, &art_id);
    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect.x = 503;
    window_data.rect.y = 104;
    window_data.rect.width = art_frame_data.width;
    window_data.rect.height = art_frame_data.height;
    window_data.background_color = 0;
    window_data.message_filter = charedit_scheme_win_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

    if (tig_window_create(&window_data, &charedit_scheme_win) != TIG_OK) {
        return false;
    }

    window_data.rect.x = 0;
    window_data.rect.y = 0;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &(window_data.rect);
    art_blit_info.dst_rect = &(window_data.rect);
    if (tig_window_blit_art(charedit_scheme_win, &art_blit_info) != TIG_OK) {
        tig_window_destroy(charedit_scheme_win);
        return false;
    }

    charedit_num_schemes = 0;
    for (index = 0; index < 200; index++) {
        charedit_scheme_ids[charedit_num_schemes] = index;
        charedit_scheme_names[charedit_num_schemes] = auto_level_scheme_name(index);
        if (charedit_scheme_names[charedit_num_schemes] != NULL) {
            charedit_num_schemes++;
        }
    }

    charedit_top_scheme_index = 0;

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = charedit_scheme_win;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.art_id = TIG_ART_ID_INVALID;

    for (index = 0; index < 15; index++) {
        button_data.x = stru_5C8E50[index].x - 503;
        button_data.y = stru_5C8E50[index].y - 104;
        button_data.width = 160;
        button_data.height = 16;
        tig_button_create(&button_data, &(dword_64C7E8[index]));
    }

    return true;
}

// 0x55D210
void charedit_scheme_win_refresh()
{
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect rect;
    int cnt;
    int index;

    tig_art_interface_id_create(567, 0, 0, 0, &(art_blit_info.art_id));
    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = art_frame_data.width;
    rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &rect;
    art_blit_info.dst_rect = &rect;
    tig_window_blit_art(charedit_scheme_win, &art_blit_info);

    if (obj_field_int32_get(charedit_obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && (obj_field_int32_get(charedit_obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        if (!multiplayer_level_scheme_name(charedit_obj, byte_64D714)) {
            byte_64D714[0] = '\0';
        }
        stru_5C8E40.str = byte_64D714;
    } else {
        stru_5C8E40.str = auto_level_scheme_name(auto_level_scheme_get(charedit_obj));
    }

    cnt = charedit_num_schemes;
    if (cnt >= 15) {
        cnt = 15;
    }

    for (index = 0; index < cnt; index++) {
        stru_5C8E50[index].str = charedit_scheme_names[charedit_top_scheme_index + index];
    }

    sub_55B880(charedit_scheme_win, charedit_flare12_blue_font, &stru_5C8E40, NULL, -1, 1);
    sub_55B880(charedit_scheme_win, charedit_flare12_white_font, &(stru_5C8E50[0]), NULL, -1, cnt);

    scrollbar_ui_control_redraw(charedit_scheme_scrollbar);
}

// 0x55D3A0
bool charedit_skills_win_message_filter(TigMessage* msg)
{
    int index;

    switch (msg->type) {
    case TIG_MESSAGE_MOUSE:
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_IDLE) {
            charedit_show_hint(charedit_skills_hint);
        }
        break;
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            for (index = 0; index < CHAREDIT_SKILL_GROUP_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skill_group_buttons[index].button_handle) {
                    charedit_skills_hint = index + 135;
                    return true;
                }
            }

            for (index = 0; index < CHAREDIT_SKILLS_PER_GROUP; index++) {
                if (msg->data.button.button_handle == charedit_skills_hover_areas[index].button_handle) {
                    charedit_skills_hint = 4 * dword_64E020 + 1000 + index;
                    return true;
                }
            }

            for (index = 0; index < BASIC_SKILL_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skills_plus_buttons[index].button_handle) {
                    charedit_skills_hint = charedit_skills_plus_buttons[index].art_num + 1000;
                    return true;
                }

                if (msg->data.button.button_handle == charedit_skills_minus_buttons[index].button_handle) {
                    charedit_skills_hint = charedit_skills_minus_buttons[index].art_num + 1000;
                    return true;
                }
            }
            break;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            for (index = 0; index < CHAREDIT_SKILL_GROUP_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skill_group_buttons[index].button_handle) {
                    charedit_skills_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            for (index = 0; index < CHAREDIT_SKILLS_PER_GROUP; index++) {
                if (msg->data.button.button_handle == charedit_skills_hover_areas[index].button_handle) {
                    charedit_skills_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            for (index = 0; index < BASIC_SKILL_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skills_plus_buttons[index].button_handle) {
                    charedit_skills_hint = -1;
                    sub_550720();
                    return true;
                }

                if (msg->data.button.button_handle == charedit_skills_minus_buttons[index].button_handle) {
                    charedit_skills_hint = -1;
                    sub_550720();
                    return true;
                }
            }
            break;
        case TIG_BUTTON_STATE_PRESSED:
            for (index = 0; index < BASIC_SKILL_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skills_plus_buttons[index].button_handle) {
                    if (tig_net_is_active()
                        && !tig_net_is_host()
                        && !multiplayer_is_locked()) {
                        PacketChareditTraitChange pkt;

                        pkt.type = 127;
                        pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC;
                        pkt.trait = MP_CHAREDIT_TRAIT_BASIC_SKILL;
                        pkt.param = charedit_skills_plus_buttons[index].art_num;
                        tig_net_send_app_all(&pkt, sizeof(pkt));

                        return true;
                    }

                    skill_ui_inc_skill(charedit_obj, charedit_skills_plus_buttons[index].art_num);
                    return true;
                }

                if (msg->data.button.button_handle == charedit_skills_minus_buttons[index].button_handle) {
                    // FIX: Original code has mess with plus/minus buttons, but
                    // it does not affect outcome as both plus/minus buttons
                    // refer to the same skills.
                    if (tig_net_is_active()
                        && !tig_net_is_host()
                        && !multiplayer_is_locked()) {
                        PacketChareditTraitChange pkt;

                        pkt.type = 127;
                        pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC;
                        pkt.trait = MP_CHAREDIT_TRAIT_BASIC_SKILL;
                        pkt.param = charedit_skills_minus_buttons[index].art_num;
                        tig_net_send_app_all(&pkt, sizeof(pkt));

                        return true;
                    }

                    if (basic_skill_points_get(charedit_obj, charedit_skills_minus_buttons[index].art_num) == dword_64C7B8[charedit_skills_minus_buttons[index].art_num]) {
                        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SKILL_AT_ACCEPTED_LEVEL];
                        sub_550750(&charedit_error_msg);
                    } else {
                        skill_ui_dec_skill(charedit_obj, charedit_skills_minus_buttons[index].art_num);
                    }

                    return true;
                }
            }

            for (index = 0; index < CHAREDIT_SKILL_GROUP_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skill_group_buttons[index].button_handle) {
                    sub_55BD10(index);
                    return true;
                }
            }
            break;
        }
        break;
    }

    return sub_55D6F0(msg);
}

// 0x55D6F0
bool sub_55D6F0(TigMessage* msg)
{
    int index;
    PacketChareditTraitChange pkt;

    if (msg->type == TIG_MESSAGE_BUTTON) {
        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_INSIDE) {
            for (index = 0; index < TECH_SKILL_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skills_plus_buttons[BASIC_SKILL_COUNT + index].button_handle) {
                    charedit_skills_hint = charedit_skills_plus_buttons[BASIC_SKILL_COUNT + index].art_num + 1012;
                    return true;
                }

                if (msg->data.button.button_handle == charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].button_handle) {
                    charedit_skills_hint = charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].art_num + 1012;
                    return true;
                }
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_OUTSIDE) {
            // NOTE: Original code is buggy.
            for (index = 0; index < TECH_SKILL_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skills_plus_buttons[BASIC_SKILL_COUNT + index].button_handle) {
                    charedit_skills_hint = -1;
                    sub_550720();
                    return true;
                }

                if (msg->data.button.button_handle == charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].button_handle) {
                    charedit_skills_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_PRESSED) {
            for (index = 0; index < TECH_SKILL_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_skills_plus_buttons[BASIC_SKILL_COUNT + index].button_handle) {
                    if (!tig_net_is_active()
                        || tig_net_is_host()
                        || multiplayer_is_locked()) {
                        skill_ui_inc_skill(charedit_obj, charedit_skills_plus_buttons[BASIC_SKILL_COUNT + index].art_num + 12);
                    } else {
                        pkt.type = 127;
                        pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC;
                        pkt.trait = MP_CHAREDIT_TRAIT_TECH_SKILL;
                        pkt.param = charedit_skills_plus_buttons[BASIC_SKILL_COUNT + index].art_num + 12;
                        tig_net_send_app_all(&pkt, sizeof(pkt));
                    }

                    return true;
                }

                if (msg->data.button.button_handle == charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].button_handle) {
                    if (!tig_net_is_active()
                        || tig_net_is_host()
                        || multiplayer_is_locked()) {
                        if (tech_skill_points_get(charedit_obj, charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].art_num) == dword_64C82C[charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].art_num]) {
                            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SKILL_AT_ACCEPTED_LEVEL];
                            sub_550750(&charedit_error_msg);
                        } else {
                            skill_ui_dec_skill(charedit_obj, charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].art_num + 12);
                        }
                    } else {
                        pkt.type = 127;
                        pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC;
                        pkt.trait = MP_CHAREDIT_TRAIT_TECH_SKILL;
                        pkt.param = charedit_skills_minus_buttons[BASIC_SKILL_COUNT + index].art_num + 12;
                        tig_net_send_app_all(&pkt, sizeof(pkt));
                    }

                    return true;
                }
            }

            return false;
        }

        return false;
    }

    return false;
}

// 0x55D940
bool charedit_tech_win_message_filter(TigMessage* msg)
{
    int index;
    PacketChareditTraitChange pkt;
    int v1;

    if (msg->type == TIG_MESSAGE_MOUSE) {
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_IDLE) {
            charedit_show_hint(charedit_tech_hint);
        }

        return false;
    }

    if (msg->type == TIG_MESSAGE_BUTTON) {
        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_INSIDE) {
            for (index = 0; index < TECH_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_tech_buttons[index].button_handle) {
                    charedit_tech_hint = 4000 + index;
                    return true;
                }
            }

            for (index = 0; index < DEGREE_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_tech_degree_buttons[index].button_handle) {
                    if (index == 0) {
                        charedit_tech_hint = 139;
                    } else {
                        charedit_tech_hint = 5000 + 8 * charedit_selected_tech + index;
                    }
                    return true;
                }
            }

            if (msg->data.button.button_handle == charedit_inc_tech_degree_btn) {
                v1 = tech_degree_get(charedit_obj, charedit_selected_tech);
                charedit_tech_hint = 5001 + 8 * charedit_selected_tech + v1;
                return true;
            }

            if (msg->data.button.button_handle == charedit_dec_tech_degree_btn) {
                v1 = tech_degree_get(charedit_obj, charedit_selected_tech);
                charedit_tech_hint = 5000 + 8 * charedit_selected_tech + v1;
                return true;
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_OUTSIDE) {
            for (index = 0; index < TECH_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_tech_buttons[index].button_handle) {
                    charedit_tech_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            for (index = 0; index < DEGREE_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_tech_degree_buttons[index].button_handle) {
                    charedit_tech_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            if (msg->data.button.button_handle == charedit_inc_tech_degree_btn) {
                charedit_tech_hint = -1;
                sub_550720();
                return true;
            }

            if (msg->data.button.button_handle == charedit_dec_tech_degree_btn) {
                charedit_tech_hint = -1;
                sub_550720();
                return true;
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_PRESSED) {
            for (index = 0; index < TECH_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_tech_buttons[index].button_handle) {
                    if (charedit_selected_tech != index) {
                        charedit_selected_tech = index;
                        charedit_refresh_tech_win();
                    }

                    return true;
                }
            }

            if (msg->data.button.button_handle == charedit_inc_tech_degree_btn) {
                if (!tig_net_is_active()
                    || tig_net_is_host()
                    || multiplayer_is_locked()) {
                    tech_ui_inc_degree(charedit_obj, charedit_selected_tech);
                } else {
                    pkt.type = 127;
                    pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC;
                    pkt.trait = MP_CHAREDIT_TRAIT_DEGREE;
                    pkt.param = charedit_selected_tech;
                    tig_net_send_app_all(&pkt, sizeof(pkt));
                }

                return true;
            }

            if (msg->data.button.button_handle == charedit_dec_tech_degree_btn) {
                if (!tig_net_is_active()
                    || tig_net_is_host()
                    || multiplayer_is_locked()) {
                    tech_ui_dec_degree(charedit_obj, charedit_selected_tech);
                } else {
                    pkt.type = 127;
                    pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC;
                    pkt.trait = MP_CHAREDIT_TRAIT_DEGREE;
                    pkt.param = charedit_selected_tech;
                    tig_net_send_app_all(&pkt, sizeof(pkt));
                }
            }
        }

        return false;
    }

    return false;
}

// 0x55DC60
bool charedit_spells_win_message_filter(TigMessage* msg)
{
    int index;
    PacketChareditTraitChange pkt;
    int v1;

    if (msg->type == TIG_MESSAGE_MOUSE) {
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_IDLE) {
            charedit_show_hint(charedit_spells_hint);
        }

        return false;
    }

    if (msg->type == TIG_MESSAGE_BUTTON) {
        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_INSIDE) {
            for (index = 0; index < COLLEGE_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_college_buttons[index].button_handle) {
                    charedit_spells_hint = 3000 + index;
                    return true;
                }
            }

            for (index = 0; index < 5; index++) {
                if (msg->data.button.button_handle == stru_5C8DC8[index].button_handle) {
                    charedit_spells_hint = 2000 + 5 * dword_64E024 + index;
                    return true;
                }
            }

            if (msg->data.button.button_handle == spell_plus_bid) {
                charedit_spells_hint = spell_college_level_get(charedit_obj, dword_64E024) + 5 * (dword_64E024 + 400);
                return true;
            }

            if (msg->data.button.button_handle == spell_minus_bid) {
                charedit_spells_hint = spell_college_level_get(charedit_obj, dword_64E024) - 1 + 5 * (dword_64E024 + 400);
                return true;
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_OUTSIDE) {
            for (index = 0; index < COLLEGE_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_college_buttons[index].button_handle) {
                    charedit_spells_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            for (index = 0; index < 5; index++) {
                if (msg->data.button.button_handle == stru_5C8DC8[index].button_handle) {
                    charedit_spells_hint = -1;
                    sub_550720();
                    return true;
                }
            }

            if (msg->data.button.button_handle == spell_plus_bid) {
                charedit_spells_hint = -1;
                sub_550720();
                return true;
            }

            if (msg->data.button.button_handle == spell_minus_bid) {
                charedit_spells_hint = -1;
                sub_550720();
                return true;
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_PRESSED) {
            for (index = 0; index < COLLEGE_COUNT; index++) {
                if (msg->data.button.button_handle == charedit_college_buttons[index].button_handle) {
                    if (dword_64E024 != index) {
                        dword_64E024 = index;
                        charedit_refresh_spells_win();
                    }

                    return true;
                }
            }

            if (msg->data.button.button_handle == spell_plus_bid) {
                if (!tig_net_is_active()
                    || tig_net_is_host()
                    || multiplayer_is_locked()) {
                    v1 = spell_college_level_get(charedit_obj, dword_64E024);
                    spell_ui_add(charedit_obj, 5 * dword_64E024 + v1);
                } else {
                    pkt.type = 127;
                    pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_INC;
                    pkt.trait = MP_CHAREDIT_TRAIT_COLLEGE;
                    pkt.param = dword_64E024;
                    tig_net_send_app_all(&pkt, sizeof(pkt));
                }

                return true;
            }

            if (msg->data.button.button_handle == spell_minus_bid) {
                if (!tig_net_is_active()
                    || tig_net_is_host()
                    || multiplayer_is_locked()) {
                    v1 = spell_college_level_get(charedit_obj, dword_64E024);
                    if (v1 == dword_64D364[dword_64E024]) {
                        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SPELL_AT_ACCEPTED_LEVEL];
                        sub_550750(&charedit_error_msg);
                    } else {
                        spell_ui_remove(charedit_obj, 5 * dword_64E024 + v1 - 1);
                    }
                } else {
                    pkt.type = 127;
                    pkt.subtype = PACKET_CHAREDIT_TRAIT_CHANGE_SUBTYPE_DEC;
                    pkt.trait = MP_CHAREDIT_TRAIT_COLLEGE;
                    pkt.param = dword_64E024;
                    tig_net_send_app_all(&pkt, sizeof(pkt));
                }

                return true;
            }

            return false;
        }

        return false;
    }

    return false;
}

// 0x55DF90
bool charedit_scheme_win_message_filter(TigMessage* msg)
{
    int index;

    if (msg->type == TIG_MESSAGE_BUTTON) {
        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_INSIDE) {
            for (index = 0; index < 15; index++) {
                if (msg->data.button.button_handle == dword_64C7E8[index]) {
                    if (index < charedit_num_schemes) {
                        sub_55B880(charedit_scheme_win, charedit_flare12_blue_font, &stru_5C8E50[index], NULL, -1, 1);
                    }

                    return true;
                }
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_MOUSE_OUTSIDE) {
            for (index = 0; index < 15; index++) {
                if (msg->data.button.button_handle == dword_64C7E8[index]) {
                    if (index < charedit_num_schemes) {
                        sub_55B880(charedit_scheme_win, charedit_flare12_white_font, &stru_5C8E50[index], NULL, -1, 1);
                    }

                    return true;
                }
            }

            return false;
        }

        if (msg->data.button.state == TIG_BUTTON_STATE_PRESSED) {
            for (index = 0; index < 15; index++) {
                if (msg->data.button.button_handle == dword_64C7E8[index]) {
                    if (index < charedit_num_schemes) {
                        auto_level_scheme_set(charedit_obj, charedit_scheme_ids[index + charedit_top_scheme_index]);
                        if (tig_net_is_active()) {
                            multiplayer_notify_level_scheme_changed(player_get_local_pc_obj());
                        }
                        charedit_scheme_win_refresh();
                    }

                    return true;
                }
            }

            if (msg->data.button.button_handle == dword_64CDD8) {
                if (charedit_top_scheme_index > 0) {
                    charedit_top_scheme_index--;
                    charedit_scheme_win_refresh();
                }

                return true;
            }

            if (msg->data.button.button_handle == dword_64D3AC) {
                if (charedit_top_scheme_index < charedit_num_schemes - 15) {
                    charedit_top_scheme_index++;
                    charedit_scheme_win_refresh();
                }

                return true;
            }

            return false;
        }

        return false;
    }

    return false;
}

// 0x55E110
bool charedit_labels_init()
{
    int num;
    int index;
    MesFileEntry mes_file_entry;
    TigFont font_desc;

    if (!mes_load("mes\\charedit.mes", &charedit_mes_file)) {
        return false;
    }

    num = 0;

    for (index = 0; index < 9; index++) {
        mes_file_entry.num = num++;
        mes_get_msg(charedit_mes_file, &mes_file_entry);
        stru_5C8150[index].str = mes_file_entry.str;
    }

    // Skip "Base" and "Adj."
    for (index = 0; index < 2; index++) {
        mes_file_entry.num = num++;
        mes_get_msg(charedit_mes_file, &mes_file_entry);
    }

    // Skip "Total".
    num++;

    for (index = 0; index < 13; index++) {
        if (index < 8) {
            stru_5C81E0[index].str = stat_name(stru_5C81E0[index].value);
        } else {
            mes_file_entry.num = num++;
            mes_get_msg(charedit_mes_file, &mes_file_entry);
            stru_5C81E0[index].str = mes_file_entry.str;
        }
    }

    for (index = 0; index < CHAREDIT_ERR_COUNT; index++) {
        mes_file_entry.num = num++;
        mes_get_msg(charedit_mes_file, &mes_file_entry);
        charedit_errors[index] = mes_file_entry.str;
    }

    mes_file_entry.num = num++;
    mes_get_msg(charedit_mes_file, &mes_file_entry);
    charedit_fatigue_str = mes_file_entry.str;

    mes_file_entry.num = num++;
    mes_get_msg(charedit_mes_file, &mes_file_entry);
    charedit_quest_str = mes_file_entry.str;

    mes_file_entry.num = num++;
    mes_get_msg(charedit_mes_file, &mes_file_entry);
    charedit_second_str = mes_file_entry.str;

    mes_file_entry.num = num++;
    mes_get_msg(charedit_mes_file, &mes_file_entry);
    charedit_seconds_str = mes_file_entry.str;

    mes_file_entry.num = num++;
    mes_get_msg(charedit_mes_file, &mes_file_entry);
    charedit_minimum_level_str = mes_file_entry.str;

    for (index = 0; index < BASIC_SKILL_COUNT; index++) {
        stru_5C82F0[index].str = basic_skill_name(index);
    }

    for (index = 0; index < TECH_SKILL_COUNT; index++) {
        stru_5C82F0[BASIC_SKILL_COUNT + index].str = tech_skill_name(index);
    }

    for (index = 0; index < TRAINING_COUNT; index++) {
        dword_64CA74[index] = training_name(index);
    }

    for (index = 0; index < 4; index++) {
        stru_5C83F0[index].str = stru_5C8150[0].str;
    }

    font_desc.flags = 0;
    tig_art_interface_id_create(26, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_arial10_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_morph15_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 210, 0);
    tig_font_create(&font_desc, &charedit_morph15_yellow_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(128, 128, 128);
    tig_font_create(&font_desc, &charedit_morph15_gray_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(0, 255, 0);
    tig_font_create(&font_desc, &charedit_morph15_green_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 0, 0);
    tig_font_create(&font_desc, &charedit_morph15_red_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(28, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_pork12_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(28, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 0);
    tig_font_create(&font_desc, &charedit_pork12_yellow_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(28, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(128, 128, 128);
    tig_font_create(&font_desc, &charedit_pork12_gray_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(171, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_cloister18_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(300, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_icons17_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(301, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_nick16_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(301, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 0);
    tig_font_create(&font_desc, &charedit_nick16_yellow_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(301, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(128, 128, 128);
    tig_font_create(&font_desc, &charedit_nick16_gray_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(0, 0, 255);
    tig_font_create(&font_desc, &charedit_flare12_blue_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_flare12_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(483, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &charedit_garmond9_white_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(483, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 0);
    tig_font_create(&font_desc, &charedit_garmond9_yellow_font);

    font_desc.flags = 0;
    tig_art_interface_id_create(483, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(128, 128, 128);
    tig_font_create(&font_desc, &charedit_garmond9_gray_font);

    return true;
}

// 0x55EBA0
void charedit_labels_exit()
{
    mes_unload(charedit_mes_file);
    tig_font_destroy(charedit_arial10_white_font);
    tig_font_destroy(charedit_morph15_white_font);
    tig_font_destroy(charedit_morph15_yellow_font);
    tig_font_destroy(charedit_morph15_gray_font);
    tig_font_destroy(charedit_morph15_green_font);
    tig_font_destroy(charedit_morph15_red_font);
    tig_font_destroy(charedit_pork12_white_font);
    tig_font_destroy(charedit_pork12_yellow_font);
    tig_font_destroy(charedit_pork12_gray_font);
    tig_font_destroy(charedit_cloister18_white_font);
    tig_font_destroy(charedit_icons17_white_font);
    tig_font_destroy(charedit_nick16_white_font);
    tig_font_destroy(charedit_nick16_yellow_font);
    tig_font_destroy(charedit_nick16_gray_font);
    tig_font_destroy(charedit_flare12_blue_font);
    tig_font_destroy(charedit_flare12_white_font);
    tig_font_destroy(charedit_garmond9_white_font);
    tig_font_destroy(charedit_garmond9_yellow_font);
    tig_font_destroy(charedit_garmond9_gray_font);
}

// 0x55EC90
void charedit_refresh_alignment_aptitude_bars()
{
    int index;
    char buffer[3][10];
    const char* labels[3];
    int value;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;

    for (index = 0; index < 3; index++) {
        labels[index] = buffer[index];
    }

    value = stat_level_get(charedit_obj, STAT_ALIGNMENT) / 10;
    sprintf(buffer[0], "%d", value);

    tig_art_interface_id_create(254, value / 10 + 10, 0, 0, &(art_blit_info.art_id));
    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    dst_rect.x = 56 - art_frame_data.hot_x;
    dst_rect.y = 181 - art_frame_data.hot_y;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(charedit_window_handle, &art_blit_info);

    tig_art_interface_id_create(255, 0, 0, 0, &(art_blit_info.art_id));
    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    tig_art_interface_id_create(22, 0, 0, 0, &(art_blit_info.art_id));

    src_rect.x = 775;
    src_rect.y = 87;
    src_rect.width = art_frame_data.width;
    src_rect.height = 278;

    dst_rect.x = 775;
    dst_rect.y = 87;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = 278;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(charedit_window_handle, &art_blit_info);

    value = stat_level_get(charedit_obj, STAT_MAGICK_TECH_APTITUDE);
    if (value < 0) {
        sprintf(buffer[2], "%d", -value);
        strcpy(buffer[1], "0");
    } else {
        sprintf(buffer[1], "%d", value);
        strcpy(buffer[2], "0");
    }

    tig_art_interface_id_create(255, 0, 0, 0, &(art_blit_info.art_id));

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    dst_rect.x = 775;
    dst_rect.y = 221 - (value * 128) / 100;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(charedit_window_handle, &art_blit_info);

    tig_art_interface_id_create(22, 0, 0, 0, &(art_blit_info.art_id));

    art_blit_info.src_rect = &stru_5C8970;
    art_blit_info.dst_rect = &stru_5C8970;
    art_blit_info.flags = 0;
    tig_window_blit_art(charedit_window_handle, &art_blit_info);

    art_blit_info.src_rect = &stru_5C8980;
    art_blit_info.dst_rect = &stru_5C8980;
    tig_window_blit_art(charedit_window_handle, &art_blit_info);

    sub_55B880(charedit_window_handle, charedit_cloister18_white_font, stru_5C8940, labels, -1, 3);
}

// 0x55EFB0
void sub_55EFB0()
{
    if (charedit_scheme_scrollbar_initialized) {
        scrollbar_ui_control_destroy(charedit_scheme_scrollbar);
        charedit_scheme_scrollbar_initialized = false;
    }
}

// 0x55EFE0
void sub_55EFE0()
{
    sub_55EFF0();
}

// 0x55EFF0
void sub_55EFF0()
{
    ScrollbarUiControlInfo sb;

    if (!charedit_scheme_scrollbar_initialized) {
        sb.flags = 0x39F;
        sb.value = charedit_top_scheme_index;
        sb.field_3C = charedit_scheme_scrollbar_value_changed;
        if (charedit_num_schemes > 15) {
            sb.max_value = charedit_num_schemes - 15;
        } else {
            sb.max_value = 0;
        }
        sb.scrollbar_rect = stru_5C8F40;
        sb.min_value = 0;
        sb.content_rect.width = stru_5C8F40.width + 160;
        sb.content_rect.x = 23;
        sb.content_rect.y = 66;
        sb.content_rect.height = 240;
        sb.field_40 = charedit_scheme_scrollbar_refresh_rect;
        sb.field_2C = 1;
        scrollbar_ui_control_create(&charedit_scheme_scrollbar, &sb, charedit_scheme_win);
        scrollbar_ui_control_redraw(charedit_scheme_scrollbar);
        charedit_scheme_scrollbar_initialized = true;
    }
}

// 0x55F0D0
void sub_55F0D0()
{
    sub_55EFB0();
}

// 0x55F0E0
void charedit_scheme_scrollbar_value_changed(int value)
{
    if (charedit_scheme_scrollbar_initialized) {
        charedit_top_scheme_index = value;
        if (dword_64E01C == 3) {
            charedit_scheme_win_refresh();
        }
    }
}

// 0x55F110
void charedit_scheme_scrollbar_refresh_rect(TigRect* rect)
{
    TigArtBlitInfo blit_info;

    if (charedit_scheme_scrollbar_initialized) {
        tig_art_interface_id_create(567, 0, 0, 0, &(blit_info.art_id));
        blit_info.flags = 0;
        blit_info.src_rect = rect;
        blit_info.dst_rect = rect;
        tig_window_blit_art(charedit_scheme_win, &blit_info);
    }
}

// 0x55F160
void charedit_error_not_enough_character_points()
{
    if (!charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_CHARACTER_POINTS];
    sub_550750(&charedit_error_msg);
}

// 0x55F180
void charedit_error_not_enough_level()
{
    if (!charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_LEVEL];
    sub_550750(&charedit_error_msg);
}

// 0x55F1A0
void charedit_error_not_enough_intelligence()
{
    if (!charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_INTELLIGENCE];
    sub_550750(&charedit_error_msg);
}

// 0x55F1C0
void charedit_error_not_enough_willpower()
{
    if (!charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_WILLPOWER];
    sub_550750(&charedit_error_msg);
}

// 0x55F1E0
void charedit_error_skill_at_max()
{
    if (!charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SKILL_AT_MAX];
    sub_550750(&charedit_error_msg);
}

// 0x55F200
void charedit_error_not_enough_stat(int stat)
{
    if (!charedit_created) {
        return;
    }

    switch (stat) {
    case STAT_STRENGTH:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_STRENGTH];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_DEXTERITY:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_DEXTERITY];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_CONSTITUTION:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_CONSTITUTION];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_BEAUTY:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_BEAUTY];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_INTELLIGENCE:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_INTELLIGENCE];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_PERCEPTION:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_PERCEPTION];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_WILLPOWER:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_WILLPOWER];
        sub_550750(&charedit_error_msg);
        break;
    case STAT_CHARISMA:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_CHARISMA];
        sub_550750(&charedit_error_msg);
        break;
    default:
        charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_STATS];
        sub_550750(&charedit_error_msg);
        break;
    }
}

// 0x55F320
void charedit_error_skill_is_zero()
{
    if (!charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NO_POINTS_IN_SKILL];
    sub_550750(&charedit_error_msg);
}

// 0x55F340
void charedit_error_skill_at_min()
{
    if (charedit_created) {
        return;
    }

    charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SKILL_AT_MIN];
    sub_550750(&charedit_error_msg);
}

// 0x55F360
void mp_charedit_cache_traits(int player)
{
    int64_t obj;
    int index;

    obj = sub_4A2B60(player);
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    for (index = 0; index < CHAREDIT_STAT_COUNT; index++) {
        charedit_player_stats_tbl[player][index] = charedit_stat_value_get(obj, index);
    }

    for (index = 0; index < TECH_COUNT; index++) {
        charedit_player_tech_degrees_tbl[player][index] = tech_degree_get(obj, index);
    }

    for (index = 0; index < COLLEGE_COUNT; index++) {
        charedit_player_spell_college_levels_tbl[player][index] = spell_college_level_get(obj, index);
    }

    for (index = 0; index < BASIC_SKILL_COUNT; index++) {
        charedit_player_basic_skills_tbl[player][index] = basic_skill_points_get(obj, index);
    }

    for (index = 0; index < TECH_SKILL_COUNT; index++) {
        charedit_player_tech_skills_tbl[player][index] = tech_skill_points_get(obj, index);
    }
}

// 0x55F450
void mp_charedit_trait_inc(int player, int trait, int param)
{
    int64_t obj;
    int value;
    int cost;
    int unspent_points;

    obj = sub_4A2B60(player);
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    switch (trait) {
    case MP_CHAREDIT_TRAIT_STAT:
        value = charedit_stat_value_get(obj, param) + 1;
        if (param == CHAREDIT_STAT_HP_PTS_MAXIMUM || param == CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM) {
            cost = 1;
        } else {
            int stat;

            stat = charedit_stat_map_to_critter_stat(param);
            if (stat_level_get(obj, stat) > stat_level_max(obj, stat)) {
                charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_MAX];
                sub_4EDA60(&charedit_error_msg, player, 0);
                return;
            }

            cost = stat_cost(value);
        }

        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        if (cost > unspent_points) {
            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_NOT_ENOUGH_CHARACTER_POINTS];
            sub_4EDA60(&charedit_error_msg, player, 0);
            return;
        }

        charedit_stat_value_set(obj, param, value);

        if (charedit_stat_value_get(obj, param) < value) {
            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_MAX];
            sub_4EDA60(&charedit_error_msg, player, 0);
            return;
        }

        stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - cost);
        break;
    case MP_CHAREDIT_TRAIT_BASIC_SKILL:
    case MP_CHAREDIT_TRAIT_TECH_SKILL:
        skill_ui_inc_skill(obj, param);
        break;
    case MP_CHAREDIT_TRAIT_DEGREE:
        tech_ui_inc_degree(obj, param);
        break;
    case MP_CHAREDIT_TRAIT_COLLEGE:
        spell_ui_add(obj, spell_college_level_get(obj, param) + 5 * param);
        break;
    }
}

// 0x55F5F0
void mp_charedit_trait_dec(int player, int trait, int param)
{
    int64_t obj;
    int value;
    int cost;
    int unspent_points;

    obj = sub_4A2B60(player);
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    switch (trait) {
    case MP_CHAREDIT_TRAIT_STAT:
        value = charedit_stat_value_get(obj, param);
        if (value == charedit_player_stats_tbl[player][param]) {
            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_ACCEPTED_LEVEL];
            sub_4EDA60(&charedit_error_msg, player, 0);
            return;
        }

        if (param == CHAREDIT_STAT_HP_PTS_MAXIMUM || param == CHAREDIT_STAT_FATIGUE_PTS_MAXIMUM) {
            cost = 1;
        } else {
            cost = stat_cost(value);
        }

        value--;

        charedit_stat_value_set(obj, param, value);
        if (charedit_stat_value_get(obj, param) > value) {
            if (value == 0) {
                charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_AT_MIN];
                sub_4EDA60(&charedit_error_msg, player, 0);
                return;
            }

            if (dword_5C8124[param] != -1
                && !skill_check_stat(obj, dword_5C8124[param], value - 1)) {
                charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_STAT_SKILL_PREREQUISITE];
                sub_4EDA60(&charedit_error_msg, player, 0);
                return;
            }

            if (stru_5C8028[param].art_num == CHAREDIT_STAT_INTELLIGENCE_BASE
                && !spell_check_intelligence(obj, value - 1)) {
                charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_INTELLIGENCE_SPELL_PREREQUISITE];
                sub_4EDA60(&charedit_error_msg, player, 0);
                return;
            }

            if (stru_5C8028[param].art_num == CHAREDIT_STAT_INTELLIGENCE_BASE
                && !tech_check_intelligence(obj, value - 1)) {
                charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_INTELLIGENCE_TECH_PREREQUISITE];
                sub_4EDA60(&charedit_error_msg, player, 0);
                return;
            }

            if (stru_5C8028[param].art_num == CHAREDIT_STAT_WILLPOWER_BASE
                && !spell_check_willpower(obj, value - 1)) {
                charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_WILLPOWER_SPELL_PREREQUISITE];
                sub_4EDA60(&charedit_error_msg, player, 0);
                return;
            }

            // Something else is wrong.
            return;
        }

        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points + cost);
        break;
    case MP_CHAREDIT_TRAIT_BASIC_SKILL:
        if (basic_skill_points_get(obj, GET_BASIC_SKILL(param)) == charedit_player_basic_skills_tbl[player][param]) {
            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SKILL_AT_ACCEPTED_LEVEL];
            sub_4EDA60(&charedit_error_msg, player, 0);
            return;
        }
        skill_ui_dec_skill(obj, param);
        break;
    case MP_CHAREDIT_TRAIT_TECH_SKILL:
        if (tech_skill_points_get(obj, GET_TECH_SKILL(param)) == charedit_player_tech_skills_tbl[player][param]) {
            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SKILL_AT_ACCEPTED_LEVEL];
            sub_4EDA60(&charedit_error_msg, player, 0);
            return;
        }
        skill_ui_dec_skill(obj, param);
        break;
    case MP_CHAREDIT_TRAIT_DEGREE:
        tech_ui_dec_degree(obj, param);
        break;
    case MP_CHAREDIT_TRAIT_COLLEGE:
        value = spell_college_level_get(obj, param);
        if (value == charedit_player_spell_college_levels_tbl[player][param]) {
            charedit_error_msg.str = charedit_errors[CHAREDIT_ERR_SPELL_AT_ACCEPTED_LEVEL];
            sub_4EDA60(&charedit_error_msg, player, 0);
            return;
        }

        spell_ui_remove(obj, value + 5 * param - 1);
        break;
    }
}
