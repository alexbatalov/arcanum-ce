#include "game/broadcast.h"

#include "game/ai.h"
#include "game/anim.h"
#include "game/critter.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/player.h"
#include "game/text_filter.h"
#include "game/ui.h"

typedef enum BroadcastCommandType {
    BROADCAST_CMD_TYPE_NONE,
    BROADCAST_CMD_TYPE_LEAVE,
    BROADCAST_CMD_TYPE_WAIT,
    BROADCAST_CMD_TYPE_FOLLOW,
    BROADCAST_CMD_TYPE_MOVE,
    BROADCAST_CMD_TYPE_STAY_CLOSE,
    BROADCAST_CMD_TYPE_SPREAD_OUT,
    BROADCAST_CMD_TYPE_CURSE,
    BROADCAST_CMD_TYPE_JOIN,
    BROADCAST_CMD_TYPE_DISBAND,
    BROADCAST_CMD_TYPE_ATTACK,
    BROADCAST_CMD_TYPE_WALK_HERE,
    BROADCAST_CMD_TYPE_BACK_OFF,
    BROADCAST_CMD_TYPE_FOLLOW_OTHER,
    BROADCAST_CMD_TYPE_COUNT,
} BroadcastCommandType;

typedef struct BroadcastCommand {
    char* str;
    int type;
} BroadcastCommand;

static int broadcast_match_str_to_base_type(const char* str);
static int sub_4C3B40(const char* str);
static void sub_4C3B90(void);

// 0x5FDC4C
static char* broadcast_cmd_type_lookup[BROADCAST_CMD_TYPE_COUNT];

// 0x5FDC84
static mes_file_handle_t broadcast_mes_file;

// 0x5FDC88
static BroadcastFloatLineFunc* broadcast_float_line_func;

// 0x5FDC8C
static mes_file_handle_t broadcast_multiplayer_mes_file;

// 0x5FDC90
static char byte_5FDC90[6][1000];

// 0x5FF400
static bool broadcast_initialized;

// 0x5FF404
static BroadcastCommand* broadcast_commands;

// 0x5FF408
static int num_broadcast_commands;

// 0x4C2C10
bool broadcast_init(GameInitInfo* init_info)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    int index;

    (void)init_info;

    broadcast_float_line_func = NULL;

    if (!mes_load("mes\\broadcast.mes", &broadcast_mes_file)) {
        tig_debug_printf("broadcast_init: ERROR: couldn't load message file: mes\\broadcast.mes!\n");
        exit(EXIT_FAILURE);
    }

    if (!mes_load("mes\\MultiPlayer.mes", &broadcast_multiplayer_mes_file)) {
        tig_debug_printf("broadcast_init: ERROR: couldn't load message file: %s!\n", "mes\\MultiPlayer.mes");
        exit(EXIT_FAILURE);
    }

    mes_file_entry.num = 100;
    while (mes_search(broadcast_mes_file, &mes_file_entry)) {
        mes_get_msg(broadcast_mes_file, &mes_file_entry);
        broadcast_commands = (BroadcastCommand*)REALLOC(broadcast_commands, sizeof(*broadcast_commands) * (num_broadcast_commands + 1));
        if (broadcast_commands == NULL) {
            tig_debug_printf("broadcast_init: ERROR: realloc failed!\n");
            exit(EXIT_FAILURE);
        }
        broadcast_commands[num_broadcast_commands++].str = mes_file_entry.str;
        mes_file_entry.num++;
    }

    if (!mes_load("Rules\\BroadCastRules.mes", &mes_file)) {
        tig_debug_printf("broadcast_init: ERROR: couldn't load message file: Rules\\BroadCastRules.mes!\n");
        exit(EXIT_FAILURE);
    }

    tig_str_parse_set_separator(',');

    mes_file_entry.num = 0;
    for (index = 0; index < BROADCAST_CMD_TYPE_COUNT; index++) {
        mes_get_msg(mes_file, &mes_file_entry);
        broadcast_cmd_type_lookup[index] = mes_file_entry.str;
        mes_file_entry.num++;
    }

    mes_file_entry.num = 100;
    for (index = 0; index < num_broadcast_commands; index++) {
        mes_get_msg(mes_file, &mes_file_entry);
        broadcast_commands[index].type = broadcast_match_str_to_base_type(mes_file_entry.str);
        mes_file_entry.num++;
    }

    if (!mes_unload(mes_file)) {
        tig_debug_printf("broadcast_init: ERROR: couldn't unload message file: bcRuleMessID!\n");
    }

    sub_4C3B90();

    if (!text_filter_init()) {
        return false;
    }

    broadcast_initialized = true;

    return true;
}

// 0x4C2E20
void broadcast_exit(void)
{
    if (broadcast_commands != NULL) {
        FREE(broadcast_commands);
        broadcast_commands = NULL;
    }

    num_broadcast_commands = 0;

    if (!mes_unload(broadcast_mes_file)) {
        tig_debug_printf("broadcast_exit: ERROR: couldn't unload message file: bcMessID!\n");
    }

    if (!mes_unload(broadcast_multiplayer_mes_file)) {
        tig_debug_printf("broadcast_exit: ERROR: couldn't unload message file: mp_mes_id!\n");
    }

    text_filter_exit();

    broadcast_initialized = false;
}

// 0x4C2EA0
void broadcast_set_float_line_func(BroadcastFloatLineFunc* func)
{
    broadcast_float_line_func = func;
}

// 0x4C2EB0
int broadcast_match_str_to_base_type(const char* str)
{
    int index;

    for (index = 0; index < BROADCAST_CMD_TYPE_COUNT; index++) {
        if (SDL_strcasecmp(str, broadcast_cmd_type_lookup[index]) == 0) {
            return index;
        }
    }

    tig_debug_printf("broadcast_match_str_to_base_type: ERROR: couldn't map str to type!\n");
    return 0;
}

// 0x4C2F00
void broadcast_msg(int64_t obj, Broadcast* bcast)
{
    if (tig_net_is_active() && !tig_net_is_host()) {
        PacketBroadcastMsg pkt;

        pkt.type = 18;
        pkt.oid = obj_get_id(obj);
        pkt.bcast = *bcast;
        tig_net_send_app_all(&pkt, sizeof(pkt));

        return;
    }

    broadcast_msg_client(obj, bcast);
}

// 0x4C2FA0
void sub_4C2FA0(void)
{
    // TODO: Incomplete.
}

// 0x4C3040
void sub_4C3040(void)
{
    // TODO: Incomplete.
}

// 0x4C31A0
void broadcast_msg_client(int64_t obj, Broadcast* bcast)
{
    // TODO: Incomplete.
}

// 0x4C3B40
int sub_4C3B40(const char* str)
{
    int index;

    while (*str == ' ') {
        str++;
    }

    for (index = 0; index < num_broadcast_commands; index++) {
        if (SDL_strcasecmp(str, broadcast_commands[index].str) == 0) {
            return broadcast_commands[index].type;
        }
    }

    return 0;
}

// 0x4C3B90
void sub_4C3B90(void)
{
    int index;
    MesFileEntry mes_file_entry;

    for (index = 0; index < 6; index++) {
        mes_file_entry.num = 500 + index;
        mes_get_msg(broadcast_mes_file, &mes_file_entry);
        strncpy(byte_5FDC90[index], mes_file_entry.str, 1000);
    }
}

// 0x4C3BE0
void sub_4C3BE0(unsigned int a1, int64_t loc)
{
    Broadcast bcast;

    if (a1 < 6) {
        bcast.loc = loc;
        strcpy(bcast.str, byte_5FDC90[a1]);
        broadcast_msg(player_get_local_pc_obj(), &bcast);
    }
}
