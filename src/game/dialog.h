#ifndef ARCANUM_GAME_DIALOG_H_
#define ARCANUM_GAME_DIALOG_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/object.h"

typedef struct DialogState {
    /* 0000 */ int dlg;
    /* 0008 */ int64_t pc_obj;
    /* 0010 */ Ryan field_10;
    /* 0038 */ int64_t npc_obj;
    /* 0040 */ Ryan field_40;
    /* 0068 */ int num;
    /* 006C */ int script_num;
    /* 0070 */ char reply[1000];
    /* 0458 */ int speech_id;
    /* 045C */ int num_options;
    /* 0460 */ char options[5][1000];
    /* 17E8 */ int field_17E8;
    /* 17EC */ int field_17EC;
    /* 17F0 */ int field_17F0[5];
    /* 1804 */ int field_1804[5];
    /* 1818 */ int field_1818[5];
    /* 182C */ char* actions[5];
    /* 1840 */ int field_1840;
    /* 1844 */ unsigned int seed;
} DialogState;

bool dialog_init(GameInitInfo* init_info);
void dialog_exit(void);
bool dialog_load(const char* path, int* dlg_ptr);
void dialog_unload(int dlg);
void sub_412F60(int dlg);
bool sub_412FD0(DialogState* state);
void sub_413130(DialogState* state, int index);
void sub_413280(DialogState* state);
void sub_4132A0(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_farewell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_normally_wont_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_buy_stolen_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_normally_wont_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_buy_for_less_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_not_enough_money_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_let_me_handle_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void sub_413A30(DialogState* a1, bool a2);
void dialog_copy_npc_wont_follow_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_order_ok_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_order_no_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_insult_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_accidental_attack_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_fleeing_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_witness_pc_critical_msg(int64_t npc_obj, int64_t pc_obj, int type, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_gamble_msg(int64_t npc_obj, int64_t pc_obj, int type, char* buffer);
void dialog_copy_npc_warning_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_newspaper_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_warning_follow_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_upset_attacking_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_near_death_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_wont_identify_already_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_identify_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_repair_broken_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_repair_undamaged_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_repair_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_check(void);
void dialog_enable_numbers(void);
void sub_418A00(int a1, int* a2, int* a3);

#endif /* ARCANUM_GAME_DIALOG_H_ */
