#include "game/party.h"

#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/quest.h"
#include "game/rumor.h"
#include "game/timeevent.h"
#include "game/ui.h"

#define PARTY_TABLE_SIZE 8

static int sub_4BA260(void);
static bool party_update(void);
static void sub_4BA320(int64_t a1, int64_t a2, int a3);

// 0x5FC32C
static int dword_5FC32C[PARTY_TABLE_SIZE];

// 0x5FC34C
static int dword_5FC34C;

// 0x4B9E10
bool party_init(GameInitInfo* init_info)
{
    (void)init_info;

    party_reset();

    return true;
}

// 0x4B9E20
void party_exit(void)
{
    party_reset();
}

// 0x4B9E30
void party_reset(void)
{
    int index;

    for (index = 0; index < PARTY_TABLE_SIZE; index++) {
        dword_5FC32C[index] = -1;
    }
}

// 0x4B9E50
bool party_save(TigFile* stream)
{
    int cnt;

    cnt = PARTY_TABLE_SIZE;
    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        tig_debug_printf("Party: could not save, unable to write table size (%d).\n", cnt);
        return false;
    }

    if (tig_file_fwrite(dword_5FC32C, sizeof(*dword_5FC32C), PARTY_TABLE_SIZE, stream) != PARTY_TABLE_SIZE) {
        tig_debug_printf("Party: could not save, unable to write table.\n", cnt);
        return false;
    }

    return true;
}

// 0x4B9EC0
bool party_load(GameLoadInfo* load_info)
{
    int cnt;

    if (tig_file_fread(&cnt, sizeof(cnt), 1, load_info->stream) != 1) {
        tig_debug_printf("Party: could not load table size.\n");
        return false;
    }

    if (cnt != PARTY_TABLE_SIZE) {
        tig_debug_printf("Party: read table size (%d) does not match current table size (%d)!\n", cnt, PARTY_TABLE_SIZE);
        return false;
    }

    if (tig_file_fread(dword_5FC32C, sizeof(*dword_5FC32C), PARTY_TABLE_SIZE, load_info->stream) != PARTY_TABLE_SIZE) {
        tig_debug_printf("Party: could not read, unable to read table.\n");
        return false;
    }

    return true;
}

// 0x4B9F40
int64_t party_find_first(int64_t obj, int* index_ptr)
{
    int index;

    if (!tig_net_is_active()) {
        return obj;
    }

    index = multiplayer_find_slot_from_obj(obj);
    if (index == -1) {
        return OBJ_HANDLE_NULL;
    }

    if (dword_5FC32C[index] == -1) {
        *index_ptr = PARTY_TABLE_SIZE;
        return obj;
    }

    for (*index_ptr = 0; *index_ptr < PARTY_TABLE_SIZE; (*index_ptr)++) {
        if (dword_5FC32C[*index_ptr] == dword_5FC32C[index]) {
            return sub_4A2B60(*index_ptr);
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x4B9FC0
int64_t party_find_next(int* index_ptr)
{
    int index;

    if (!tig_net_is_active()) {
        return OBJ_HANDLE_NULL;
    }

    index = *index_ptr;
    if (index >= PARTY_TABLE_SIZE) {
        return OBJ_HANDLE_NULL;
    }

    if (dword_5FC32C[index] == -1) {
        *index_ptr = PARTY_TABLE_SIZE;
        return OBJ_HANDLE_NULL;
    }

    while (*index_ptr < PARTY_TABLE_SIZE) {
        if (dword_5FC32C[*index_ptr] == dword_5FC32C[index]) {
            return sub_4A2B60(*index_ptr);
        }
        (*index_ptr)++;
    }

    return OBJ_HANDLE_NULL;
}

// 0x4BA020
int party_id_from_obj(int64_t obj)
{
    int index;

    if (!tig_net_is_active()) {
        return -1;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return -1;
    }

    index = multiplayer_find_slot_from_obj(obj);
    if (index == -1) {
        return -1;
    }

    return dword_5FC32C[index];
}

// 0x4BA080
bool party_add(int64_t leader_obj, int64_t target_obj, int* id_ptr)
{
    int player1;
    int player2;
    int iter;
    int64_t party_member_obj;

    if (tig_net_is_active()) {
        player1 = multiplayer_find_slot_from_obj(leader_obj);
        if (player1 == -1) {
            sub_4BA320(leader_obj, target_obj, 1);
            return false;
        }

        player2 = multiplayer_find_slot_from_obj(target_obj);
        if (player2 == -1) {
            sub_4BA320(leader_obj, target_obj, 1);
            return false;
        }

        if (dword_5FC32C[player2] != -1) {
            sub_4BA320(leader_obj, target_obj, 3);
            return false;
        }

        if (dword_5FC32C[player1] == -1) {
            dword_5FC32C[player1] = sub_4BA260();
        }

        dword_5FC32C[player2] = dword_5FC32C[player1];

        if (id_ptr != NULL) {
            *id_ptr = dword_5FC32C[player2];
        }

        sub_4BA320(leader_obj, target_obj, 0);
        sub_460AB0(115, 0, NULL);
        party_update();

        party_member_obj = party_find_first(leader_obj, &iter);
        do {
            if (party_member_obj != target_obj) {
                quest_copy_accepted(party_member_obj, target_obj);
                quest_copy_accepted(target_obj, party_member_obj);
                rumor_copy_known(party_member_obj, target_obj);
                rumor_copy_known(target_obj, party_member_obj);
            }
            party_member_obj = party_find_next(&iter);
        } while (party_member_obj != OBJ_HANDLE_NULL);
    }

    return true;
}

// 0x4BA1E0
bool party_remove(int64_t obj)
{
    int index;

    if (tig_net_is_active()) {
        index = multiplayer_find_slot_from_obj(obj);
        if (index == -1) {
            sub_4BA320(obj, obj, 5);
            return false;
        }

        if (dword_5FC32C[index] == -1) {
            sub_4BA320(obj, obj, 6);
            return false;
        }

        dword_5FC32C[index] = -1;
        sub_4BA320(obj, obj, 6);
        sub_460AB0(115, 0, NULL);
        party_update();
    }

    return true;
}

// 0x4BA260
int sub_4BA260(void)
{
    int index;

    for (index = 0; index < 8; index++) {
        if (dword_5FC32C[index] == dword_5FC34C) {
            dword_5FC34C = (dword_5FC34C + 1) & 7;
            index = -1;
        }
    }

    return dword_5FC34C;
}

// 0x4BA290
bool party_update(void)
{
    if (tig_net_is_active()
        && tig_net_is_host()) {
        PacketPartyUpdate pkt;

        pkt.type = 71;
        memcpy(pkt.party, dword_5FC32C, sizeof(dword_5FC32C));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    return true;
}

// 0x4BA2E0
bool sub_4BA2E0(int* party, int cnt)
{
    if (!tig_net_is_active()
        || tig_net_is_host()) {
        return true;
    }

    memcpy(dword_5FC32C, party, sizeof(*party) * cnt);
    sub_460AB0(115, 0, NULL);

    return true;
}

// 0x4BA320
void sub_4BA320(int64_t a1, int64_t a2, int a3)
{
    TimeEvent timeevent;
    DateTime datetime;

    timeevent.type = TIMEEVENT_TYPE_MP_CTRL_UI;
    timeevent.params[0].integer_value = 1;
    timeevent.params[1].object_value = a1;
    timeevent.params[2].object_value = a2;
    timeevent.params[3].integer_value = a3;

    datetime.milliseconds = 500;
    datetime.days = 0;
    timeevent_add_delay(&timeevent, &datetime);
}
