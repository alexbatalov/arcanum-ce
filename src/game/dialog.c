#include "game/dialog.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/area.h"
#include "game/critter.h"
#include "game/item.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/newspaper.h"
#include "game/player.h"
#include "game/quest.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/reputation.h"
#include "game/rumor.h"
#include "game/script.h"
#include "game/script_name.h"
#include "game/skill.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/tc.h"
#include "game/timeevent.h"
#include "game/ui.h"

typedef enum GeneratedDialog {
    GD_PC2M,
    GD_PC2F,
    GD_CLS_M2M,
    GD_CLS_M2F,
    GD_CLS_F2F,
    GD_CLS_F2M,
    GD_CLS_PC2M,
    GD_CLS_PC2F,
    GD_RCE_M2M,
    GD_RCE_M2F,
    GD_RCE_F2F,
    GD_RCE_F2M,
    GD_NPC_M2M,
    GD_NPC_M2F,
    GD_NPC_F2F,
    GD_NPC_F2M,
    GD_DUMB_PC2M,
    GD_DUMB_PC2F,
    GD_CLS_DUMB_PC2M,
    GD_CLS_DUMB_PC2F,
    GD_CLS_DUMB_M2M,
    GD_CLS_DUMB_M2F,
    GD_CLS_DUMB_F2F,
    GD_CLS_DUMB_F2M,
    GD_RCE_DUMB_M2M,
    GD_RCE_DUMB_M2F,
    GD_RCE_DUMB_F2F,
    GD_RCE_DUMB_F2M,
    GD_STO_M2M,
    GD_STO_M2F,
    GD_STO_F2F,
    GD_STO_F2M,
    GD_COUNT,
} GeneratedDialog;

typedef enum DialogCondition {
    DIALOG_COND_PS,
    DIALOG_COND_CH,
    DIALOG_COND_PE,
    DIALOG_COND_AL,
    DIALOG_COND_MA,
    DIALOG_COND_TA,
    DIALOG_COND_GV,
    DIALOG_COND_GF,
    DIALOG_COND_QU,
    DIALOG_COND_RE,
    DIALOG_COND_GOLD,
    DIALOG_COND_IN,
    DIALOG_COND_HA,
    DIALOG_COND_LF,
    DIALOG_COND_LC,
    DIALOG_COND_TR,
    DIALOG_COND_SK,
    DIALOG_COND_RU,
    DIALOG_COND_RQ,
    DIALOG_COND_FO,
    DIALOG_COND_LE,
    DIALOG_COND_QB,
    DIALOG_COND_ME,
    DIALOG_COND_NI,
    DIALOG_COND_QA,
    DIALOG_COND_RA,
    DIALOG_COND_PA,
    DIALOG_COND_SS,
    DIALOG_COND_WA,
    DIALOG_COND_WT,
    DIALOG_COND_PV,
    DIALOG_COND_PF,
    DIALOG_COND_NA,
    DIALOG_COND_AR,
    DIALOG_COND_RP,
    DIALOG_COND_IA,
    DIALOG_COND_SC,
    DIALOG_COND_COUNT,
} DialogCondition;

typedef enum DialogAction {
    DIALOG_ACTION_GOLD,
    DIALOG_ACTION_RE,
    DIALOG_ACTION_QU,
    DIALOG_ACTION_FL,
    DIALOG_ACTION_CO,
    DIALOG_ACTION_GV,
    DIALOG_ACTION_GF,
    DIALOG_ACTION_MM,
    DIALOG_ACTION_AL,
    DIALOG_ACTION_IN,
    DIALOG_ACTION_LF,
    DIALOG_ACTION_LC,
    DIALOG_ACTION_TR,
    DIALOG_ACTION_RU,
    DIALOG_ACTION_RQ,
    DIALOG_ACTION_JO,
    DIALOG_ACTION_WA,
    DIALOG_ACTION_LV,
    DIALOG_ACTION_SS,
    DIALOG_ACTION_SC,
    DIALOG_ACTION_SO,
    DIALOG_ACTION_UW,
    DIALOG_ACTION_PV,
    DIALOG_ACTION_PF,
    DIALOG_ACTION_XP,
    DIALOG_ACTION_NK,
    DIALOG_ACTION_RP,
    DIALOG_ACTION_NP,
    DIALOG_ACTION_CE,
    DIALOG_ACTION_FP,
    DIALOG_ACTION_SU,
    DIALOG_ACTION_OR,
    DIALOG_ACTION_II,
    DIALOG_ACTION_RI,
    DIALOG_ACTION_ET,
    DIALOG_ACTION_COUNT,
} DialogAction;

typedef enum DialogHealingOfferType {
    DIALOG_HEALING_OFFER_OPTIONS,
    DIALOG_HEALING_OFFER_MAGICKAL_HEALING,
    DIALOG_HEALING_OFFER_MAGICKAL_POISON_HEALING,
} DialogHealingOfferType;

typedef struct DialogFileEntry {
    /* 0000 */ int num;
    /* 0004 */ char* str;
    union {
        /* 0008 */ int gender;
        /* 0008 */ char* female_str;
    } data;
    /* 000C */ int iq;
    /* 0010 */ char* conditions;
    /* 0014 */ int response_val;
    /* 0018 */ char* actions;
} DialogFileEntry;

typedef struct DialogFile {
    /* 0000 */ char path[TIG_MAX_PATH];
    /* 0104 */ int refcount;
    /* 0108 */ DateTime timestamp;
    /* 0110 */ int entries_length;
    /* 0114 */ int entries_capacity;
    /* 0118 */ DialogFileEntry* entries;
} DialogFile;

static void dialog_state_init(int64_t npc_obj, int64_t pc_obj, DialogState* state);
static void sub_414810(int a1, int a2, int a3, int a4, DialogState* a5);
static void sub_414E60(DialogState* a1, bool randomize);
static int sub_414F50(DialogState* a1, int* a2);
static bool sub_4150D0(DialogState* a1, char* a2);
static bool sub_415BA0(DialogState* a1, char* a2, int a3);
static int sub_4167C0(const char* str);
static bool sub_416840(DialogState* a1, bool a2);
static bool dialog_search(int dlg, DialogFileEntry* entry);
static void sub_416B00(char* dst, char* src, DialogState* a3);
static bool sub_416C10(int a1, int a2, DialogState* a3);
static void sub_417590(int a1, int* a2, int* a3);
static bool find_dialog(const char* path, int* index_ptr);
static void dialog_load_internal(DialogFile* dialog);
static bool dialog_parse_entry(TigFile* stream, DialogFileEntry* entry, int* line_ptr);
static bool dialog_parse_field(TigFile* stream, char* str, int* line_ptr);
static TigFile* dialog_file_fopen(const char* fname, const char* mode);
static int dialog_file_fclose(TigFile* stream);
static int dialog_file_fgetc(TigFile* stream);
static int dialog_entry_compare(const void* va, const void* vb);
static void dialog_entry_copy(DialogFileEntry* dst, const DialogFileEntry* src);
static void dialog_entry_free(DialogFileEntry* entry);
static int dialog_parse_params(int* values, char* str);
static void dialog_check_generated(int gd);
static void dialog_load_generated(int gd);
static void dialog_copy_pc_generic_msg(char* buffer, DialogState* state, int start, int end);
static void dialog_copy_pc_class_specific_msg(char* buffer, DialogState* state, int num);
static void dialog_copy_pc_story_msg(char* buffer, DialogState* state);
static void dialog_copy_npc_class_specific_msg(char* buffer, DialogState* state, int num);
static void dialog_copy_npc_race_specific_msg(char* buffer, DialogState* state, int num);
static void dialog_copy_npc_generic_msg(char* buffer, DialogState* state, int start, int end);
static bool dialog_copy_npc_override_msg(char* buffer, DialogState* state, int num);
static int sub_4189C0(const char* a1, int a2);
static void dialog_ask_money(int amt, int param1, int param2, int a4, int a5, DialogState* state);
static void sub_418B30(int a1, DialogState* a2);
static void sub_418C40(int a1, int a2, int a3, DialogState* a4);
static void dialog_offer_training(int* skills, int cnt, int back_response_val, DialogState* state);
static void dialog_ask_money_for_training(int skill, DialogState* state);
static void dialog_perform_training(int skill, DialogState* state);
static void dialog_ask_money_for_rumor(int cost, int* rumors, int num_rumors, int response_val, DialogState* state);
static void dialog_tell_rumor(int rumor, int a2, int a3, DialogState* state);
static void dialog_build_pc_insult_option(int a1, int a2, int a3, DialogState* a4);
static void dialog_insult_reply(int a1, int a2, DialogState* state);
static void sub_419260(DialogState* a1, const char* a2);
static bool sub_4197D0(unsigned int flags, int a2, DialogState* a3);
static void dialog_offer_healing(DialogHealingOfferType type, int response_val, DialogState* state);
static void dialog_build_use_skill_option(int index, int skill, int response_val, DialogState* state);
static void dialog_build_use_spell_option(int index, int spell, int response_val, DialogState* state);
static void dialog_ask_money_for_skill(int skill, int response_val, DialogState* state);
static void dialog_ask_money_for_spell(int a1, int a2, DialogState* a3);
static void dialog_use_skill(int a1, int a2, int a3, DialogState* a4);
static void dialog_use_spell(int spell, int a2, int a3, DialogState* state);
static int filter_unknown_areas(int64_t obj, int* areas, int cnt);
static void dialog_offer_directions(const char* str, int response_val, int offset, bool mark, DialogState* state);
static void dialog_ask_money_for_directions(int cost, int area, int response_val, DialogState* state);
static void dialog_give_directions(int area, int a2, int a3, DialogState* state);
static void dialog_ask_money_for_mark_area(int cost, int area, int response_val, DialogState* state);
static void dialog_mark_area(int area, int a2, int a3, DialogState* state);
static void dialog_check_story(int response_val, DialogState* state);
static void dialog_copy_npc_story_msg(char* buffer, DialogState* state);
static void dialog_ask_about_buying_newspapers(int response_val, DialogState* state);
static void dialog_offer_today_newspaper(int response_val, DialogState* state);
static void dialog_offer_older_newspaper(int response_val, DialogState* state);
static void dialog_ask_money_for_newspaper(int newspaper, int response_val, DialogState* state);
static void dialog_buy_newspaper(int a1, int a2, int a3, DialogState* a4);

// 0x5A063C
static const char* dialog_gd_mes_file_names[GD_COUNT] = {
    "gd_pc2m",
    "gd_pc2f",
    "gd_cls_m2m",
    "gd_cls_m2f",
    "gd_cls_f2f",
    "gd_cls_f2m",
    "gd_cls_pc2m",
    "gd_cls_pc2f",
    "gd_rce_m2m",
    "gd_rce_m2f",
    "gd_rce_f2f",
    "gd_rce_f2m",
    "gd_npc_m2m",
    "gd_npc_m2f",
    "gd_npc_f2f",
    "gd_npc_f2m",
    "gd_dumb_pc2m",
    "gd_dumb_pc2f",
    "gd_cls_dumb_pc2m",
    "gd_cls_dumb_pc2f",
    "gd_cls_dumb_m2m",
    "gd_cls_dumb_m2f",
    "gd_cls_dumb_f2f",
    "gd_cls_dumb_f2m",
    "gd_rce_dumb_m2m",
    "gd_rce_dumb_m2f",
    "gd_rce_dumb_f2f",
    "gd_rce_dumb_f2m",
    "gd_sto_m2m",
    "gd_sto_m2f",
    "gd_sto_f2f",
    "gd_sto_f2m",
};

// 0x5A06BC
static const char* off_5A06BC[DIALOG_COND_COUNT] = {
    "ps",
    "ch",
    "pe",
    "al",
    "ma",
    "ta",
    "gv",
    "gf",
    "qu",
    "re",
    "$$",
    "in",
    "ha",
    "lf",
    "lc",
    "tr",
    "sk",
    "ru",
    "rq",
    "fo",
    "le",
    "qb",
    "me",
    "ni",
    "qa",
    "ra",
    "pa",
    "ss",
    "wa",
    "wt",
    "pv",
    "pf",
    "na",
    "ar",
    "rp",
    "ia",
    "sc",
};

// 0x5A0750
static const char* off_5A0750[DIALOG_ACTION_COUNT] = {
    "$$",
    "re",
    "qu",
    "fl",
    "co",
    "gv",
    "gf",
    "mm",
    "al",
    "in",
    "lf",
    "lc",
    "tr",
    "ru",
    "rq",
    "jo",
    "wa",
    "lv",
    "ss",
    "sc",
    "so",
    "uw",
    "pv",
    "pf",
    "xp",
    "nk",
    "rp",
    "np",
    "ce",
    "fp",
    "su",
    "or",
    "ii",
    "ri",
    "et",
};

// 0x5D1224
static char dialog_file_tmp_str[MAX_STRING];

// 0x5D19F4
static mes_file_handle_t* dialog_mes_files;

// 0x5D19F8
static size_t dialog_file_tmp_str_size;

// 0x5D19FC
static size_t dialog_file_tmp_str_pos;

// 0x5D1A00
static int dialog_files_length;

// 0x5D1A04
static int dialog_files_capacity;

// 0x5D1A08
static DialogFile* dialog_files;

// 0x5D1A0C
static bool dialog_numbers_enabled;

// 0x412D40
bool dialog_init(GameInitInfo* init_info)
{
    int index;

    (void)init_info;

    dialog_mes_files = (mes_file_handle_t*)CALLOC(GD_COUNT, sizeof(*dialog_mes_files));
    for (index = 0; index < GD_COUNT; index++) {
        dialog_mes_files[index] = MES_FILE_HANDLE_INVALID;
    }

    return true;
}

// 0x412D80
void dialog_exit(void)
{
    int index;

    for (index = 0; index < GD_COUNT; index++) {
        if (dialog_mes_files[index] != MES_FILE_HANDLE_INVALID) {
            mes_unload(dialog_mes_files[index]);
        }
    }
    FREE(dialog_mes_files);

    for (index = 0; index < dialog_files_capacity; index++) {
        if (dialog_files[index].path[0] != '\0') {
            sub_412F60(index);
        }
    }

    if (dialog_files_capacity > 0) {
        FREE(dialog_files);
        dialog_files = NULL;
        dialog_files_capacity = 0;
    }
}

// 0x412E10
bool dialog_load(const char* path, int* dlg_ptr)
{
    int index;
    DialogFile dialog;

    if (find_dialog(path, &index)) {
        dialog_files[index].refcount++;
        dialog_files[index].timestamp = sub_45A7C0();
    } else {
        strcpy(dialog.path, path);
        dialog.refcount = 1;
        dialog.timestamp = sub_45A7C0();
        dialog.entries_length = 0;
        dialog.entries_capacity = 0;
        dialog.entries = NULL;
        dialog_load_internal(&dialog);

        if (dialog.entries_length == 0) {
            return false;
        }

        dialog_files[index] = dialog;
        dialog_files_length++;
    }

    *dlg_ptr = index;

    return true;
}

// 0x412F40
void dialog_unload(int dlg)
{
    dialog_files[dlg].refcount--;
}

// 0x412F60
void sub_412F60(int dlg)
{
    int index;

    for (index = 0; index < dialog_files[dlg].entries_length; index++) {
        dialog_entry_free(&(dialog_files[dlg].entries[index]));
    }

    FREE(dialog_files[dlg].entries);
    dialog_files[dlg].path[0] = '\0';

    dialog_files_length--;
}

// 0x412FD0
bool sub_412FD0(DialogState* state)
{
    int64_t pc_loc;
    int64_t npc_loc;
    int64_t tmp;
    int64_t pc_loc_y;
    int64_t npc_loc_y;

    if (ai_can_speak(state->npc_obj, state->pc_obj, false) != AI_SPEAK_OK) {
        return false;
    }

    sub_4C1020(state->npc_obj, state->pc_obj);

    if (critter_is_dead(state->npc_obj) || ai_check_kos(state->npc_obj, state->pc_obj) == AI_KOS_NO) {
        if (player_is_local_pc_obj(state->pc_obj)) {
            pc_loc = obj_field_int64_get(state->pc_obj, OBJ_F_LOCATION);
            npc_loc = obj_field_int64_get(state->npc_obj, OBJ_F_LOCATION);
            location_xy(pc_loc, &tmp, &pc_loc_y);
            location_xy(npc_loc, &tmp, &npc_loc_y);
            if (npc_loc_y > pc_loc_y) {
                location_origin_set(npc_loc);
            } else {
                location_origin_set(pc_loc);
            }
        }

        state->field_17EC = state->num;
        state->field_17E8 = 0;
        sub_414E60(state, 0);
    } else {
        dialog_copy_npc_race_specific_msg(state->reply, state, 1000);
        state->field_17E8 = 4;
    }

    return true;
}

// 0x413130
void sub_413130(DialogState* state, int index)
{
    int v1;
    int v2;
    int v3;

    if (state->field_17E8 == 4
        || state->field_17E8 == 5
        || state->field_17E8 == 7
        || state->field_17E8 == 6
        || state->field_17E8 == 8
        || state->field_17E8 == 9) {
        return;
    }

    if (ai_can_speak(state->npc_obj, state->pc_obj, false) != AI_SPEAK_OK) {
        state->field_17E8 = 1;
        return;
    }

    if (state->field_17E8 == 3) {
        sub_417590(state->field_17EC, &v1, &v2);
        v3 = 0;
    } else if (sub_415BA0(state, state->actions[index], index)) {
        v1 = state->field_17F0[index];
        v2 = state->field_1804[index];
        v3 = state->field_1818[index];
    } else {
        return;
    }

    if (critter_is_dead(state->npc_obj) || ai_check_kos(state->npc_obj, state->pc_obj) == AI_KOS_NO) {
        sub_414810(v1, v2, v3, index, state);
    } else {
        dialog_copy_npc_race_specific_msg(state->reply, state, 1000);
        state->field_17E8 = 4;
    }
}

// 0x413280
void sub_413280(DialogState* state)
{
    sub_4C10A0(state->npc_obj, state->pc_obj);
}

// 0x4132A0
void sub_4132A0(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        if (critter_pc_leader_get(npc_obj) == pc_obj) {
            sub_419260(&state, "1 0, 2 0, 3 0, 4 0, 5 0, 6 0, 7 0, 8 0");
        } else {
            sub_419260(&state, NULL);
        }
        strcpy(buffer, state.reply);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413360
void dialog_state_init(int64_t npc_obj, int64_t pc_obj, DialogState* state)
{
    state->npc_obj = npc_obj;
    state->pc_obj = pc_obj;
    sub_443EB0(state->npc_obj, &(state->field_40));
    sub_443EB0(state->pc_obj, &(state->field_10));
    state->script_num = 0;
}

// 0x4133B0
void dialog_copy_npc_farewell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;
    int reaction_level;
    int reaction_type;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);

        reaction_level = reaction_get(npc_obj, pc_obj);
        reaction_type = reaction_translate(reaction_level);
        if (reaction_type < REACTION_SUSPICIOUS) {
            dialog_copy_npc_generic_msg(buffer, &state, 2000, 2099);
        } else {
            dialog_copy_npc_generic_msg(buffer, &state, 1900, 1999);
        }
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413490
void dialog_copy_npc_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 800, 899);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413520
void dialog_copy_npc_wont_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 900, 999);
    } else {
        buffer[0] = '\0';
    }
}

// 0x4135B0
void dialog_copy_npc_normally_wont_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 1000, 1099);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413640
void dialog_copy_npc_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 1100, 1199);
    } else {
        buffer[0] = '\0';
    }
}

// 0x4136D0
void dialog_copy_npc_wont_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 1200, 1299);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413760
void dialog_copy_npc_wont_buy_stolen_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 4600, 4699);
    } else {
        buffer[0] = '\0';
    }
}

// 0x4137F0
void dialog_copy_npc_normally_wont_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 1300, 1399);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413880
void dialog_copy_npc_buy_for_less_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 1400, 1499);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413910
void dialog_copy_npc_not_enough_money_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_class_specific_msg(buffer, &state, 2000);
    } else {
        buffer[0] = '\0';
    }
}

// 0x4139A0
void dialog_copy_npc_let_me_handle_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 4700, 4799);
    } else {
        buffer[0] = '\0';
    }
}

// 0x413A30
void sub_413A30(DialogState* a1, bool a2)
{
    if (a2 || ai_can_speak(a1->npc_obj, a1->pc_obj, false) == AI_SPEAK_OK) {
        a1->field_17EC = a1->num;
        a1->field_17E8 = 0;
        sub_416840(a1, 0);
    } else {
        a1->reply[0] = '\0';
        a1->speech_id = -1;
    }
}

// 0x413A90
void dialog_copy_npc_wont_follow_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr)
{
    DialogState state;
    int start;
    int end;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);

        switch (reason) {
        case AI_FOLLOW_TOO_GOOD:
            start = 1600;
            end = 1699;
            break;
        case AI_FOLLOW_TOO_BAD:
            start = 1700;
            end = 1799;
            break;
        case AI_FOLLOW_DISLIKE:
            start = 1800;
            end = 1899;
            break;
        case AI_FOLLOW_ALREADY_IN_GROUP:
            start = 2600;
            end = 2699;
            break;
        case AI_FOLLOW_LIMIT_REACHED:
            start = 2700;
            end = 2799;
            break;
        case AI_FOLLOW_NOT_ALLOWED:
            start = 2800;
            end = 2899;
            break;
        case AI_FOLLOW_LOW_LEVEL:
            start = 2900;
            end = 2999;
            break;
        default:
            buffer[0] = '\0';
            *speech_id_ptr = -1;
            return;
        }

        dialog_copy_npc_generic_msg(buffer, &state, start, end);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413BE0
void dialog_copy_npc_order_ok_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 2100, 2199);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413C90
void dialog_copy_npc_order_no_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 2200, 2299);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413D40
void dialog_copy_npc_insult_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_race_specific_msg(buffer, &state, 1000);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413DF0
void dialog_copy_npc_accidental_attack_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 2300, 2399);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413EA0
void dialog_copy_npc_fleeing_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 3000, 3099);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x413F50
void dialog_copy_npc_witness_pc_critical_msg(int64_t npc_obj, int64_t pc_obj, int type, char* buffer, int* speech_id_ptr)
{
    DialogState state;
    int start;
    int end;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);

        switch (type) {
        case AI_NPC_WITNESS_PC_CRITICAL_HIT:
            start = 2400;
            end = 2499;
            break;
        case AI_NPC_WITNESS_PC_CRITICAL_MISS:
            start = 2500;
            end = 2599;
            break;
        default:
            buffer[0] = '\0';
            *speech_id_ptr = -1;
            return;
        }

        dialog_copy_npc_generic_msg(buffer, &state, start, end);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x414020
void dialog_copy_npc_gamble_msg(int64_t npc_obj, int64_t pc_obj, int type, char* buffer)
{
    DialogState state;
    int start;
    int end;

    dialog_state_init(npc_obj, pc_obj, &state);

    switch (type) {
    case 0:
        start = 3100;
        end = 3199;
        break;
    case 1:
        start = 3200;
        end = 3299;
        break;
    case 2:
        start = 3300;
        end = 3399;
        break;
    case 3:
        start = 3400;
        end = 3499;
        break;
    case 4:
        start = 3500;
        end = 3599;
        break;
    case 5:
        start = 3600;
        end = 3699;
        break;
    case 6:
        start = 3700;
        end = 3799;
        break;
    case 7:
        start = 3800;
        end = 3899;
        break;
    case 8:
        start = 3900;
        end = 3999;
        break;
    case 9:
        start = 5600;
        end = 5699;
        break;
    default:
        return;
    }

    dialog_copy_npc_generic_msg(buffer, &state, start, end);
}

// 0x414130
void dialog_copy_npc_warning_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 4000, 4099);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x4141E0
void dialog_copy_npc_newspaper_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 4100, 4199);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x414290
void dialog_copy_npc_warning_follow_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr)
{
    DialogState state;
    int start;
    int end;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);

        switch (reason) {
        case AI_FOLLOW_TOO_GOOD:
            start = 5200;
            end = 5299;
            break;
        case AI_FOLLOW_TOO_BAD:
            start = 5300;
            end = 5399;
            break;
        case AI_FOLLOW_DISLIKE:
            start = 5400;
            end = 5499;
            break;
        default:
            buffer[0] = '\0';
            *speech_id_ptr = -1;
            return;
        }

        dialog_copy_npc_generic_msg(buffer, &state, start, end);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x414370
void dialog_copy_npc_upset_attacking_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr)
{
    DialogState state;
    int start;
    int end;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);

        switch (reason) {
        case AI_UPSET_ATTACKING_GOOD:
            start = 4900;
            end = 4999;
            break;
        case AI_UPSET_ATTACKING_ORIGIN:
            start = 5000;
            end = 5099;
            break;
        case AI_UPSET_ATTACKING_FACTION:
            start = 5100;
            end = 5199;
            break;
        case AI_UPSET_ATTACKING_PARTY:
            start = 4800;
            end = 4899;
            break;
        default:
            buffer[0] = '\0';
            *speech_id_ptr = -1;
            return;
        }

        dialog_copy_npc_generic_msg(buffer, &state, start, end);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x414490
void dialog_copy_npc_near_death_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, true) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 5500, 5599);
        *speech_id_ptr = state.speech_id;
    } else {
        buffer[0] = '\0';
        *speech_id_ptr = -1;
    }
}

// 0x414540
void dialog_copy_npc_wont_identify_already_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 5700, 5799);
    } else {
        buffer[0] = '\0';
    }
}

// 0x4145D0
void dialog_copy_npc_identify_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 5800, 5899);
    } else {
        buffer[0] = '\0';
    }
}

// 0x414660
void dialog_copy_npc_wont_repair_broken_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 5900, 5999);
    } else {
        buffer[0] = '\0';
    }
}

// 0x4146F0
void dialog_copy_npc_wont_repair_undamaged_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 6000, 6099);
    } else {
        buffer[0] = '\0';
    }
}

// 0x414780
void dialog_copy_npc_repair_msg(int64_t npc_obj, int64_t pc_obj, char* buffer)
{
    DialogState state;

    if (ai_can_speak(npc_obj, pc_obj, false) == AI_SPEAK_OK) {
        dialog_state_init(npc_obj, pc_obj, &state);
        dialog_copy_npc_generic_msg(buffer, &state, 6100, 6199);
    } else {
        buffer[0] = '\0';
    }
}

// 0x414810
void sub_414810(int a1, int a2, int a3, int a4, DialogState* a5)
{
    int v1[100];
    int cnt;

    switch (a1) {
    case 0:
        a5->field_17EC = a2;
        a5->field_17E8 = 0;
        sub_414E60(a5, 0);
        break;
    case 1:
        a5->field_17E8 = 1;
        break;
    case 2:
        a5->field_17EC = a2;
        a5->field_17E8 = 2;
        break;
    case 3:
        a5->field_17EC = a2;
        a5->field_17E8 = 3;
        break;
    case 4:
        sub_418B30(a2, a5);
        break;
    case 5:
        cnt = dialog_parse_params(v1, &(a5->options[a4][strlen(a5->options[a4]) + 1]));
        dialog_offer_training(v1, cnt, a2, a5);
        break;
    case 6:
        dialog_ask_money_for_training(a2, a5);
        break;
    case 7:
        dialog_perform_training(a2, a5);
        break;
    case 8: {
        char* pch;
        int cost;

        pch = strchr(&(a5->options[a4][strlen(a5->options[a4]) + 1]), '$');
        cost = atoi(pch + 1);

        pch = strchr(pch, ',');
        cnt = dialog_parse_params(v1, pch + 1);
        dialog_ask_money_for_rumor(cost, v1, cnt, a2, a5);
        break;
    }
    case 9:
        dialog_tell_rumor(a2, a5->field_17F0[1], a5->field_1804[1], a5);
        break;
    case 10:
        dialog_insult_reply(a2, a3, a5);
        break;
    case 11:
        dialog_offer_healing(DIALOG_HEALING_OFFER_OPTIONS, a2, a5);
        break;
    case 12:
        dialog_offer_healing(DIALOG_HEALING_OFFER_MAGICKAL_HEALING, a2, a5);
        break;
    case 13:
        dialog_offer_healing(DIALOG_HEALING_OFFER_MAGICKAL_POISON_HEALING, a2, a5);
        break;
    case 14:
        dialog_ask_money_for_skill(a2, a3, a5);
        break;
    case 15:
        dialog_ask_money_for_spell(a2, a3, a5);
        break;
    case 16:
        dialog_use_skill(a2, a5->field_17F0[1], a5->field_1804[1], a5);
        break;
    case 17:
        dialog_use_spell(a2, a5->field_17F0[1], a5->field_1804[1], a5);
        break;
    case 18:
        dialog_offer_directions(&(a5->options[a4][strlen(a5->options[a4]) + 1]), a2, a3, false, a5);
        break;
    case 19:
        dialog_ask_money_for_directions(a5->field_17EC, a2, a3, a5);
        break;
    case 20:
        dialog_give_directions(a2, a5->field_17F0[1], a5->field_1804[1], a5);
        break;
    case 21:
        dialog_offer_directions(&(a5->options[a4][strlen(a5->options[a4]) + 1]), a2, a3, true, a5);
        break;
    case 22:
        dialog_ask_money_for_mark_area(a5->field_17EC, a2, a3, a5);
        break;
    case 23:
        dialog_mark_area(a2, a5->field_17F0[1], a5->field_1804[1], a5);
        break;
    case 24:
        dialog_check_story(a2, a5);
        break;
    case 25:
        dialog_ask_about_buying_newspapers(a2, a5);
        break;
    case 26:
        dialog_offer_today_newspaper(a2, a5);
        break;
    case 27:
        dialog_offer_older_newspaper(a2, a5);
        break;
    case 28:
        dialog_ask_money_for_newspaper(a2, a3, a5);
        break;
    case 29:
        dialog_buy_newspaper(a2, a5->field_17F0[1], a5->field_1804[1], a5);
        break;
    case 30:
        a5->field_17E8 = 6;
        break;
    }
}

// 0x414E60
void sub_414E60(DialogState* a1, bool randomize)
{
    int index;
    int v1[5];
    int v2;

    for (index = 0; index < 5; index++) {
        a1->actions[index] = NULL;
        a1->options[index][0] = '\0';
    }

    if (randomize) {
        random_seed(a1->seed);
    } else {
        a1->seed = random_seed_generate();
    }

    if (sub_416840(a1, randomize)) {
        a1->num_options = sub_414F50(a1, v1);
        v2 = 0;
        for (index = 0; index < a1->num_options; index++) {
            if (!sub_416C10(v1[index], index - v2, a1)) {
                v2++;
            }
        }
        a1->num_options -= v2;
        if (a1->num_options == 0) {
            dialog_copy_pc_generic_msg(a1->options[0], a1, 400, 499);
            a1->field_17F0[0] = 1;
            a1->num_options = 1;
        }
    }
}

// 0x414F50
int sub_414F50(DialogState* a1, int* a2)
{
    DialogFileEntry key;
    int gender;
    int intelligence;
    DialogFile* dialog;
    DialogFileEntry* entry;
    int idx;
    int cnt;

    key.num = a1->field_17EC;
    gender = stat_level_get(a1->pc_obj, STAT_GENDER);
    intelligence = stat_level_get(a1->pc_obj, STAT_INTELLIGENCE);

    if (intelligence > LOW_INTELLIGENCE
        && critter_is_dumb(a1->pc_obj)) {
        intelligence = 1;
    }

    dialog = &(dialog_files[a1->dlg]);
    entry = bsearch(&key,
        dialog->entries,
        dialog->entries_length,
        sizeof(key),
        dialog_entry_compare);

    if (entry == NULL) {
        return 0;
    }

    cnt = 0;

    for (idx = (int)(entry - dialog->entries) + 1; idx < dialog->entries_length && cnt < 5; idx++) {
        entry = &(dialog->entries[idx]);
        if (entry->iq == 0) {
            return cnt;
        }

        if (entry->data.gender == -1 || entry->data.gender == gender) {
            if ((entry->iq < 0 && intelligence <= -entry->iq)
                || (entry->iq >= 0 && intelligence >= entry->iq)) {
                if (entry->conditions == NULL || sub_4150D0(a1, entry->conditions)) {
                    a2[cnt++] = entry->num;
                }
            }
        }
    }

    return cnt;
}

// 0x4150D0
bool sub_4150D0(DialogState* a1, char* a2)
{
    char* pch;
    int cond;
    int value;
    ObjectList followers;
    ObjectNode* node;
    int gold;
    int training;
    int level;
    int v39;
    int v40;
    bool inverse;
    int64_t substitute_inventory_obj;
    char code[3];

    if (a2 == NULL || a2[0] == '\0') {
        return true;
    }

    code[2] = '\0';

    pch = a2;
    while (*pch != '\0') {
        while (*pch != '\0' && !SDL_isalpha(*pch) && *pch != '$') {
            pch++;
        }

        if (*pch == '\0') {
            break;
        }

        code[0] = *pch++;
        if (*pch == '\0') {
            break;
        }

        code[1] = *pch++;

        value = atoi(pch);
        for (cond = 0; cond < DIALOG_COND_COUNT; cond++) {
            if (SDL_strcasecmp(off_5A06BC[cond], code) == 0) {
                break;
            }
        }

        switch (cond) {
        case DIALOG_COND_PS:
            if (value < 0) {
                if (basic_skill_level(a1->pc_obj, BASIC_SKILL_PERSUATION) > -value) {
                    return false;
                }
            } else {
                if (basic_skill_level(a1->pc_obj, BASIC_SKILL_PERSUATION) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_CH:
            if (value < 0) {
                if (stat_level_get(a1->pc_obj, STAT_CHARISMA) > -value) {
                    return false;
                }
            } else {
                if (stat_level_get(a1->pc_obj, STAT_CHARISMA) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_PE:
            if (value < 0) {
                if (stat_level_get(a1->pc_obj, STAT_PERCEPTION) > -value) {
                    return false;
                }
            } else {
                if (stat_level_get(a1->pc_obj, STAT_PERCEPTION) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_AL:
            if (value < 0) {
                if (stat_level_get(a1->pc_obj, STAT_ALIGNMENT) > -value) {
                    return false;
                }
            } else {
                if (stat_level_get(a1->pc_obj, STAT_ALIGNMENT) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_MA:
            if (value < 0) {
                if (stat_level_get(a1->pc_obj, STAT_MAGICK_TECH_APTITUDE) > -value) {
                    return false;
                }
            } else {
                if (stat_level_get(a1->pc_obj, STAT_MAGICK_TECH_APTITUDE) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_TA:
            if (value < 0) {
                if (-stat_level_get(a1->pc_obj, STAT_MAGICK_TECH_APTITUDE) > -value) {
                    return false;
                }
            } else {
                if (-stat_level_get(a1->pc_obj, STAT_MAGICK_TECH_APTITUDE) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_GV:
            if (script_global_var_get(value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_GF:
            if (script_global_flag_get(value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_QU:
            if (quest_state_get(a1->pc_obj, value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_RE:
            if (value < 0) {
                if (reaction_get(a1->npc_obj, a1->pc_obj) > -value) {
                    return false;
                }
            } else {
                if (reaction_get(a1->npc_obj, a1->pc_obj) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_GOLD:
            gold = item_gold_get(a1->pc_obj);
            object_list_followers(a1->pc_obj, &followers);
            node = followers.head;
            while (node != NULL) {
                gold += item_gold_get(node->obj);
                node = node->next;
            }
            object_list_destroy(&followers);
            if (value < 0) {
                if (gold > value) {
                    return false;
                }
            } else {
                if (gold < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_IN:
            if (value < 0) {
                value = -value;
                if (item_find_by_name(a1->npc_obj, value) == OBJ_HANDLE_NULL) {
                    substitute_inventory_obj = critter_substitute_inventory_get(a1->npc_obj);
                    if (substitute_inventory_obj == OBJ_HANDLE_NULL) {
                        return false;
                    }
                    if (item_find_by_name(substitute_inventory_obj, value) == OBJ_HANDLE_NULL) {
                        return false;
                    }
                }
            } else {
                if (!item_find_by_name(a1->pc_obj, value)) {
                    object_list_followers(a1->pc_obj, &followers);
                    node = followers.head;
                    while (node != NULL) {
                        if (item_find_by_name(node->obj, value) != OBJ_HANDLE_NULL) {
                            break;
                        }
                        node = node->next;
                    }
                    object_list_destroy(&followers);
                    if (node == NULL) {
                        return false;
                    }
                }
            }
            break;
        case DIALOG_COND_HA:
            if (value < 0) {
                if (basic_skill_level(a1->pc_obj, BASIC_SKILL_HAGGLE) > -value) {
                    return false;
                }
            } else {
                if (basic_skill_level(a1->pc_obj, BASIC_SKILL_HAGGLE) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_LF:
            if (script_local_flag_get(a1->npc_obj, SAP_DIALOG, value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_LC:
            if (script_local_counter_get(a1->npc_obj, SAP_DIALOG, value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_TR:
            training = sub_4167C0(pch);
            if (IS_TECH_SKILL(value)) {
                if (training < 0) {
                    if (tech_skill_training_get(a1->pc_obj, GET_TECH_SKILL(value)) > -training) {
                        return false;
                    }
                } else {
                    if (tech_skill_training_get(a1->pc_obj, GET_TECH_SKILL(value)) < training) {
                        return false;
                    }
                }
            } else {
                if (training < 0) {
                    if (basic_skill_training_get(a1->pc_obj, GET_BASIC_SKILL(value)) > -training) {
                        return false;
                    }
                } else {
                    if (basic_skill_training_get(a1->pc_obj, GET_BASIC_SKILL(value)) < training) {
                        return false;
                    }
                }
            }
            break;
        case DIALOG_COND_SK:
            level = sub_4167C0(pch);
            if (IS_TECH_SKILL(value)) {
                if (level < 0) {
                    if (tech_skill_level(a1->pc_obj, GET_TECH_SKILL(value)) > -level) {
                        return false;
                    }
                } else {
                    if (tech_skill_level(a1->pc_obj, GET_TECH_SKILL(value)) < level) {
                        return false;
                    }
                }
            } else {
                if (level < 0) {
                    if (basic_skill_level(a1->pc_obj, GET_BASIC_SKILL(value)) > -level) {
                        return false;
                    }
                } else {
                    if (basic_skill_level(a1->pc_obj, GET_BASIC_SKILL(value)) < level) {
                        return false;
                    }
                }
            }
            break;
        case DIALOG_COND_RU:
            if (value < 0) {
                if (rumor_known_get(a1->pc_obj, -value)) {
                    return false;
                }
            } else {
                if (!rumor_known_get(a1->pc_obj, value)) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_RQ:
            if (value < 0) {
                if (rumor_qstate_get(-value)) {
                    return false;
                }
            } else {
                if (!rumor_qstate_get(value)) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_FO:
            if (value == 0) {
                if (critter_leader_get(a1->npc_obj) != a1->pc_obj) {
                    return false;
                }
            } else if (value == 1) {
                if (critter_leader_get(a1->npc_obj) == a1->pc_obj) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_LE:
            if (value < 0) {
                if (-stat_level_get(a1->pc_obj, STAT_LEVEL) > -value) {
                    return false;
                }
            } else {
                if (-stat_level_get(a1->pc_obj, STAT_LEVEL) < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_QB:
            if (quest_state_get(a1->pc_obj, value) > sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_ME:
            if (value == 0) {
                if (reaction_met_before(a1->npc_obj, a1->pc_obj)) {
                    return false;
                }
            } else if (value == 1) {
                if (!reaction_met_before(a1->npc_obj, a1->pc_obj)) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_NI:
            if (value < 0) {
                value = -value;
                if (item_find_by_name(a1->npc_obj, value) != OBJ_HANDLE_NULL) {
                    return false;
                }

                substitute_inventory_obj = critter_substitute_inventory_get(a1->npc_obj);
                if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
                    if (item_find_by_name(substitute_inventory_obj, value) != OBJ_HANDLE_NULL) {
                        return false;
                    }
                }
            } else {
                if (item_find_by_name(a1->pc_obj, value)) {
                    return false;
                }

                object_list_followers(a1->pc_obj, &followers);
                node = followers.head;
                while (node != NULL) {
                    if (item_find_by_name(node->obj, value)) {
                        break;
                    }
                    node = node->next;
                }
                object_list_destroy(&followers);
                if (node != NULL) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_QA:
            if (quest_state_get(a1->pc_obj, value) < sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_RA:
            if (value > 0) {
                if (stat_level_get(a1->pc_obj, STAT_RACE) + 1 != value) {
                    return false;
                }
            } else {
                if (stat_level_get(a1->pc_obj, STAT_RACE) + 1 == -value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_PA:
            if (value < 0) {
                value = -value;
                inverse = true;
            } else {
                inverse = false;
            }
            object_list_followers(a1->pc_obj, &followers);
            node = followers.head;
            while (node != NULL) {
                if ((obj_field_int32_get(node->obj, OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) == 0
                    && obj_field_int32_get(node->obj, OBJ_F_NAME) == value) {
                    break;
                }
                node = node->next;
            }
            object_list_destroy(&followers);
            if (!inverse) {
                if (node == NULL) {
                    return false;
                }
            } else {
                if (node != NULL) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_SS:
            if (value < 0) {
                if (script_story_state_get() > -value) {
                    return false;
                }
            } else {
                if (script_story_state_get() < value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_WA:
            if ((obj_field_int32_get(a1->npc_obj, OBJ_F_NPC_FLAGS) & ONF_AI_WAIT_HERE) != 0) {
                if (value == 0) {
                    return false;
                }
            } else {
                if (value == 1) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_WT:
            if ((obj_field_int32_get(a1->npc_obj, OBJ_F_NPC_FLAGS) & ONF_JILTED) != 0) {
                if (value == 0) {
                    return false;
                }
            } else {
                if (value == 1) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_PV:
            if (script_pc_var_get(a1->pc_obj, value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_PF:
            if (script_pc_flag_get(a1->pc_obj, value) != sub_4167C0(pch)) {
                return false;
            }
            break;
        case DIALOG_COND_NA:
            if (value < 0) {
                if (stat_level_get(a1->pc_obj, STAT_ALIGNMENT) > value) {
                    return false;
                }
            } else {
                if (stat_level_get(a1->pc_obj, STAT_ALIGNMENT) < -value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_AR:
            if (value > 0) {
                if (!area_is_known(a1->pc_obj, value)) {
                    return false;
                }
            } else {
                if (area_is_known(a1->pc_obj, -value)) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_RP:
            if (value > 0) {
                if (!reputation_has(a1->pc_obj, value)) {
                    return false;
                }
            } else {
                if (reputation_has(a1->pc_obj, -value)) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_IA:
            if (value > 0) {
                if (area_of_object(a1->pc_obj) != value) {
                    return false;
                }
            } else {
                if (area_of_object(a1->pc_obj) == -value) {
                    return false;
                }
            }
            break;
        case DIALOG_COND_SC:
            v39 = sub_4167C0(pch);
            v40 = spell_college_level_get(a1->pc_obj, value);
            if (v39 > 0) {
                if (v40 < v39) {
                    return false;
                }
            } else {
                if (v40 > -v39) {
                    return false;
                }
            }
            break;
        default:
            // Unknown condition.
            return false;
        }
    }

    return true;
}

// 0x415BA0
bool sub_415BA0(DialogState* a1, char* a2, int a3)
{
    char* pch;
    int act;
    int value;
    ObjectList followers;
    ObjectNode* node;
    bool v57 = true;
    bool attack = false;
    char code[3];

    if (a2 == NULL || a2[0] == '\0') {
        return true;
    }

    code[2] = '\0';

    pch = a2;
    while (*pch != '\0') {
        while (*pch != '\0' && !SDL_isalpha(*pch) && *pch != '$') {
            pch++;
        }

        if (*pch == '\0') {
            break;
        }

        code[0] = *pch++;
        if (*pch == '\0') {
            break;
        }

        code[1] = *pch++;

        value = atoi(pch);
        for (act = 0; act < DIALOG_ACTION_COUNT; act++) {
            if (SDL_strcasecmp(off_5A0750[act], code) == 0) {
                break;
            }
        }

        switch (act) {
        case DIALOG_ACTION_GOLD:
            if (value > 0) {
                item_gold_transfer(OBJ_HANDLE_NULL, a1->pc_obj, value, OBJ_HANDLE_NULL);
            } else if (value < 0) {
                int total_gold;

                value = -value;

                total_gold = item_gold_get(a1->pc_obj);
                item_gold_transfer(a1->pc_obj, a1->npc_obj, value, OBJ_HANDLE_NULL);

                value -= total_gold;
                if (value > 0) {
                    object_list_followers(a1->pc_obj, &followers);
                    node = followers.head;
                    while (node != NULL) {
                        if (value <= 0) {
                            break;
                        }

                        total_gold = item_gold_get(node->obj);
                        item_gold_transfer(node->obj, a1->npc_obj, value, OBJ_HANDLE_NULL);

                        node = node->next;
                    }
                    object_list_destroy(&followers);
                }
            }
            break;
        case DIALOG_ACTION_RE: {
            int reaction;

            reaction = reaction_get(a1->npc_obj, a1->pc_obj);

            while (SDL_isspace(*pch)) {
                pch++;
            }

            if (*pch == '+' || *pch == '-') {
                reaction_adj(a1->npc_obj, a1->pc_obj, value);
            } else if (*pch == '>') {
                if (reaction < value) {
                    reaction_adj(a1->npc_obj, a1->pc_obj, value - reaction);
                }
            } else if (*pch == '<') {
                if (reaction > value) {
                    reaction_adj(a1->npc_obj, a1->pc_obj, value - reaction);
                }
            } else {
                reaction_adj(a1->npc_obj, a1->pc_obj, value - reaction);
            }

            break;
        }
        case DIALOG_ACTION_QU:
            quest_state_set(a1->pc_obj, value, sub_4167C0(pch), a1->npc_obj);
            break;
        case DIALOG_ACTION_FL:
            a1->num = value;
            sub_413A30(a1, false);
            a1->field_17E8 = 4;
            v57 = false;
            break;
        case DIALOG_ACTION_CO:
            attack = true;
            break;
        case DIALOG_ACTION_GV:
            script_global_var_set(value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_GF:
            script_global_flag_set(value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_MM:
            area_set_known(a1->pc_obj, value);
            break;
        case DIALOG_ACTION_AL: {
            int alignment;

            alignment = stat_base_get(a1->pc_obj, STAT_ALIGNMENT);

            while (SDL_isspace(*pch)) {
                pch++;
            }

            if (*pch == '+' || *pch == '-') {
                stat_base_set(a1->pc_obj, STAT_ALIGNMENT, alignment + value);
            } else if (*pch == '>') {
                if (alignment < value) {
                    stat_base_set(a1->pc_obj, STAT_ALIGNMENT, value);
                }
            } else if (*pch == '<') {
                if (alignment > value) {
                    stat_base_set(a1->pc_obj, STAT_ALIGNMENT, value);
                }
            } else {
                stat_base_set(a1->pc_obj, STAT_ALIGNMENT, value);
            }

            break;
        }
        case DIALOG_ACTION_IN: {
            int64_t item_obj;
            int64_t substitute_inventory_obj;
            int64_t parent_obj;

            if (value < 0) {
                value = -value;

                item_obj = item_find_by_name(a1->npc_obj, value);
                if (item_obj == OBJ_HANDLE_NULL) {
                    substitute_inventory_obj = critter_substitute_inventory_get(a1->npc_obj);
                    if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
                        item_obj = item_find_by_name(substitute_inventory_obj, value);
                    }
                }

                if (item_obj != OBJ_HANDLE_NULL) {
                    item_transfer(item_obj, a1->pc_obj);
                    item_parent(item_obj, &parent_obj);
                    if (parent_obj != a1->pc_obj) {
                        item_remove(item_obj);
                        sub_466E50(item_obj, obj_field_int64_get(a1->pc_obj, OBJ_F_LOCATION));
                    }
                }
            } else {
                item_obj = item_find_by_name(a1->pc_obj, value);
                if (item_obj == OBJ_HANDLE_NULL) {
                    object_list_followers(a1->pc_obj, &followers);
                    node = followers.head;
                    while (node != NULL) {
                        item_obj = item_find_by_name(node->obj, value);
                        if (item_obj != OBJ_HANDLE_NULL) {
                            break;
                        }
                        node = node->next;
                    }
                    object_list_destroy(&followers);
                }

                if (item_obj != OBJ_HANDLE_NULL) {
                    item_transfer(item_obj, a1->npc_obj);
                    item_parent(item_obj, &parent_obj);
                    if (parent_obj != a1->npc_obj) {
                        if (critter_pc_leader_get(a1->npc_obj) != OBJ_HANDLE_NULL) {
                            item_remove(item_obj);
                            sub_466E50(item_obj, obj_field_int64_get(a1->npc_obj, OBJ_F_LOCATION));
                        } else {
                            object_destroy(item_obj);
                        }
                    }
                }
            }
            break;
        }
        case DIALOG_ACTION_LF:
            script_local_flag_set(a1->npc_obj, SAP_DIALOG, value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_LC:
            script_local_counter_set(a1->npc_obj, SAP_DIALOG, value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_TR: {
            int training;

            training = sub_4167C0(pch);
            if (IS_TECH_SKILL(value)) {
                tech_skill_training_set(a1->pc_obj, GET_TECH_SKILL(value), training);
            } else {
                basic_skill_training_set(a1->pc_obj, GET_BASIC_SKILL(value), training);
            }
            break;
        }
        case DIALOG_ACTION_RU:
            rumor_known_set(a1->pc_obj, value);
            break;
        case DIALOG_ACTION_RQ:
            rumor_qstate_set(value);
            break;
        case DIALOG_ACTION_JO: {
            int v41;
            int rc;

            v41 = sub_4167C0(pch);
            rc = ai_check_follow(a1->npc_obj, a1->pc_obj, value);
            if (rc == AI_FOLLOW_OK) {
                critter_follow(a1->npc_obj, a1->pc_obj, value);
                a1->field_17EC = v41;
                a1->field_17E8 = 0;
                sub_414E60(a1, false);
                v57 = false;
            } else {
                dialog_copy_npc_wont_follow_msg(a1->npc_obj, a1->pc_obj, rc, a1->reply, &(a1->speech_id));
                a1->num_options = 1;
                dialog_copy_pc_generic_msg(a1->options[0], a1, 600, 699);
                a1->field_17F0[0] = a1->field_17F0[a3];
                a1->field_1804[0] = a1->field_1804[a3];
                a1->actions[0] = 0;
                v57 = false;
            }
            break;
        }
        case DIALOG_ACTION_WA:
            ai_npc_wait(a1->npc_obj);
            break;
        case DIALOG_ACTION_LV:
            critter_disband(a1->npc_obj, true);
            break;
        case DIALOG_ACTION_SS:
            script_story_state_set(value);
            break;
        case DIALOG_ACTION_SC:
            critter_stay_close(a1->npc_obj);
            break;
        case DIALOG_ACTION_SO:
            critter_spread_out(a1->npc_obj);
            break;
        case DIALOG_ACTION_UW: {
            int v43;
            int rc;

            v43 = sub_4167C0(pch);
            rc = ai_check_follow(a1->npc_obj, a1->pc_obj, value);
            if (rc == AI_FOLLOW_OK) {
                ai_npc_unwait(a1->npc_obj, value);
                a1->field_17EC = v43;
                a1->field_17E8 = 0;
                sub_414E60(a1, false);
                v57 = false;
            } else {
                dialog_copy_npc_wont_follow_msg(a1->npc_obj, a1->pc_obj, rc, a1->reply, &(a1->speech_id));
                a1->num_options = 1;
                dialog_copy_pc_generic_msg(a1->options[0], a1, 600, 699);
                a1->field_17F0[0] = a1->field_17F0[a3];
                a1->field_1804[0] = a1->field_1804[a3];
                a1->actions[0] = NULL;
                v57 = false;
            }
            break;
        }
        case DIALOG_ACTION_PV:
            script_pc_var_set(a1->pc_obj, value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_PF:
            script_pc_flag_set(a1->pc_obj, value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_XP:
            critter_give_xp(a1->pc_obj, quest_get_xp(value));
            break;
        case DIALOG_ACTION_NK:
            critter_kill(a1->npc_obj);
            break;
        case DIALOG_ACTION_RP:
            if (value > 0) {
                reputation_add(a1->pc_obj, value);
            } else {
                reputation_remove(a1->pc_obj, -value);
            }
            break;
        case DIALOG_ACTION_NP:
            newspaper_enqueue(value, sub_4167C0(pch));
            break;
        case DIALOG_ACTION_CE:
            dialog_copy_npc_order_ok_msg(a1->npc_obj, a1->pc_obj, a1->reply, &(a1->speech_id));
            a1->field_17E8 = 5;
            v57 = false;
            break;
        case DIALOG_ACTION_FP:
            stat_base_set(a1->pc_obj,
                STAT_FATE_POINTS,
                stat_base_get(a1->pc_obj, STAT_FATE_POINTS) + 1);
            break;
        case DIALOG_ACTION_SU:
            dialog_copy_npc_order_ok_msg(a1->npc_obj, a1->pc_obj, a1->reply, &(a1->speech_id));
            a1->field_17E8 = 7;
            v57 = false;
            break;
        case DIALOG_ACTION_OR:
            critter_origin_set(a1->npc_obj, value);
            break;
        case DIALOG_ACTION_II:
            dialog_copy_npc_order_ok_msg(a1->npc_obj, a1->pc_obj, a1->reply, &(a1->speech_id));
            a1->field_17E8 = 8;
            v57 = false;
            break;
        case DIALOG_ACTION_RI:
            dialog_copy_npc_order_ok_msg(a1->npc_obj, a1->pc_obj, a1->reply, &(a1->speech_id));
            a1->field_17E8 = 9;
            v57 = false;
            break;
        case DIALOG_ACTION_ET: {
            int v50;
            int training;
            int level;

            v50 = sub_4167C0(pch);
            if (IS_TECH_SKILL(value)) {
                level = tech_skill_level(a1->pc_obj, GET_TECH_SKILL(value));
                training = tech_skill_training_get(a1->pc_obj, GET_TECH_SKILL(value));
            } else {
                level = basic_skill_level(a1->pc_obj, GET_BASIC_SKILL(value));
                training = basic_skill_training_get(a1->pc_obj, GET_BASIC_SKILL(value));
            }

            if (level < training_min_skill_level_required(TRAINING_EXPERT)) {
                dialog_copy_npc_wont_follow_msg(a1->npc_obj, a1->pc_obj, 34, a1->reply, &(a1->speech_id));
                dialog_copy_npc_generic_msg(a1->reply, a1, 6200, 6299);
                a1->num_options = 1;
                dialog_copy_pc_generic_msg(a1->options[0], a1, 600, 699);
                a1->field_17F0[0] = a1->field_17F0[a3];
                a1->field_1804[0] = a1->field_1804[a3];
                a1->actions[0] = NULL;
                v57 = false;
            } else if (training < TRAINING_APPRENTICE) {
                dialog_copy_npc_wont_follow_msg(a1->npc_obj, a1->pc_obj, 34, a1->reply, &(a1->speech_id));
                dialog_copy_npc_generic_msg(a1->reply, a1, 6300, 6399);
                a1->num_options = 1;
                dialog_copy_pc_generic_msg(a1->options[0], a1, 600, 699);
                a1->field_17F0[0] = a1->field_17F0[a3];
                a1->field_1804[0] = a1->field_1804[a3];
                a1->actions[0] = NULL;
                v57 = false;
            } else {
                a1->field_17EC = v50;
                a1->field_17E8 = 0;
                sub_414E60(a1, false);
                v57 = false;
            }
            break;
        }
        default:
            // Unknown action.
            return v57;
        }
    }

    if (attack) {
        ai_attack(a1->pc_obj, a1->npc_obj, LOUDNESS_NORMAL, 0);
    }

    return v57;
}

// 0x4167C0
int sub_4167C0(const char* str)
{
    while (SDL_isspace(*str)) {
        str++;
    }

    while (SDL_isdigit(*str)) {
        str++;
    }

    return atoi(str);
}

// 0x416840
bool sub_416840(DialogState* a1, bool a2)
{
    DialogFileEntry entry;

    entry.num = a1->field_17EC;
    if (!dialog_search(a1->dlg, &entry)) {
        a1->speech_id = -1;
        a1->reply[0] = ' ';
        a1->reply[1] = '\0';
        return false;
    }

    a1->field_1840 = entry.response_val;
    a1->speech_id = sub_4189C0(entry.conditions, a1->script_num);

    if (!a2) {
        sub_415BA0(a1, entry.actions, 0);
    }

    if (SDL_strncasecmp(entry.str, "g:", 2) == 0) {
        sub_419260(a1, &(entry.str[2]));

        if (a1->field_17E8) {
            a1->num_options = 0;
            return false;
        }
        return true;
    }

    if (SDL_strcasecmp(entry.str, "i:") == 0) {
        dialog_copy_npc_race_specific_msg(a1->reply, a1, 1000);
        return true;
    }

    if (SDL_strncasecmp(entry.str, "m:", 2) == 0) {
        char* pch;
        int v2;
        int v3;
        int v4;
        int v5;
        int v6;
        int v7;

        pch = strchr(entry.str, '$') + 1;
        v2 = atoi(pch);

        pch = strchr(pch, ',') + 1;
        v3 = atoi(pch);

        sub_417590(v3, &v4, &v5);
        sub_417590(entry.response_val, &v6, &v7);
        dialog_ask_money(v2, v4, v5, v6, v7, a1);

        return false;
    }

    if (SDL_strncasecmp(entry.str, "r:", 2) == 0) {
        char* pch;
        int cost;
        int cnt;
        int rumors[100];

        pch = strchr(entry.str, '$') + 1;
        cost = atoi(pch);

        pch = strchr(pch, ',');
        cnt = dialog_parse_params(rumors, pch + 1);

        dialog_ask_money_for_rumor(cost, rumors, cnt, entry.response_val, a1);
        return false;
    }

    if (obj_type_is_critter(obj_field_int32_get(a1->pc_obj, OBJ_F_TYPE))
        && stat_level_get(a1->pc_obj, STAT_GENDER) != GENDER_MALE) {
        sub_416B00(a1->reply, entry.data.female_str, a1);
    } else {
        sub_416B00(a1->reply, entry.str, a1);
    }

    return true;
}

// 0x416AB0
bool dialog_search(int dlg, DialogFileEntry* entry)
{
    DialogFileEntry* found;

    found = bsearch(entry,
        dialog_files[dlg].entries,
        dialog_files[dlg].entries_length,
        sizeof(*entry),
        dialog_entry_compare);
    if (found == NULL) {
        return false;
    }

    *entry = *found;

    return true;
}

// 0x416B00
void sub_416B00(char* dst, char* src, DialogState* a3)
{
    char* remainder;
    char* start;
    char* end;

    remainder = src;
    *dst = '\0';

    start = strchr(remainder, '@');
    while (start != NULL) {
        *start = '\0';
        strcat(dst, remainder);
        end = strchr(start + 1, '@');
        *end = '\0';
        if (SDL_strcasecmp(start + 1, "pcname") == 0) {
            object_examine(a3->pc_obj, OBJ_HANDLE_NULL, dst + strlen(dst));
        } else if (SDL_strcasecmp(start + 1, "npcname") == 0) {
            object_examine(a3->pc_obj, a3->npc_obj, dst + strlen(dst));
        }

        *start = '@';
        *end = '@';
        remainder = end + 1;
        start = strchr(remainder, '@');
    }

    strcat(dst, remainder);
}

// 0x416C10
bool sub_416C10(int a1, int a2, DialogState* a3)
{
    DialogFileEntry entry;
    char* pch;
    int values[100];
    int cnt;
    int v2;
    int v3;
    int v4;

    entry.num = a1;
    dialog_search(a3->dlg, &entry);

    if (SDL_strcasecmp(entry.str, "a:") == 0) {
        dialog_copy_pc_class_specific_msg(a3->options[a2], a3, 1000);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strcasecmp(entry.str, "b:") == 0) {
        if (critter_leader_get(a3->npc_obj) == a3->pc_obj) {
            dialog_copy_pc_generic_msg(a3->options[a2], a3, 1600, 1699);
        } else {
            dialog_copy_pc_generic_msg(a3->options[a2], a3, 300, 399);
        }
        a3->field_17F0[a2] = 3;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strncasecmp(entry.str, "c:", 2) == 0) {
        dialog_copy_pc_story_msg(a3->options[a2], a3);
        a3->field_17F0[a2] = 24;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strncasecmp(entry.str, "d:", 2) == 0) {
        pch = strchr(entry.str, ',');
        cnt = dialog_parse_params(values, pch + 1);
        if (filter_unknown_areas(a3->pc_obj, values, cnt) == 0) {
            return false;
        }
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 1300, 1399);

        pch = a3->options[a2];
        strcpy(&(pch[strlen(pch) + 1]), entry.str + 2);
        a3->field_17F0[a2] = 18;
        a3->field_1804[a2] = entry.response_val;
        a3->field_1818[a2] = 0;
    } else if (SDL_strcasecmp(entry.str, "e:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 400, 499);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strcasecmp(entry.str, "f:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 800, 899);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strcasecmp(entry.str, "h:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 700, 799);
        a3->field_17F0[a2] = 11;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strcasecmp(entry.str, "i:") == 0) {
        sub_417590(entry.response_val, &v2, &v3);
        dialog_build_pc_insult_option(a2, v2, v3, a3);
    } else if (SDL_strcasecmp(entry.str, "k:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 1500, 1599);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strcasecmp(entry.str, "l:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 2300, 2399);
        a3->field_17F0[a2] = 30;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strcasecmp(entry.str, "n:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 100, 199);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strcasecmp(entry.str, "p:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 1900, 1999);
        a3->field_17F0[a2] = 25;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strncasecmp(entry.str, "q:", 2) == 0) {
        v4 = quest_dialog_line(a3->pc_obj, a3->npc_obj, atoi(entry.str + 2));
        if (v4 != -1) {
            return sub_416C10(v4, a2, a3);
        }
        return false;
    } else if (SDL_strncasecmp(entry.str, "r:", 2) == 0) {
        dialog_copy_pc_class_specific_msg(a3->options[a2], a3, 2000);

        pch = a3->options[a2];
        strcpy(&(pch[strlen(pch) + 1]), entry.str + 2);
        a3->field_17F0[a2] = 8;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strcasecmp(entry.str, "s:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 200, 299);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strncasecmp(entry.str, "t:", 2) == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 500, 599);

        pch = a3->options[a2];
        strcpy(&(pch[strlen(pch) + 1]), entry.str + 2);
        a3->field_17F0[a2] = 5;
        a3->field_1804[a2] = entry.response_val;
    } else if (SDL_strncasecmp(entry.str, "u:", 2) == 0) {
        v4 = atoi(entry.str + 2);
        if (ai_check_use_skill(a3->npc_obj, a3->pc_obj, skill_supplementary_item(a3->npc_obj, v4), v4) != AI_USE_SKILL_OK) {
            return false;
        }

        dialog_build_use_skill_option(a2, v4, entry.response_val, a3);
    } else if (SDL_strcasecmp(entry.str, "w:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 1800, 1899);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strncasecmp(entry.str, "x:", 2) == 0) {
        pch = strchr(entry.str, ',');
        cnt = dialog_parse_params(values, pch + 1);
        if (filter_unknown_areas(a3->pc_obj, values, cnt) == 0) {
            return false;
        }

        dialog_copy_pc_generic_msg(a3->options[a2], a3, 1400, 1499);
        pch = a3->options[a2];
        strcpy(&(pch[strlen(pch) + 1]), entry.str + 2);
        a3->field_17F0[a2] = 21;
        a3->field_1804[a2] = entry.response_val;
        a3->field_1818[a2] = 0;
    } else if (SDL_strcasecmp(entry.str, "y:") == 0) {
        dialog_copy_pc_generic_msg(a3->options[a2], a3, 1, 99);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    } else if (SDL_strncasecmp(entry.str, "z:", 2) == 0) {
        dialog_build_use_spell_option(a2, atoi(entry.str + 2), entry.response_val, a3);
    } else {
        sub_416B00(a3->options[a2], entry.str, a3);
        sub_417590(entry.response_val, &(a3->field_17F0[a2]), &(a3->field_1804[a2]));
    }

    a3->actions[a2] = entry.actions;

    if (dialog_numbers_enabled) {
        char str[20];
        size_t len;
        size_t pos;

        sprintf(str, "[%d]", entry.num);
        len = strlen(str);
        for (pos = 999; pos >= len; pos--) {
            a3->options[a2][pos] = a3->options[a2][pos - len];
        }
        if (len > 0) {
            strncpy(a3->options[a2], str, len);
        }
    }

    return true;
}

// 0x417590
void sub_417590(int a1, int* a2, int* a3)
{
    if (a1 > 0) {
        *a2 = 0;
        *a3 = a1;
    } else if (a1 < 0) {
        *a2 = 2;
        *a3 = -a1;
    } else {
        *a2 = 1;
    }
}

// 0x4175D0
bool find_dialog(const char* path, int* index_ptr)
{
    int candidate = -1;
    int index;

    for (index = 0; index < dialog_files_capacity; index++) {
        if (SDL_strcasecmp(path, dialog_files[index].path) == 0) {
            *index_ptr = index;
            return true;
        }

        if (dialog_files[index].refcount == 0) {
            if (candidate == -1) {
                candidate = index;
            } else if (dialog_files[candidate].path[0] != '\0') {
                if (dialog_files[index].path[0] == '\0'
                    || datetime_compare(&(dialog_files[candidate].timestamp), &(dialog_files[index].timestamp)) > 0) {
                    candidate = index;
                }
            }
        }
    }

    if (candidate != -1) {
        *index_ptr = candidate;
        if (dialog_files[candidate].path[0] != '\0') {
            sub_412F60(candidate);
        }
        return false;
    }

    *index_ptr = index;

    dialog_files_capacity += 10;
    dialog_files = (DialogFile*)REALLOC(dialog_files, sizeof(*dialog_files) * dialog_files_capacity);
    while (index < dialog_files_capacity) {
        dialog_files[index].refcount = 0;
        dialog_files[index].path[0] = '\0';
        index++;
    }

    return false;
}

// 0x417720
void dialog_load_internal(DialogFile* dialog)
{
    TigFile* stream;
    DialogFileEntry tmp_entry;
    char female_str[1000];
    char conditions[1000];
    char str[1000];
    char actions[1000];
    int line;

    stream = dialog_file_fopen(dialog->path, "rt");
    if (stream == NULL) {
        return;
    }

    tmp_entry.data.female_str = female_str;
    tmp_entry.conditions = conditions;
    tmp_entry.str = str;
    tmp_entry.actions = actions;

    line = 1;
    while (dialog_parse_entry(stream, &tmp_entry, &line)) {
        if (dialog->entries_length == dialog->entries_capacity) {
            dialog->entries_capacity += 10;
            dialog->entries = (DialogFileEntry*)REALLOC(dialog->entries, sizeof(*dialog->entries) * dialog->entries_capacity);
        }

        dialog_entry_copy(&(dialog->entries[dialog->entries_length]), &tmp_entry);
        dialog->entries_length++;

        tmp_entry.data.female_str = female_str;
    }

    dialog_file_fclose(stream);

    if (dialog->entries_length != 0) {
        qsort(dialog->entries,
            dialog->entries_length,
            sizeof(*dialog->entries),
            dialog_entry_compare);
    }
}

// 0x417860
bool dialog_parse_entry(TigFile* stream, DialogFileEntry* entry, int* line_ptr)
{
    char str[1000];
    char gender_str[1000];

    if (!dialog_parse_field(stream, str, line_ptr)) {
        return false;
    }
    entry->num = atoi(str);

    if (!dialog_parse_field(stream, str, line_ptr)) {
        tig_debug_printf("Missing text on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }
    strcpy(entry->str, str);

    if (!dialog_parse_field(stream, gender_str, line_ptr)) {
        tig_debug_printf("Missing gender field on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }

    if (!dialog_parse_field(stream, str, line_ptr)) {
        tig_debug_printf("Missing minimum IQ value on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }

    entry->iq = atoi(str);
    if (entry->iq == 0 && str[0] != '\0') {
        tig_debug_printf("Invalid minimum IQ value on line: %d (dialog line %d). Must be blank (for an NPC) or non-zero (for a PC).\n", *line_ptr, entry->num);
        return false;
    }

    if (!dialog_parse_field(stream, str, line_ptr)) {
        tig_debug_printf("Missing test field on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }
    strcpy(entry->conditions, str);

    if (!dialog_parse_field(stream, str, line_ptr)) {
        tig_debug_printf("Missing response value on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }

    if (str[0] == '#') {
        tig_debug_printf("Saw a # in a response value on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }
    entry->response_val = atoi(str);

    if (!dialog_parse_field(stream, str, line_ptr)) {
        tig_debug_printf("Missing effect field on line: %d (dialog line %d)\n", *line_ptr, entry->num);
        return false;
    }
    strcpy(entry->actions, str);

    if (entry->iq != 0) {
        if (gender_str[0] != '\0') {
            entry->data.gender = atoi(gender_str);
        } else {
            entry->data.gender = -1;
        }
    } else {
        size_t pos;
        size_t end;

        strcpy(entry->data.female_str, gender_str);

        pos = 0;
        end = strlen(gender_str);
        while (pos < end) {
            if (!SDL_isspace(gender_str[pos])) {
                break;
            }
            pos++;
        }

        if (pos == end) {
            pos = 0;
            end = strlen(entry->str);
            while (pos < end) {
                if (!SDL_isspace(entry->str[pos])) {
                    break;
                }
                pos++;
            }

            if (pos != end) {
                tig_debug_printf("Missing NPC response line for females: %d (dialog line %d)\n", *line_ptr, entry->num);
            }
        }
    }

    return true;
}

// 0x417C20
bool dialog_parse_field(TigFile* stream, char* str, int* line_ptr)
{
    int prev = 0;
    int ch;
    int len;
    bool overflow;

    ch = dialog_file_fgetc(stream);
    while (ch != '{') {
        if (ch == EOF) {
            return false;
        }

        if (ch == '\n') {
            (*line_ptr)++;
        } else if (ch == '}') {
            tig_debug_printf("Warning! Possible missing left brace { on or before line %d\n", *line_ptr);
        } else if (ch == '/' && prev == '/') {
            do {
                ch = dialog_file_fgetc(stream);
                if (ch == EOF) {
                    return false;
                }
            } while (ch != '\n');
            prev = 0;
            (*line_ptr)++;
        } else {
            prev = ch;
        }

        ch = dialog_file_fgetc(stream);
    }

    len = 0;
    overflow = false;

    ch = dialog_file_fgetc(stream);
    while (ch != '}') {
        if (ch == EOF) {
            tig_debug_printf("Expected right bracket }, reached end of file.\n");
            return false;
        }

        if (ch == '\n') {
            (*line_ptr)++;
        } else if (ch == '{') {
            tig_debug_printf("Warning! Possible missing right brace } on or before line %d\n", *line_ptr);
        }

        if (len < 999) {
            str[len++] = (char)(unsigned char)ch;
        } else {
            overflow = true;
        }

        ch = dialog_file_fgetc(stream);
    }

    str[len] = '\0';

    if (overflow) {
        tig_debug_printf("String too long on line: %d\n", *line_ptr);
    }

    return true;
}

// 0x417D60
TigFile* dialog_file_fopen(const char* fname, const char* mode)
{
    dialog_file_tmp_str_pos = 0;
    dialog_file_tmp_str_size = 0;
    return tig_file_fopen(fname, mode);
}

// 0x417D80
int dialog_file_fclose(TigFile* stream)
{
    return tig_file_fclose(stream);
}

// 0x417D90
int dialog_file_fgetc(TigFile* stream)
{
    if (dialog_file_tmp_str_pos != dialog_file_tmp_str_size) {
        return dialog_file_tmp_str[dialog_file_tmp_str_pos++];
    }

    dialog_file_tmp_str_pos = 0;
    dialog_file_tmp_str_size = 0;

    if (tig_file_fgets(dialog_file_tmp_str, sizeof(dialog_file_tmp_str), stream) == NULL) {
        return EOF;
    }

    dialog_file_tmp_str_size = strlen(dialog_file_tmp_str);
    if (dialog_file_tmp_str_size == 0) {
        return EOF;
    }

    return dialog_file_tmp_str[dialog_file_tmp_str_pos++];
}

// 0x417E00
int dialog_entry_compare(const void* va, const void* vb)
{
    const DialogFileEntry* a = (const DialogFileEntry*)va;
    const DialogFileEntry* b = (const DialogFileEntry*)vb;

    if (a->num < b->num) {
        return -1;
    } else if (a->num > b->num) {
        return 1;
    } else {
        return 0;
    }
}

// 0x417E20
void dialog_entry_copy(DialogFileEntry* dst, const DialogFileEntry* src)
{
    size_t pos;
    size_t end;

    *dst = *src;
    dst->str = STRDUP(src->str);

    pos = 0;
    end = strlen(src->conditions);
    while (pos < end) {
        if (!SDL_isspace(src->conditions[pos])) {
            break;
        }
        pos++;
    }

    if (pos != end) {
        dst->conditions = STRDUP(src->conditions);
    } else {
        dst->conditions = NULL;
    }

    pos = 0;
    end = strlen(src->actions);
    while (pos < end) {
        if (!SDL_isspace(src->actions[pos])) {
            break;
        }
        pos++;
    }

    if (pos != end) {
        dst->actions = STRDUP(src->actions);
    } else {
        dst->actions = NULL;
    }

    if (src->iq == 0) {
        dst->data.female_str = STRDUP(src->data.female_str);
    }
}

// 0x417F40
void dialog_entry_free(DialogFileEntry* entry)
{
    if (entry->iq == 0) {
        FREE(entry->data.female_str);
    }

    FREE(entry->str);

    if (entry->conditions != NULL) {
        FREE(entry->conditions);
    }

    if (entry->actions != NULL) {
        FREE(entry->actions);
    }
}

// 0x417F90
int dialog_parse_params(int* values, char* str)
{
    int cnt = 0;
    char* comma;
    char* dash;
    int start;
    int end;

    while (cnt < 100) {
        comma = strchr(str, ',');
        if (comma != NULL) {
            *comma = '\0';
        }

        dash = strchr(str, '-');
        if (dash != NULL) {
            *dash = '\0';
            start = atoi(str);
            end = atoi(dash + 1);
            while (start <= end && cnt < 100) {
                values[cnt++] = start;
                start++;
            }
        } else {
            values[cnt++] = atoi(str);
        }

        if (comma == NULL) {
            break;
        }

        *comma = ',';
        str = comma + 1;
    }

    return cnt;
}

// 0x418030
void dialog_check(void)
{
    TigFileList file_list;
    unsigned int index;
    char path[80];
    int dlg;
    int entry_index;
    DialogFileEntry* entry;
    int overflow;
    DialogFileEntry tmp_entry;

    tig_file_list_create(&file_list, "dlg\\*.dlg");
    for (index = 0; index < file_list.count; index++) {
        sprintf(path, "dlg\\%s", file_list.entries[index].path);
        tig_debug_printf("Checking dialog file %s\n", path);
        if (dialog_load(path, &dlg)) {
            for (entry_index = 0; entry_index < dialog_files[dlg].entries_length; entry_index++) {
                entry = &(dialog_files[dlg].entries[entry_index]);
                if (entry->iq) {
                    overflow = tc_check_size(entry->str);
                    if (overflow > 0) {
                        tig_debug_printf("PC response %d too long by %d pixels\n", entry->num, overflow);
                    }

                    if (entry->response_val > 0) {
                        tmp_entry.num = entry->response_val;
                        if (dialog_search(dlg, &tmp_entry)) {
                            if (tmp_entry.iq) {
                                tig_debug_printf("PC response %d jumps to non-PC line %d\n", entry->num, entry->response_val);
                            }
                        } else {
                            tig_debug_printf("PC response %d jumps to non-existent dialog line %d\n", entry->num, entry->response_val);
                        }
                    }
                }
            }
            dialog_unload(dlg);
        }
    }
    tig_file_list_destroy(&file_list);

    dialog_check_generated(GD_PC2M);
    dialog_check_generated(GD_PC2F);
    dialog_check_generated(GD_CLS_PC2M);
    dialog_check_generated(GD_CLS_PC2F);
    dialog_check_generated(GD_DUMB_PC2M);
    dialog_check_generated(GD_DUMB_PC2F);
    dialog_check_generated(GD_CLS_DUMB_PC2M);
    dialog_check_generated(GD_CLS_DUMB_PC2F);
}

// 0x4181D0
void dialog_check_generated(int gd)
{
    MesFileEntry mes_file_entry;
    int overflow;

    tig_debug_printf("Checking generated dialog file mes\\%s.mes\n", dialog_gd_mes_file_names[gd]);
    dialog_load_generated(gd);

    if (mes_find_first(dialog_mes_files[gd], &mes_file_entry)) {
        do {
            overflow = tc_check_size(mes_file_entry.str);
            if (overflow > 0) {
                tig_debug_printf("Generated response %d too long by %d pixels\n", mes_file_entry.num, overflow);
            }
        } while (mes_find_next(dialog_mes_files[gd], &mes_file_entry));
    }
}

// 0x418250
void dialog_load_generated(int gd)
{
    char path[TIG_MAX_PATH];

    if (dialog_mes_files[gd] == MES_FILE_HANDLE_INVALID) {
        sprintf(path, "mes\\%s.mes", dialog_gd_mes_file_names[gd]);
        if (!mes_load(path, &(dialog_mes_files[gd]))) {
            tig_debug_printf("Cannot open generated dialog file %s\n", path);
            exit(EXIT_SUCCESS); // FIXME: Should be EXIT_FAILURE.
        }
    }
}

// 0x4182C0
void dialog_enable_numbers(void)
{
    dialog_numbers_enabled = true;
}

// 0x4182D0
void dialog_copy_pc_generic_msg(char* buffer, DialogState* state, int start, int end)
{
    int gd;
    int cnt;
    MesFileEntry mes_file_entry;

    if (critter_is_dumb(state->pc_obj)) {
        gd = stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_DUMB_PC2M
            : GD_DUMB_PC2F;
    } else {
        gd = stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_PC2M
            : GD_PC2F;
    }

    dialog_load_generated(gd);

    cnt = mes_entries_count_in_range(dialog_mes_files[gd], start, end);
    mes_file_entry.num = start + random_between(0, cnt - 1);
    mes_get_msg(dialog_mes_files[gd], &mes_file_entry);

    sub_416B00(buffer, mes_file_entry.str, state);
}

// 0x418390
void dialog_copy_pc_class_specific_msg(char* buffer, DialogState* state, int num)
{
    int gd;
    int cnt;
    MesFileEntry mes_file_entry;

    if (critter_is_dumb(state->pc_obj)) {
        gd = stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_CLS_DUMB_PC2M
            : GD_CLS_DUMB_PC2F;
    } else {
        gd = stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_CLS_PC2M
            : GD_CLS_PC2F;
    }

    num += 50 * critter_social_class_get(state->npc_obj);

    dialog_load_generated(gd);

    cnt = mes_entries_count_in_range(dialog_mes_files[gd], num, num + 49);
    mes_file_entry.num = num + random_between(0, cnt - 1);
    mes_get_msg(dialog_mes_files[gd], &mes_file_entry);

    sub_416B00(buffer, mes_file_entry.str, state);
}

// 0x418460
void dialog_copy_pc_story_msg(char* buffer, DialogState* state)
{
    int story_state = script_story_state_get();
    dialog_copy_pc_generic_msg(buffer, state, story_state + 1700, story_state + 1700);
}

// 0x418480
void dialog_copy_npc_class_specific_msg(char* buffer, DialogState* state, int num)
{
    int gd;
    int cnt;
    MesFileEntry mes_file_entry;

    if (dialog_copy_npc_override_msg(buffer, state, num / 1000 + 9999)) {
        return;
    }

    if (critter_is_dumb(state->npc_obj)) {
        if (stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE) {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_CLS_DUMB_M2M
                : GD_CLS_DUMB_M2F;
        } else {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_CLS_DUMB_F2M
                : GD_CLS_DUMB_F2F;
        }
    } else {
        if (stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE) {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_CLS_M2M
                : GD_CLS_M2F;
        } else {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_CLS_F2M
                : GD_CLS_F2F;
        }

        num += 50 * critter_social_class_get(state->npc_obj);
    }

    dialog_load_generated(gd);

    cnt = mes_entries_count_in_range(dialog_mes_files[gd], num, num + 49);
    mes_file_entry.num = num + random_between(0, cnt - 1);
    mes_get_msg(dialog_mes_files[gd], &mes_file_entry);

    sub_416B00(buffer, mes_file_entry.str, state);
    state->speech_id = -1;
}

// 0x4185F0
void dialog_copy_npc_race_specific_msg(char* buffer, DialogState* state, int num)
{
    int gd;
    int cnt;
    MesFileEntry mes_file_entry;
    int race;

    if (dialog_copy_npc_override_msg(buffer, state, num / 1000 + 10999)) {
        return;
    }

    if (critter_is_dumb(state->npc_obj)) {
        if (stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE) {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_RCE_DUMB_M2M
                : GD_RCE_DUMB_M2F;
        } else {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_RCE_DUMB_F2M
                : GD_RCE_DUMB_F2F;
        }
    } else {
        if (stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE) {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_RCE_M2M
                : GD_RCE_M2F;
        } else {
            gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
                ? GD_RCE_F2M
                : GD_RCE_F2F;
        }

        race = stat_level_get(state->pc_obj, STAT_RACE);
        if (race != stat_level_get(state->npc_obj, STAT_RACE)) {
            num += 50 * (race + 1);
        }
    }

    dialog_load_generated(gd);

    cnt = mes_entries_count_in_range(dialog_mes_files[gd], num, num + 49);
    mes_file_entry.num = num + random_between(0, cnt - 1);
    mes_get_msg(dialog_mes_files[gd], &mes_file_entry);

    sub_416B00(buffer, mes_file_entry.str, state);
    state->speech_id = -1;
}

// 0x418780
void dialog_copy_npc_generic_msg(char* buffer, DialogState* state, int start, int end)
{
    int gd;
    int cnt;
    MesFileEntry mes_file_entry;

    if (dialog_copy_npc_override_msg(buffer, state, start / 100 + 11999)) {
        return;
    }

    if (stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE) {
        gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_NPC_M2M
            : GD_NPC_M2F;
    } else {
        gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_NPC_F2M
            : GD_NPC_F2F;
    }

    dialog_load_generated(gd);

    cnt = mes_entries_count_in_range(dialog_mes_files[gd], start, end);
    mes_file_entry.num = start + random_between(0, cnt - 1);
    mes_get_msg(dialog_mes_files[gd], &mes_file_entry);

    sub_416B00(buffer, mes_file_entry.str, state);
    state->speech_id = -1;
}

// 0x418870
bool dialog_copy_npc_override_msg(char* buffer, DialogState* state, int num)
{
    Script scr;
    char path[TIG_MAX_PATH];
    int dlg;
    DialogFileEntry entry;
    bool exists;

    obj_arrayfield_script_get(state->npc_obj, OBJ_F_SCRIPTS_IDX, SAP_DIALOG_OVERRIDE, &scr);

    if (scr.num == 0) {
        return false;
    }

    if (!script_name_build_dlg_name(scr.num, path)) {
        return false;
    }

    if (!dialog_load(path, &dlg)) {
        return false;
    }

    entry.num = num;
    exists = dialog_search(dlg, &entry);
    dialog_unload(dlg);

    if (!exists) {
        return false;
    }

    if (state->pc_obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(state->pc_obj, OBJ_F_TYPE))
        && stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE) {
        strcpy(buffer, entry.str);
    } else {
        strcpy(buffer, entry.data.female_str);
    }

    state->speech_id = sub_4189C0(entry.conditions, scr.num);

    return true;
}

// 0x4189C0
int sub_4189C0(const char* a1, int a2)
{
    if (a1 != NULL && a2 != 0) {
        return ((a2 & 0x7FFF) << 16) | (atoi(a1) & 0xFFFF);
    } else {
        return -1;
    }
}

// 0x418A00
void sub_418A00(int a1, int* a2, int* a3)
{
    if (a1 != -1) {
        *a2 = (a1 >> 16) & 0x7FFF;
        *a3 = a1 & 0xFFFF;
    } else {
        *a2 = 0;
        *a3 = 0;
    }
}

// 0x418A40
void dialog_ask_money(int amt, int param1, int param2, int a4, int a5, DialogState* state)
{
    char buffer[1000];

    amt = sub_4C1150(state->npc_obj, state->pc_obj, amt);
    if (amt == 1) {
        amt = 2;
    }

    // NPC: "It will cost you %d coins. Are you interested?"
    dialog_copy_npc_class_specific_msg(buffer, state, 1000);
    sprintf(state->reply, buffer, amt);

    // PC: "Yes."
    dialog_copy_pc_generic_msg(state->options[0], state, 1, 99);
    state->field_17F0[0] = 4;
    state->field_1804[0] = amt;
    state->actions[0] = NULL;

    // PC: "No."
    dialog_copy_pc_generic_msg(state->options[1], state, 100, 199);
    state->field_17F0[1] = a4;
    state->field_1804[1] = a5;
    state->actions[1] = NULL;

    state->num_options = 2;

    state->field_17F0[2] = param1;
    state->field_1804[2] = param2;
}

// 0x418B30
void sub_418B30(int a1, DialogState* a2)
{
    if (item_gold_get(a2->pc_obj) < a1) {
        sub_418C40(2000, a2->field_17F0[1], a2->field_1804[1], a2);
    } else {
        item_gold_transfer(a2->pc_obj, a2->npc_obj, a1, OBJ_HANDLE_NULL);
        sub_414810(a2->field_17F0[2], a2->field_1804[2], a2->field_1818[2], 2, a2);
    }
}

// 0x418C40
void sub_418C40(int a1, int a2, int a3, DialogState* a4)
{
    dialog_copy_npc_class_specific_msg(a4->reply, a4, a1);
    a4->num_options = 1;
    dialog_copy_pc_generic_msg(a4->options[0], a4, 600, 699);
    a4->field_17F0[0] = a2;
    a4->field_1804[0] = a3;
    a4->actions[0] = NULL;
}

// 0x418CA0
void dialog_offer_training(int* skills, int cnt, int back_response_val, DialogState* state)
{
    int index;

    // "What type of training are you seeking?"
    dialog_copy_npc_class_specific_msg(state->reply, state, 3000);

    // Set the number of available options to the number of skills plus one for
    // "Forget it" option.
    state->num_options = cnt + 1;

    // Iterate through offered skills and use their names as options.
    for (index = 0; index < cnt; index++) {
        if (IS_TECH_SKILL(skills[index])) {
            strcpy(state->options[index], tech_skill_name(GET_TECH_SKILL(skills[index])));
        } else {
            strcpy(state->options[index], basic_skill_name(GET_BASIC_SKILL(skills[index])));
        }

        state->field_17F0[index] = 6;
        state->field_1804[index] = skills[index];
        state->actions[index] = NULL;
    }

    // Last option - "Forget it".
    state->actions[cnt] = NULL;
    dialog_copy_pc_generic_msg(state->options[cnt], state, 800, 899);
    sub_417590(back_response_val, &(state->field_17F0[cnt]), &(state->field_1804[cnt]));
}

// 0x418DE0
void dialog_ask_money_for_training(int skill, DialogState* state)
{
    int index;

    index = state->num_options - 1;
    if (IS_TECH_SKILL(skill)) {
        // Check if PC is already trained.
        if (tech_skill_training_get(state->pc_obj, GET_TECH_SKILL(skill)) != TRAINING_NONE) {
            // "You already have received apprentice training in that skill."
            sub_418C40(4000, state->field_17F0[index], state->field_1804[index], state);
            return;
        }

        // Check if PC can be trained - attempt to temporarily grant apprentice
        // training.
        if (tech_skill_training_set(state->pc_obj, GET_TECH_SKILL(skill), TRAINING_APPRENTICE) == TRAINING_NONE) {
            // "You are not skilled enough to be trained as an apprentice."
            sub_418C40(5000, state->field_17F0[index], state->field_1804[index], state);
            return;
        }

        // Apprentice training was successfully applied, set it back to none...
        tech_skill_training_set(state->pc_obj, GET_TECH_SKILL(skill), TRAINING_NONE);

        // ...and ask for 100 coins.
        dialog_ask_money(100, 7, skill, state->field_17F0[index], state->field_1804[index], state);
    } else {
        // NOTE: The same flow as above (using basic skill function set).
        if (basic_skill_training_get(state->pc_obj, GET_BASIC_SKILL(skill)) != TRAINING_NONE) {
            sub_418C40(4000, state->field_17F0[index], state->field_1804[index], state);
            return;
        }

        if (basic_skill_training_set(state->pc_obj, GET_BASIC_SKILL(skill), TRAINING_APPRENTICE) == TRAINING_NONE) {
            sub_418C40(5000, state->field_17F0[index], state->field_1804[index], state);
            return;
        }

        basic_skill_training_set(state->pc_obj, GET_BASIC_SKILL(skill), TRAINING_NONE);
        dialog_ask_money(100, 7, skill, state->field_17F0[index], state->field_1804[index], state);
    }
}

// 0x418F30
void dialog_perform_training(int skill, DialogState* state)
{
    if (IS_TECH_SKILL(skill)) {
        tech_skill_training_set(state->pc_obj, GET_TECH_SKILL(skill), TRAINING_APPRENTICE);
    } else {
        basic_skill_training_set(state->pc_obj, GET_BASIC_SKILL(skill), TRAINING_APPRENTICE);
    }

    // "Your apprentice training is complete."
    dialog_copy_npc_class_specific_msg(state->reply, state, 6000);

    // Appreciate NPC for training.
    state->num_options = 1;
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000); // "Thank you, sir."
    state->field_17F0[0] = state->field_17F0[1];
    state->field_1804[0] = state->field_1804[1];
    state->actions[0] = NULL;
}

// 0x418FC0
void dialog_ask_money_for_rumor(int cost, int* rumors, int num_rumors, int response_val, DialogState* state)
{
    int index;
    int num_known_rumors = 0;
    int v1;
    int v2;

    for (index = 0; index < num_rumors; index++) {
        if (!rumor_qstate_get(rumors[index])
            && !rumor_known_get(state->pc_obj, rumors[index])) {
            rumors[num_known_rumors++] = rumors[index];
        }
    }

    sub_417590(response_val, &v1, &v2);

    if (num_known_rumors != 0) {
        index = random_between(0, num_known_rumors - 1);
        if (cost > 0) {
            dialog_ask_money(cost, 9, rumors[index], v1, v2, state);
        } else {
            dialog_tell_rumor(rumors[index], v1, v2, state);
        }
    } else {
        // NPC: "I have no information to impart."
        dialog_copy_npc_class_specific_msg(state->reply, state, 7000);

        // PC: "[continue]"
        dialog_copy_pc_generic_msg(state->options[0], state, 600, 699);
        state->field_17F0[0] = v1;
        state->field_1804[0] = v2;

        state->num_options = 1;

        state->actions[0] = NULL;
    }
}

// 0x4190E0
void dialog_tell_rumor(int rumor, int a2, int a3, DialogState* state)
{
    char buffer[1000];

    // NPC: Rumor-specific.
    rumor_copy_interaction_str(state->pc_obj, state->npc_obj, rumor, buffer);
    sub_416B00(state->reply, buffer, state);
    state->speech_id = -1;

    rumor_known_set(state->pc_obj, rumor);

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    state->field_17F0[0] = a2;
    state->field_1804[0] = a3;

    state->num_options = 1;

    state->actions[0] = NULL;
}

// 0x419190
void dialog_build_pc_insult_option(int index, int a2, int a3, DialogState* state)
{
    dialog_copy_pc_class_specific_msg(state->options[index], state, 3000);
    state->field_17F0[index] = 10;
    state->field_1804[index] = a2;
    state->field_1818[index] = a3;
}

// 0x4191E0
void dialog_insult_reply(int a1, int a2, DialogState* state)
{
    reaction_adj(state->npc_obj, state->pc_obj, -10);
    if (reaction_get(state->npc_obj, state->pc_obj) > 20) {
        sub_414810(a1, a2, 0, 0, state);
    } else {
        // NPC: "You insolent worm! How dare you?"
        dialog_copy_npc_race_specific_msg(state->reply, state, 1000);
        state->num_options = 0;
        state->field_17E8 = 4;
    }
}

// 0x419260
void sub_419260(DialogState* a1, const char* str)
{
    unsigned int flags[10];
    int values[10];
    unsigned int spell_flags;
    tig_art_id_t aid;
    int armor_type;

    memset(flags, 0, sizeof(flags));

    if (str != NULL) {
        int index;
        int value;

        while (*str != '\0') {
            while (!SDL_isdigit(*str)) {
                str++;
            }

            if (*str == '\0') {
                break;
            }

            index = atoi(str);

            while (*str != '\0' && *str != ' ') {
                str++;
            }

            if (*str == '\0') {
                break;
            }

            // TODO: Refactor.
            if (*str != 'f') {
                value = atoi(str);
                flags[index] = value != 0 ? 0x4 : 0x2;
                values[index] = value;
            } else if (str[1] == 'l') {
                value = atoi(str + 2);
                flags[index] = 0x1;
                values[index] = value;
            } else {
                break;
            }

            while (*str != '\0' && *str != ',') {
                str++;
            }
        }
    }

    if ((flags[0] & 0x2) == 0 && critter_is_dead(a1->npc_obj)) {
        if (!sub_4197D0(flags[0], values[0], a1)) {
            dialog_copy_npc_generic_msg(a1->reply, a1, 1500, 1599);
        }
        return;
    }

    if (ai_surrendered(a1->npc_obj, NULL)) {
        dialog_copy_npc_fleeing_msg(a1->npc_obj, a1->pc_obj, a1->reply, &(a1->speech_id));
        return;
    }

    if ((flags[1] & 0x2) == 0 && critter_has_bad_associates(a1->pc_obj)) {
        if (!sub_4197D0(flags[1], values[1], a1)) {
            dialog_copy_npc_class_specific_msg(a1->reply, a1, 18000);

            if (critter_social_class_get(a1->npc_obj) != SOCIAL_CLASS_WIZARD) {
                a1->field_17E8 = 4;
            }
        }
        return;
    }

    spell_flags = obj_field_int32_get(a1->pc_obj, OBJ_F_SPELL_FLAGS);

    if ((flags[2] & 0x2) == 0 && (spell_flags & OSF_INVISIBLE) != 0) {
        if (!sub_4197D0(flags[2], values[2], a1)) {
            dialog_copy_npc_class_specific_msg(a1->reply, a1, 22000);

            if (critter_social_class_get(a1->npc_obj) != SOCIAL_CLASS_WIZARD) {
                a1->field_17E8 = 4;
            }
        }
        return;
    }

    if ((flags[3] & 0x2) == 0 && (spell_flags & (OSF_BODY_OF_AIR | OSF_BODY_OF_EARTH | OSF_BODY_OF_FIRE | OSF_BODY_OF_WATER)) != 0) {
        if (!sub_4197D0(flags[3], values[3], a1)) {
            dialog_copy_npc_class_specific_msg(a1->reply, a1, 19000);

            if (critter_social_class_get(a1->npc_obj) != SOCIAL_CLASS_WIZARD) {
                a1->field_17E8 = 4;
            }
        }
        return;
    }

    if ((flags[4] & 0x2) == 0 && (spell_flags & OSF_POLYMORPHED) != 0) {
        if (!sub_4197D0(flags[4], values[4], a1)) {
            dialog_copy_npc_class_specific_msg(a1->reply, a1, 20000);

            if (critter_social_class_get(a1->npc_obj) != SOCIAL_CLASS_WIZARD) {
                a1->field_17E8 = 4;
            }
        }
        return;
    }

    if ((flags[6] & 0x2) == 0 && (spell_flags & OSF_SHRUNK) != 0) {
        if (!sub_4197D0(flags[6], values[6], a1)) {
            dialog_copy_npc_race_specific_msg(a1->reply, a1, 6000);
        }
        return;
    }

    aid = obj_field_int32_get(a1->pc_obj, OBJ_F_CURRENT_AID);
    armor_type = tig_art_critter_id_armor_get(aid);

    if ((flags[7] & 0x2) == 0
        && armor_type == TIG_ART_ARMOR_TYPE_UNDERWEAR) {
        if (!sub_4197D0(flags[7], values[7], a1)) {
            dialog_copy_npc_class_specific_msg(a1->reply, a1, 16000);
        }
        return;
    }

    if ((flags[8] & 0x2) == 0
        && armor_type == TIG_ART_ARMOR_TYPE_BARBARIAN) {
        if (!sub_4197D0(flags[8], values[8], a1)) {
            dialog_copy_npc_class_specific_msg(a1->reply, a1, 17000);
        }
        return;
    }

    if ((flags[9] & 0x2) == 0) {
        if (!sub_4197D0(flags[9], values[9], a1)) {
            int reaction_level;
            int reputation;

            if (random_between(1, 100) <= 20) {
                reputation = reputation_pick(a1->pc_obj, a1->npc_obj);
                if (reputation != 0) {
                    reputation_copy_greeting(a1->pc_obj, a1->npc_obj, reputation, a1->reply);
                    a1->speech_id = -1;
                    if (a1->reply[0] != '\0') {
                        return;
                    }
                }
            }

            reaction_level = reaction_translate(reaction_get(a1->npc_obj, a1->pc_obj));
            if (reaction_met_before(a1->npc_obj, a1->pc_obj)) {
                switch (reaction_level) {
                case REACTION_LOVE:
                    dialog_copy_npc_class_specific_msg(a1->reply, a1, 11000);
                    break;
                case REACTION_AMIABLE:
                case REACTION_COURTEOUS:
                    dialog_copy_npc_class_specific_msg(a1->reply, a1, 12000);
                    break;
                case REACTION_NEUTRAL:
                    dialog_copy_npc_class_specific_msg(a1->reply, a1, 13000);
                    break;
                case REACTION_SUSPICIOUS:
                case REACTION_DISLIKE:
                    dialog_copy_npc_race_specific_msg(a1->reply, a1, 4000);
                    break;
                case REACTION_HATRED:
                    dialog_copy_npc_race_specific_msg(a1->reply, a1, 5000);
                    break;
                }
            } else {
                switch (reaction_level) {
                case REACTION_LOVE:
                    dialog_copy_npc_class_specific_msg(a1->reply, a1, 8000);
                    break;
                case REACTION_AMIABLE:
                case REACTION_COURTEOUS:
                    dialog_copy_npc_class_specific_msg(a1->reply, a1, 9000);
                    break;
                case REACTION_NEUTRAL:
                    dialog_copy_npc_class_specific_msg(a1->reply, a1, 10000);
                    break;
                case REACTION_SUSPICIOUS:
                case REACTION_DISLIKE:
                    dialog_copy_npc_race_specific_msg(a1->reply, a1, 2000);
                    break;
                case REACTION_HATRED:
                    dialog_copy_npc_race_specific_msg(a1->reply, a1, 3000);
                    break;
                }
            }
        }
        return;
    }
}

// 0x4197D0
bool sub_4197D0(unsigned int flags, int a2, DialogState* a3)
{
    if ((flags & 0x1) != 0) {
        a3->num = a2;
        sub_413A30(a3, 0);
        a3->field_17E8 = 4;
        return true;
    }

    if ((flags & 0x4) != 0) {
        a3->field_17EC = a2;
        a3->field_17E8 = 0;
        sub_414E60(a3, 0);
        return true;
    }

    return false;
}

// 0x419830
void dialog_offer_healing(DialogHealingOfferType type, int response_val, DialogState* state)
{
    int lvl;
    int64_t item_obj;
    int cnt;
    int index;

    // NPC: "What type of healing would you be seeking?"
    dialog_copy_npc_class_specific_msg(state->reply, state, 14000);

    lvl = spell_college_level_get(state->npc_obj, COLLEGE_NECROMANTIC_WHITE);
    cnt = 0;

    switch (type) {
    case DIALOG_HEALING_OFFER_OPTIONS:
        item_obj = skill_supplementary_item(state->npc_obj, SKILL_HEAL);
        if (ai_check_use_skill(state->npc_obj, state->pc_obj, item_obj, SKILL_HEAL) == AI_USE_SKILL_OK) {
            dialog_build_use_skill_option(cnt, SKILL_HEAL, response_val, state);
            cnt++;
        }
        if (lvl >= 1) {
            // PC: "[Magickal Healing]"
            dialog_copy_pc_generic_msg(state->options[cnt], state, 1100, 1199);
            state->field_17F0[cnt] = 12;
            state->field_1804[cnt] = response_val;
            cnt++;
        }
        if (lvl >= 2) {
            // PC: "[Magickal Poison Healing]"
            dialog_copy_pc_generic_msg(state->options[cnt], state, 1200, 1299);
            state->field_17F0[cnt] = 13;
            state->field_1804[cnt] = response_val;
            cnt++;
        }
        if (lvl == 5) {
            dialog_build_use_spell_option(cnt, SPELL_RESURRECT, response_val, state);
            cnt++;
        }
        break;
    case DIALOG_HEALING_OFFER_MAGICKAL_HEALING:
        if (lvl >= 1) {
            dialog_build_use_spell_option(cnt, SPELL_MINOR_HEALING, response_val, state);
            cnt++;
        }
        if (lvl >= 3) {
            dialog_build_use_spell_option(cnt, SPELL_MAJOR_HEALING, response_val, state);
            cnt++;
        }
        break;
    case DIALOG_HEALING_OFFER_MAGICKAL_POISON_HEALING:
        if (lvl >= 2) {
            dialog_build_use_spell_option(cnt, SPELL_HALT_POISON, response_val, state);
            cnt++;
        }
        break;
    }

    // PC: "Forget it."
    dialog_copy_pc_generic_msg(state->options[cnt], state, 800, 899);
    sub_417590(response_val, &(state->field_17F0[cnt]), &(state->field_1804[cnt]));

    state->num_options = cnt + 1;

    for (index = 0; index < state->num_options; index++) {
        state->actions[index] = NULL;
    }
}

// 0x419A00
void dialog_build_use_skill_option(int index, int skill, int response_val, DialogState* state)
{
    char buffer[1000];
    const char* name;

    // PC: "[%s Skill]"
    dialog_copy_pc_generic_msg(buffer, state, 900, 999);

    if (IS_TECH_SKILL(skill)) {
        name = tech_skill_name(GET_TECH_SKILL(skill));
    } else {
        name = basic_skill_name(GET_BASIC_SKILL(skill));
    }

    sprintf(state->options[index], buffer, name);
    state->field_17F0[index] = 14;
    state->field_1804[index] = skill;
    state->field_1818[index] = response_val;
}

// 0x419AC0
void dialog_build_use_spell_option(int index, int spell, int response_val, DialogState* state)
{
    char buffer[1000];

    // PC: "[%s Spell]"
    dialog_copy_pc_generic_msg(buffer, state, 1000, 1099);

    sprintf(state->options[index], buffer, spell_name(spell));
    state->field_17F0[index] = 15;
    state->field_1804[index] = spell;
    state->field_1818[index] = response_val;
}

// 0x419B50
void dialog_ask_money_for_skill(int skill, int response_val, DialogState* state)
{
    int v1;
    int v2;
    int skill_level;
    int training;
    int amt;

    sub_417590(response_val, &v1, &v2);
    if (critter_leader_get(state->npc_obj) == state->pc_obj) {
        dialog_use_skill(skill, v1, v2, state);
    } else {
        if (IS_TECH_SKILL(skill)) {
            skill_level = tech_skill_level(state->npc_obj, GET_TECH_SKILL(skill));
            training = tech_skill_training_get(state->npc_obj, GET_TECH_SKILL(skill));
            amt = tech_skill_money(GET_TECH_SKILL(skill), skill_level, training);
        } else {
            skill_level = basic_skill_level(state->npc_obj, GET_BASIC_SKILL(skill));
            training = basic_skill_training_get(state->npc_obj, GET_BASIC_SKILL(skill));
            amt = basic_skill_money(GET_BASIC_SKILL(skill), skill_level, training);
        }
        dialog_ask_money(amt, 16, skill, v1, v2, state);
    }
}

// 0x419C30
void dialog_ask_money_for_spell(int spell, int response_val, DialogState* state)
{
    int v1;
    int v2;
    int amt;

    sub_417590(response_val, &v1, &v2);
    if (critter_leader_get(state->npc_obj) == state->pc_obj) {
        dialog_use_spell(spell, v1, v2, state);
    } else {
        amt = spell_money(spell);
        dialog_ask_money(amt, 17, spell, v1, v2, state);
    }
}

// 0x419CB0
void dialog_use_skill(int skill, int a2, int a3, DialogState* state)
{
    unsigned int flags;
    int64_t item_obj;

    if (critter_pc_leader_get(state->npc_obj) != OBJ_HANDLE_NULL) {
        flags = 0;
    } else {
        flags = 0x2000;
    }

    item_obj = skill_supplementary_item(state->npc_obj, skill);
    anim_goal_use_skill_on(state->npc_obj, state->pc_obj, item_obj, skill, flags);

    // NPC: "It is done."
    dialog_copy_npc_class_specific_msg(state->reply, state, 15000);

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    state->field_17F0[0] = a2;
    state->field_1804[0] = a3;
    state->actions[0] = NULL;

    state->num_options = 1;
}

// 0x419D50
void dialog_use_spell(int spell, int a2, int a3, DialogState* state)
{
    MagicTechInvocation mt_invocation;

    magictech_invocation_init(&mt_invocation, state->npc_obj, spell);
    sub_4440E0(state->pc_obj, &(mt_invocation.target_obj));
    if (critter_pc_leader_get(state->npc_obj) == OBJ_HANDLE_NULL) {
        mt_invocation.flags |= MAGICTECH_INVOCATION_FREE;
    }
    magictech_invocation_run(&mt_invocation);

    // NPC: "It is done."
    dialog_copy_npc_class_specific_msg(state->reply, state, 15000);

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    state->field_17F0[0] = a2;
    state->field_1804[0] = a3;
    state->actions[0] = NULL;

    state->num_options = 1;
}

// 0x419E20
int filter_unknown_areas(int64_t obj, int* areas, int cnt)
{
    int unknown = 0;
    int index;

    for (index = 0; index < cnt; index++) {
        if (!area_is_known(obj, areas[index])) {
            areas[unknown++] = areas[index];
        }
    }

    return unknown;
}

// 0x419E70
void dialog_offer_directions(const char* str, int response_val, int offset, bool mark, DialogState* state)
{
    char buffer[1000];
    char* pch;
    int cost;
    int cnt;
    int areas[100];
    int idx;

    strcpy(buffer, str);

    pch = strchr(buffer, '$');
    cost = atoi(pch + 1);

    pch = strchr(pch, ',');
    cnt = dialog_parse_params(areas, pch + 1);

    cnt = filter_unknown_areas(state->pc_obj, areas, cnt);

    if (mark) {
        // NPC: "What place do you want marked on your map?"
        dialog_copy_npc_generic_msg(state->reply, state, 500, 599);
    } else {
        // NPC: "Where do you need directions to?"
        dialog_copy_npc_generic_msg(state->reply, state, 100, 199);
    }

    if (critter_leader_get(state->npc_obj) == state->pc_obj) {
        state->field_17EC = 0;
    } else {
        state->field_17EC = cost;
    }

    // PC: Up to 4 options, each is the name of the area.
    for (idx = 0; idx < 4; idx++) {
        if (offset + idx >= cnt) {
            break;
        }

        strcpy(state->options[idx], area_get_name(areas[offset + idx]));
        state->field_17F0[idx] = mark ? 22 : 19;
        state->field_1804[idx] = areas[offset + idx];
        state->field_1818[idx] = response_val;
    }

    if (offset + idx < cnt) {
        // PC: "[continue]"
        dialog_copy_pc_generic_msg(state->options[idx], state, 600, 699);
        state->field_17F0[idx] = mark ? 21 : 18;
        state->field_1804[idx] = response_val;
        state->field_1818[idx] = offset + idx;
    } else {
        // PC: "Forget it."
        dialog_copy_pc_generic_msg(state->options[idx], state, 800, 899);
        sub_417590(response_val, &(state->field_17F0[idx]), &(state->field_1804[idx]));
    }

    state->num_options = idx + 1;

    for (idx = 0; idx < state->num_options; idx++) {
        state->actions[idx] = NULL;
    }
}

// 0x41A0F0
void dialog_ask_money_for_directions(int cost, int area, int response_val, DialogState* state)
{
    int v1;
    int v2;

    sub_417590(response_val, &v1, &v2);
    if (cost > 0) {
        dialog_ask_money(cost, 20, area, v1, v2, state);
    } else {
        dialog_give_directions(area, v1, v2, state);
    }
}

// 0x41A150
void dialog_give_directions(int area, int a2, int a3, DialogState* state)
{
    int64_t src_loc;
    int64_t dst_loc;
    int rot;
    int64_t leagues;
    int base;

    src_loc = obj_field_int64_get(state->npc_obj, OBJ_F_LOCATION);
    dst_loc = area_get_location(area);
    rot = location_rot(src_loc, dst_loc);
    leagues = location_dist(src_loc, dst_loc) / 3168;

    if (leagues < 2) {
        // Neary: "It is just a stone's throw away to the north."
        base = 200;
    } else if (leagues < 100) {
        // Medium: "It is to the north a ways."
        base = 300;
    } else {
        // Far: "It is a long ways going to the north. A long ways."
        base = 400;
    }

    // NPC: Distance/rotation-specific.
    dialog_copy_npc_generic_msg(state->reply, state, base + 10 * rot, base + 10 * rot + 9);

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    state->field_17F0[0] = a2;
    state->field_1804[0] = a3;
    state->actions[0] = NULL;

    state->num_options = 1;
}

// 0x41A230
void dialog_ask_money_for_mark_area(int cost, int area, int response_val, DialogState* state)
{
    int v1;
    int v2;

    sub_417590(response_val, &v1, &v2);
    if (cost > 0) {
        dialog_ask_money(cost, 23, area, v1, v2, state);
    } else {
        dialog_mark_area(area, v1, v2, state);
    }
}

// 0x41A290
void dialog_mark_area(int area, int a2, int a3, DialogState* state)
{
    int64_t loc;
    int64_t area_loc;
    int rot;
    int64_t leagues;
    int base;
    char buffer[1000];

    loc = obj_field_int64_get(state->npc_obj, OBJ_F_LOCATION);
    area_loc = area_get_location(area);
    rot = location_rot(loc, area_loc);
    leagues = location_dist(loc, area_loc) / 3168;

    if (leagues < 2) {
        // Nearby: "It is very near. I have marked it just to the north."
        base = 600;
    } else {
        // Non-nearby: "It is marked on your map. Not more than %d leagues to the north."
        base = 700;
    }

    // NPC: Distance/rotation-specific.
    dialog_copy_npc_generic_msg(buffer, state, base + 10 * rot, base + 10 * rot + 9);

    if (leagues < 2) {
        strcpy(state->reply, buffer);
    } else {
        sprintf(state->reply, buffer, leagues);
    }

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    state->field_17F0[0] = a2;
    state->field_1804[0] = a3;
    state->actions[0] = NULL;

    state->num_options = 1;

    area_set_known(state->pc_obj, area);
}

// 0x41A3E0
void dialog_check_story(int response_val, DialogState* state)
{
    // NPC: Story-specific.
    dialog_copy_npc_story_msg(state->reply, state);

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    sub_417590(response_val, &(state->field_17F0[0]), &(state->field_1804[0]));

    state->actions[0] = NULL;

    state->num_options = 1;
}

// 0x41A440
void dialog_copy_npc_story_msg(char* buffer, DialogState* state)
{
    int story_state;
    int gd;
    MesFileEntry mes_file_entry;

    story_state = script_story_state_get();
    if (dialog_copy_npc_override_msg(buffer, state, story_state + 10000)) {
        return;
    }

    if (stat_level_get(state->npc_obj, STAT_GENDER) == GENDER_MALE) {
        gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_STO_M2F
            : GD_STO_M2M;
    } else {
        gd = stat_level_get(state->pc_obj, STAT_GENDER) == GENDER_MALE
            ? GD_STO_F2M
            : GD_STO_F2F;
    }

    dialog_load_generated(gd);

    mes_file_entry.num = stat_level_get(state->npc_obj, STAT_RACE) + 100 * story_state;
    mes_get_msg(dialog_mes_files[gd], &mes_file_entry);
    sub_416B00(buffer, mes_file_entry.str, state);
    state->speech_id = -1;
}

// 0x41A520
void dialog_ask_about_buying_newspapers(int response_val, DialogState* state)
{
    // NPC: "What newspaper are you looking for?"
    dialog_copy_npc_generic_msg(state->reply, state, 4200, 4299);

    // PC: "I want to buy today's newspaper."
    dialog_copy_pc_generic_msg(state->options[0], state, 2200, 2299);
    state->field_17F0[0] = 28;
    state->field_1804[0] = newspaper_get(NEWSPAPER_CURRENT);
    state->field_1818[0] = response_val;

    // PC: "What is the headline of today's paper?"
    dialog_copy_pc_generic_msg(state->options[1], state, 2000, 2099);
    state->field_17F0[1] = 26;
    state->field_1804[1] = response_val;

    // PC: "I would like see some older newspapers."
    dialog_copy_pc_generic_msg(state->options[2], state, 2100, 2199);
    state->field_17F0[2] = 27;
    state->field_1804[2] = response_val;

    // PC: "Forget it."
    dialog_copy_pc_generic_msg(state->options[3], state, 800, 899);
    sub_417590(response_val, &(state->field_17F0[3]), &(state->field_1804[3]));

    state->actions[0] = NULL;
    state->actions[1] = NULL;
    state->actions[2] = NULL;
    state->actions[3] = NULL;

    state->num_options = 4;
}

// 0x41A620
void dialog_offer_today_newspaper(int response_val, DialogState* state)
{
    int newspaper;
    size_t pos;

    newspaper = newspaper_get(NEWSPAPER_CURRENT);
    ui_written_newspaper_headline(newspaper, state->reply);

    pos = strlen(state->reply);
    if (pos > 0) {
        if (state->reply[pos - 1] != '.'
            && state->reply[pos - 1] != '?'
            && state->reply[pos - 1] != '!') {
            state->reply[pos++] = '.';
        }

        state->reply[pos++] = ' ';
        state->reply[pos++] = '\0';
    }

    // NPC: "Do you want to buy this newspaper?"
    dialog_copy_npc_generic_msg(&(state->reply[pos]), state, 4300, 4399);

    // PC: "Yes."
    dialog_copy_pc_generic_msg(state->options[0], state, 1, 99);
    state->field_17F0[0] = 28;
    state->field_1804[0] = newspaper;
    state->field_1818[0] = response_val;

    // PC: "No."
    dialog_copy_pc_generic_msg(state->options[1], state, 100, 199);
    sub_417590(response_val, &state->field_17F0[1], &state->field_1804[1]);

    state->actions[0] = NULL;
    state->actions[1] = NULL;

    state->num_options = 2;
}

// 0x41A700
void dialog_offer_older_newspaper(int response_val, DialogState* state)
{
    int cnt;
    int index;
    int newspapers[4];

    cnt = 0;
    for (index = 0; index < 4; index++) {
        newspapers[cnt] = newspaper_get(index);
        if (newspapers[cnt] != -1) {
            cnt++;
        }
    }

    if (cnt != 0) {
        // NPC: "I have these older newspapers for sale."
        dialog_copy_npc_generic_msg(state->reply, state, 4500, 4599);

        // PC: Each option is a newspaper headline.
        for (index = 0; index < cnt; index++) {
            ui_written_newspaper_headline(newspapers[index], state->options[index]);
            state->field_17F0[index] = 28;
            state->field_1804[index] = newspapers[index];
        }

        // PC: "Forget it."
        dialog_copy_pc_generic_msg(state->options[cnt], state, 800, 899);
        sub_417590(response_val, &(state->field_17F0[cnt]), &(state->field_1804[cnt]));

        state->num_options = cnt + 1;
    } else {
        // NPC: "I'm sorry. I don't have any older papers."
        dialog_copy_npc_generic_msg(state->reply, state, 4400, 4499);

        // PC: "[continue]"
        dialog_copy_pc_generic_msg(state->options[0], state, 600, 699);
        sub_417590(response_val, &(state->field_17F0[0]), &(state->field_1804[0]));

        state->num_options = 1;
    }

    for (index = 0; index < state->num_options; index++) {
        state->actions[index] = NULL;
    }
}

// 0x41A880
void dialog_ask_money_for_newspaper(int newspaper, int response_val, DialogState* state)
{
    int v1;
    int v2;

    sub_417590(response_val, &v1, &v2);
    dialog_ask_money(2, 29, newspaper, v1, v2, state);
}

// 0x41A8C0
void dialog_buy_newspaper(int newspaper, int a2, int a3, DialogState* state)
{
    int64_t loc;
    int64_t obj;

    loc = obj_field_int64_get(state->pc_obj, OBJ_F_LOCATION);
    obj = newspaper_create(newspaper, loc);
    item_transfer(obj, state->pc_obj);

    // NPC: "It is completed."
    dialog_copy_npc_class_specific_msg(state->reply, state, 15000);

    // PC: "Thank you."
    dialog_copy_pc_class_specific_msg(state->options[0], state, 1000);
    state->field_17F0[0] = a2;
    state->field_1804[0] = a3;

    state->actions[0] = NULL;
}
