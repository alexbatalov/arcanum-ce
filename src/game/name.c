#include "game/name.h"

#include <stdio.h>

#include "game/a_name.h"
#include "game/mes.h"

static void name_missing_art_init();
static bool name_missing_art_load();
static bool name_missing_art_save();
static int sub_41D390();
static void fix_missing_art(int num, int cnt, unsigned int* missing, int* anim_ptr, int* weapon_ptr);
static tig_art_id_t sub_41DFC0(int type, int* extra);
static tig_art_id_t sub_41E200(tig_art_id_t art_id);
static int sub_41E960(int a1);
static bool sub_41F190(TigMessage* msg);
static void sub_41F240();

// 0x5A1064
static const char* off_5A1064 = "art";

// 0x5A1068
static const char* off_5A1068 = "art\\missing.dat";

// 0x5A106C
static int dword_5A106C[] = {
    1,
    2,
    4,
    8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000,
    0x10000,
    0x20000,
    0x40000,
};

// 0x5A10B8
static TigArtAnim name_check_anims[] = {
    TIG_ART_ANIM_WALK,
    TIG_ART_ANIM_RUN,
    TIG_ART_ANIM_STUNNED,
    TIG_ART_ANIM_STAND,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_DECAPITATION,
    TIG_ART_ANIM_BLOWN_OUT_CHUNK,
    TIG_ART_ANIM_SEVERED_LEG,
    TIG_ART_ANIM_EXPLODE,
    TIG_ART_ANIM_STEALTH_WALK,
};

// 0x5A1104
static TigArtWeaponType name_check_weapons[] = {
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_NO_WEAPON,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_DAGGER,
    TIG_ART_WEAPON_TYPE_SWORD,
    TIG_ART_WEAPON_TYPE_AXE,
    TIG_ART_WEAPON_TYPE_MACE,
    TIG_ART_WEAPON_TYPE_PISTOL,
    TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD,
    TIG_ART_WEAPON_TYPE_BOW,
    TIG_ART_WEAPON_TYPE_RIFLE,
    TIG_ART_WEAPON_TYPE_STAFF,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_NO_WEAPON,
};

// 0x5A1150
static TigArtAnim dword_5A1150[] = {
    TIG_ART_ANIM_WALK,
    TIG_ART_ANIM_WALK,
    TIG_ART_ANIM_STAND,
    TIG_ART_ANIM_STAND,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_FALL_DOWN,
    TIG_ART_ANIM_FALL_DOWN,
    TIG_ART_ANIM_FALL_DOWN,
    TIG_ART_ANIM_FALL_DOWN,
    TIG_ART_ANIM_WALK,
};

// 0x5A119C
static char name_shielding_codes[2] = {
    'X',
    'S',
};

// 0x5A11A0
static char name_gender_codes[3] = {
    'F',
    'M',
    'X',
};

// 0x5A11A4
static const char* name_body_type_strs[TIG_ART_CRITTER_BODY_TYPE_COUNT] = {
    /*     TIG_ART_CRITTER_BODY_TYPE_HUMAN */ "HM",
    /*     TIG_ART_CRITTER_BODY_TYPE_DWARF */ "DF",
    /*  TIG_ART_CRITTER_BODY_TYPE_HALFLING */ "GH",
    /* TIG_ART_CRITTER_BODY_TYPE_HALF_OGRE */ "HG",
    /*       TIG_ART_CRITTER_BODY_TYPE_ELF */ "EF",
};

// 0x5A11B8
static const char* name_armor_type_strs[TIG_ART_ARMOR_TYPE_COUNT] = {
    /*     TIG_ART_ARMOR_TYPE_UNDERWEAR */ "UW",
    /*      TIG_ART_ARMOR_TYPE_VILLAGER */ "V1",
    /*       TIG_ART_ARMOR_TYPE_LEATHER */ "LA",
    /*         TIG_ART_ARMOR_TYPE_CHAIN */ "CM",
    /*         TIG_ART_ARMOR_TYPE_PLATE */ "PM",
    /*          TIG_ART_ARMOR_TYPE_ROBE */ "RB",
    /* TIG_ART_ARMOR_TYPE_PLATE_CLASSIC */ "PC",
    /*     TIG_ART_ARMOR_TYPE_BARBARIAN */ "BN",
    /*  TIG_ART_ARMOR_TYPE_CITY_DWELLER */ "CD",
};

// 0x5A11DC
static char name_weapon_type_codes[TIG_ART_WEAPON_TYPE_COUNT] = {
    /*        TIG_ART_WEAPON_TYPE_NO_WEAPON */ 'A',
    /*          TIG_ART_WEAPON_TYPE_UNARMED */ 'B',
    /*           TIG_ART_WEAPON_TYPE_DAGGER */ 'C',
    /*            TIG_ART_WEAPON_TYPE_SWORD */ 'D',
    /*              TIG_ART_WEAPON_TYPE_AXE */ 'E',
    /*             TIG_ART_WEAPON_TYPE_MACE */ 'F',
    /*           TIG_ART_WEAPON_TYPE_PISTOL */ 'G',
    /* TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD */ 'H',
    /*              TIG_ART_WEAPON_TYPE_BOW */ 'I',
    /*                TIG_ART_WEAPON_TYPE_9 */ 'J',
    /*            TIG_ART_WEAPON_TYPE_RIFLE */ 'K',
    /*               TIG_ART_WEAPON_TYPE_11 */ 'X',
    /*               TIG_ART_WEAPON_TYPE_12 */ 'Y',
    /*            TIG_ART_WEAPON_TYPE_STAFF */ 'N',
    /*               TIG_ART_WEAPON_TYPE_14 */ 'Z',
};

// 0x5A11EC
static char name_eye_candy_type_codes[TIG_ART_EYE_CANDY_TYPE_COUNT] = {
    /* TIG_ART_EYE_CANDY_TYPE_FOREGROUND_OVERLAY */ 'F',
    /* TIG_ART_EYE_CANDY_TYPE_BACKGROUND_OVERLAY */ 'B',
    /*           TIG_ART_EYE_CANDY_TYPE_UNDERLAY */ 'U',
};

// 0x5D55F0
static int num_critter_art;

// 0x5D55F4
static mes_file_handle_t name_monster_mes_file;

// 0x5D55F8
static mes_file_handle_t name_eye_candy_mes_file;

// 0x5D55FC
static mes_file_handle_t name_unique_npc_mes_file;

// 0x5D5600
static int num_unique_npc_art;

// 0x5D5604
static mes_file_handle_t name_container_mes_file;

// 0x5D5608
static unsigned int* missing_monster_art;

// 0x5D560C
static unsigned int* missing_critter_art;

// 0x5D5610
static unsigned int* missing_unique_npc_art;

// 0x5D5614
static mes_file_handle_t name_interface_mes_file;

// 0x5D5618
static int num_monster_art;

// 0x5D561C
static mes_file_handle_t name_scenery_mes_file;

// 0x5D5620
static bool name_initialized;

// 0x5D5624
static bool name_missing_art_initialized;

// 0x739E48
static tig_window_handle_t dword_739E48;

// 0x739E4C
static int dword_739E4C;

// 0x739E50
static int* dword_739E50;

// 0x739E54
static int dword_739E54;

// 0x739E58
static int dword_739E58;

// 0x41CA40
bool name_init(GameInitInfo* init_info)
{
    (void)init_info;

    if (name_initialized) {
        return true;
    }

    if (!mes_load("art\\scenery\\scenery.mes", &name_scenery_mes_file)) {
        return false;
    }

    if (!mes_load("art\\interface\\interface.mes", &name_interface_mes_file)) {
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!mes_load("art\\unique_npc\\unique_npc.mes", &name_unique_npc_mes_file)) {
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!mes_load("art\\monster\\monster.mes", &name_monster_mes_file)) {
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!mes_load("art\\eye_candy\\eye_candy.mes", &name_eye_candy_mes_file)) {
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!mes_load("art\\container\\container.mes", &name_container_mes_file)) {
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_light_init()) {
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_tile_init()) {
        a_name_light_exit();
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_roof_init()) {
        a_name_tile_exit();
        a_name_light_exit();
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_wall_init()) {
        a_name_roof_exit();
        a_name_tile_exit();
        a_name_light_exit();
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_portal_init()) {
        a_name_wall_exit();
        a_name_roof_exit();
        a_name_tile_exit();
        a_name_light_exit();
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_item_init()) {
        a_name_portal_exit();
        a_name_wall_exit();
        a_name_roof_exit();
        a_name_tile_exit();
        a_name_light_exit();
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    if (!a_name_facade_init()) {
        a_name_item_exit();
        a_name_portal_exit();
        a_name_wall_exit();
        a_name_roof_exit();
        a_name_tile_exit();
        a_name_light_exit();
        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);
        return false;
    }

    name_initialized = true;

    sub_4EBC40();
    name_missing_art_init();

    return true;
}

// 0x41CDA0
void name_exit()
{
    if (name_initialized) {
        FREE(missing_critter_art);
        FREE(missing_monster_art);
        FREE(missing_unique_npc_art);

        a_name_facade_exit();
        a_name_item_exit();
        a_name_portal_exit();
        a_name_wall_exit();
        a_name_roof_exit();
        a_name_tile_exit();
        a_name_light_exit();

        mes_unload(name_container_mes_file);
        mes_unload(name_eye_candy_mes_file);
        mes_unload(name_monster_mes_file);
        mes_unload(name_unique_npc_mes_file);
        mes_unload(name_interface_mes_file);
        mes_unload(name_scenery_mes_file);

        name_initialized = false;
        name_missing_art_initialized = false;
    }
}

// 0x41CE60
void name_missing_art_init()
{
    int idx;
    int num;
    tig_art_id_t aid;
    MesFileEntry mes_file_entry;

    num_critter_art = 10;
    num_monster_art = mes_num_entries(name_monster_mes_file);
    num_unique_npc_art = mes_num_entries(name_unique_npc_mes_file);

    missing_critter_art = (unsigned int*)CALLOC(num_critter_art, sizeof(*missing_critter_art));
    missing_monster_art = (unsigned int*)CALLOC(num_monster_art, sizeof(*missing_monster_art));
    missing_unique_npc_art = (unsigned int*)CALLOC(num_unique_npc_art, sizeof(*missing_unique_npc_art));

    if (!name_missing_art_load()) {
        for (idx = 0; idx < 19; idx++) {
            for (num = 0; num < num_critter_art; num++) {
                if (tig_art_critter_id_create(num & 1, num / 2, 0, 0, 0, 4, name_check_anims[idx], name_check_weapons[idx], 0, &aid) != TIG_OK) {
                    missing_critter_art[num] |= dword_5A106C[idx];
                    tig_debug_printf("Unable to create valid art id for critter %d (%s%c) using animation %d (%c%c)\n",
                        num,
                        name_body_type_strs[num / 2],
                        name_gender_codes[num & 1],
                        name_check_anims[idx],
                        name_weapon_type_codes[name_check_weapons[idx]],
                        'a' + name_check_anims[idx]);
                } else if (tig_art_exists(aid) != TIG_OK) {
                    missing_critter_art[num] |= dword_5A106C[idx];
                    tig_debug_printf("Missing art for critter %d (%s%c) using animation %d (%c%c)\n",
                        num,
                        name_body_type_strs[num / 2],
                        name_gender_codes[num & 1],
                        name_check_anims[idx],
                        name_weapon_type_codes[name_check_weapons[idx]],
                        'a' + name_check_anims[idx]);
                }
            }

            for (num = 0; num < num_monster_art; num++) {
                if (tig_art_monster_id_create(num, 0, 0, 0, 4, name_check_anims[idx], name_check_weapons[idx], 0, &aid) != TIG_OK) {
                    missing_monster_art[num] |= dword_5A106C[idx];

                    mes_file_entry.num = num;
                    mes_get_msg(name_monster_mes_file, &mes_file_entry);

                    tig_debug_printf("Unable to create valid art id for monster %d (%s) using animation %d (%c%c)\n",
                        num,
                        mes_file_entry.str,
                        name_check_anims[idx],
                        name_weapon_type_codes[name_check_weapons[idx]],
                        'a' + name_check_anims[idx]);
                } else if (tig_art_exists(aid) != TIG_OK) {
                    missing_monster_art[num] |= dword_5A106C[idx];

                    mes_file_entry.num = num;
                    mes_get_msg(name_monster_mes_file, &mes_file_entry);

                    tig_debug_printf("Missing art for monster %d (%s) using animation %d (%c%c)\n",
                        num,
                        mes_file_entry.str,
                        name_check_anims[idx],
                        name_weapon_type_codes[name_check_weapons[idx]],
                        'a' + name_check_anims[idx]);
                }
            }

            for (num = 0; num < num_unique_npc_art; num++) {
                if (tig_art_unique_npc_id_create(num, 0, 0, 4, name_check_anims[idx], name_check_weapons[idx], 0, &aid) != TIG_OK) {
                    missing_unique_npc_art[num] |= dword_5A106C[idx];

                    mes_file_entry.num = num;
                    mes_get_msg(name_unique_npc_mes_file, &mes_file_entry);

                    tig_debug_printf("Unable to create valid art id for unique npc %d (%s) using animation %d (%c%c)\n",
                        num,
                        mes_file_entry.str,
                        name_check_anims[idx],
                        name_weapon_type_codes[name_check_weapons[idx]],
                        'a' + name_check_anims[idx]);
                } else if (tig_art_exists(aid) != TIG_OK) {
                    missing_unique_npc_art[num] |= dword_5A106C[idx];

                    mes_file_entry.num = num;
                    mes_get_msg(name_monster_mes_file, &mes_file_entry);

                    tig_debug_printf("Missing art for unique npc %d (%s) using animation %d (%c%c)\n",
                        num,
                        mes_file_entry.str,
                        name_check_anims[idx],
                        name_weapon_type_codes[name_check_weapons[idx]],
                        'a' + name_check_anims[idx]);
                }
            }
        }

        name_missing_art_save();
    }

    name_missing_art_initialized = true;
}

// 0x41D1F0
bool name_missing_art_load()
{
    TigFile* stream;
    int expected_cnt;
    int actual_cnt;

    stream = tig_file_fopen(off_5A1068, "rb");
    if (stream == NULL) {
        return false;
    }

    expected_cnt = sub_41D390();

    if (tig_file_fread(&actual_cnt, sizeof(actual_cnt), 1, stream) != 1) {
        tig_file_fclose(stream);
        return false;
    }

    if (expected_cnt != actual_cnt) {
        tig_file_fclose(stream);
        return false;
    }

    if (tig_file_fread(missing_critter_art, sizeof(*missing_critter_art), num_critter_art, stream) < (size_t)num_critter_art) {
        tig_file_fclose(stream);
        return false;
    }

    if (tig_file_fread(missing_monster_art, sizeof(*missing_monster_art), num_monster_art, stream) < (size_t)num_monster_art) {
        tig_file_fclose(stream);
        return false;
    }

    if (tig_file_fread(missing_unique_npc_art, sizeof(*missing_unique_npc_art), num_unique_npc_art, stream) < (size_t)num_monster_art) {
        tig_file_fclose(stream);
        return false;
    }

    tig_file_fclose(stream);

    return true;
}

// 0x41D2C0
bool name_missing_art_save()
{
    int cnt;
    TigFile* stream;

    cnt = sub_41D390();
    tig_file_mkdir(off_5A1064);

    stream = tig_file_fopen(off_5A1068, "wb");
    if (stream == NULL) {
        return false;
    }

    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        tig_file_fclose(stream);
        return false;
    }

    if (tig_file_fwrite(missing_critter_art, sizeof(*missing_critter_art), num_critter_art, stream) != num_critter_art) {
        tig_file_fclose(stream);
        return false;
    }

    if (tig_file_fwrite(missing_monster_art, sizeof(*missing_monster_art), num_monster_art, stream) != num_monster_art) {
        tig_file_fclose(stream);
        return false;
    }

    if (tig_file_fwrite(missing_unique_npc_art, sizeof(*missing_unique_npc_art), num_unique_npc_art, stream) != num_unique_npc_art) {
        tig_file_fclose(stream);
        return false;
    }

    tig_file_fclose(stream);

    return true;
}

// 0x41D390
int sub_41D390()
{
    int cnt = 0;
    TigFileList dir_list;
    TigFileList file_list;
    char path[TIG_MAX_PATH];
    unsigned int index;

    tig_file_list_create(&dir_list, "art\\critter\\*.*");
    for (index = 0; index < dir_list.count; index++) {
        if (dir_list.entries[index].path[0] != '.') {
            sprintf(path, "art\\critter\\%s\\*.art", dir_list.entries[index].path);
            tig_file_list_create(&file_list, path);
            cnt += file_list.count;
            tig_file_list_destroy(&file_list);
        }
    }
    tig_file_list_destroy(&dir_list);

    tig_file_list_create(&dir_list, "art\\monster\\*.*");
    for (index = 0; index < dir_list.count; index++) {
        if (dir_list.entries[index].path[0] != '.') {
            sprintf(path, "art\\monster\\%s\\*.art", dir_list.entries[index].path);
            tig_file_list_create(&file_list, path);
            cnt += file_list.count;
            tig_file_list_destroy(&file_list);
        }
    }
    tig_file_list_destroy(&dir_list);

    tig_file_list_create(&dir_list, "art\\unique_npc\\*.*");
    for (index = 0; index < dir_list.count; index++) {
        if (dir_list.entries[index].path[0] != '.') {
            sprintf(path, "art\\unique_npc\\%s\\*.art", dir_list.entries[index].path);
            tig_file_list_create(&file_list, path);
            cnt += file_list.count;
            tig_file_list_destroy(&file_list);
        }
    }
    tig_file_list_destroy(&dir_list);

    return cnt;
}

// 0x41D510
tig_art_id_t name_normalize_aid(tig_art_id_t aid)
{
    switch (tig_art_type(aid)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
        aid = tig_art_id_palette_set(aid, 0);
        aid = tig_art_id_flags_set(aid, 0);
        break;
    case TIG_ART_TYPE_CRITTER: {
        int armor;
        int body_type;
        int gender;
        int shield;
        int anim;
        int weapon;

        armor = tig_art_critter_id_armor_get(aid);
        body_type = tig_art_critter_id_body_type_get(aid);
        gender = tig_art_critter_id_gender_get(aid);
        shield = tig_art_critter_id_shield_get(aid);
        anim = tig_art_id_anim_get(aid);
        weapon = tig_art_critter_id_weapon_get(aid);

        if (body_type == TIG_ART_CRITTER_BODY_TYPE_ELF
            && gender == TIG_ART_CRITTER_GENDER_FEMALE) {
            body_type = TIG_ART_CRITTER_BODY_TYPE_HUMAN;
        }

        if (armor == TIG_ART_ARMOR_TYPE_PLATE
            || armor == TIG_ART_ARMOR_TYPE_PLATE_CLASSIC) {
            gender = TIG_ART_CRITTER_GENDER_MALE;
            if (body_type == TIG_ART_CRITTER_BODY_TYPE_ELF) {
                body_type = TIG_ART_CRITTER_BODY_TYPE_HUMAN;
            } else if (body_type == TIG_ART_CRITTER_BODY_TYPE_HALFLING) {
                body_type = TIG_ART_CRITTER_BODY_TYPE_DWARF;
            }
        }

        fix_missing_art(gender + 2 * body_type, num_critter_art, missing_critter_art, &anim, &weapon);

        if (anim == 24) {
            armor = 0;
            shield = 0;
            weapon = 0;
            gender = 0;
        } else if (anim == 23) {
            weapon = 1;
            shield = 0;
        } else {
            if (weapon == 1 && (anim == 3 || anim == 5)) {
                weapon = 0;
            } else if (anim >= 6 && anim <= 19) {
                weapon = 0;
                shield = 0;
            } else if (anim == 1) {
                shield = 0;
            }
        }

        tig_art_critter_id_create(gender, body_type, armor, shield, 0, 0, anim, weapon, 0, &aid);
        break;
    }
    case TIG_ART_TYPE_PORTAL:
        aid = tig_art_id_frame_set(aid, 0);
        aid = tig_art_id_rotation_set(aid, 0);
        aid = tig_art_id_palette_set(aid, 0);
        aid = tig_art_id_flags_set(aid, 0);
        break;
    case TIG_ART_TYPE_SCENERY:
    case TIG_ART_TYPE_CONTAINER:
        aid = tig_art_id_frame_set(aid, 0);
        aid = tig_art_id_rotation_set(aid, 0);
        aid = tig_art_id_palette_set(aid, 0);
        break;
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
        aid = tig_art_id_frame_set(aid, 0);
        aid = tig_art_id_palette_set(aid, 0);
        break;
    case TIG_ART_TYPE_ITEM:
        aid = tig_art_id_damaged_set(aid, 0);
        aid = tig_art_id_palette_set(aid, 0);
        break;
    case TIG_ART_TYPE_LIGHT:
        aid = tig_art_id_frame_set(aid, 0);
        if (sub_504790(aid)) {
            int v1 = sub_504700(aid);
            aid = sub_504730(aid, v1 - v1 % 8);
        } else {
            aid = tig_art_id_rotation_set(aid, 0);
        }
        break;
    case TIG_ART_TYPE_ROOF:
        aid = tig_art_roof_id_piece_set(aid, 0);
        aid = tig_art_id_palette_set(aid, 0);
        aid = tig_art_roof_id_fill_set(aid, 0);
        aid = tig_art_roof_id_fade_set(aid, 0);
        break;
    case TIG_ART_TYPE_FACADE:
        tig_art_facade_id_create(tig_art_facade_id_num_get(aid), 0, tig_art_tile_id_type_get(aid), 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_MONSTER: {
        int armor;
        int specie;
        int shield;
        int anim;
        int weapon;

        armor = tig_art_critter_id_armor_get(aid);
        specie = tig_art_monster_id_specie_get(aid);
        shield = tig_art_critter_id_shield_get(aid);
        anim = tig_art_id_anim_get(aid);
        weapon = tig_art_critter_id_weapon_get(aid);
        fix_missing_art(specie, num_monster_art, missing_monster_art, &anim, &weapon);

        if (anim == 24) {
            armor = 0;
            shield = 0;
            weapon = 0;
        } else if (anim == 23) {
            weapon = 1;
            shield = 0;
        } else {
            if (weapon == 1 && (anim == 3 || anim == 5)) {
                weapon = 0;
            } else if (anim >= 6 && anim <= 19) {
                weapon = 0;
                shield = 0;
            } else if (anim == 1) {
                shield = 0;
            }
        }

        tig_art_monster_id_create(specie, armor, shield, 0, 0, anim, weapon, 0, &aid);
        break;
    }
    case TIG_ART_TYPE_UNIQUE_NPC: {
        int num;
        int shield;
        int anim;
        int weapon;

        num = tig_art_num_get(aid);
        shield = tig_art_critter_id_shield_get(aid);
        anim = tig_art_id_anim_get(aid);
        weapon = tig_art_critter_id_weapon_get(aid);
        fix_missing_art(num, num_unique_npc_art, missing_unique_npc_art, &anim, &weapon);
        if (anim == 24) {
            shield = 0;
            weapon = 0;
        } else if (anim == 23) {
            shield = 0;
            weapon = 1;
        } else {
            if (weapon == 1 && (anim == 3 || anim == 5)) {
                weapon = 0;
            } else if (anim >= 6 && anim <= 19) {
                weapon = 0;
                shield = 0;
            } else if (anim == 1) {
                shield = 0;
            }
        }
        tig_art_unique_npc_id_create(num, shield, 0, 0, anim, weapon, 0, &aid);
        break;
    }
    case TIG_ART_TYPE_EYE_CANDY:
        tig_art_eye_candy_id_create(tig_art_num_get(aid), 0, 0, 0, tig_art_eye_candy_id_type_get(aid), 0, 4, &aid);
        break;
    }

    return aid;
}

// 0x41DA40
void fix_missing_art(int num, int cnt, unsigned int* missing, int* anim_ptr, int* weapon_ptr)
{
    int idx;

    if (!name_missing_art_initialized) {
        return;
    }

    if (num < 0 || num >= cnt) {
        return;
    }

    for (idx = 0; idx < 19; idx++) {
        if ((dword_5A106C[idx] & missing[num]) != 0
            && *anim_ptr == name_check_anims[idx]
            && (*anim_ptr != 21
                || *weapon_ptr == name_check_weapons[idx])) {
            *anim_ptr = dword_5A1150[idx];
            if (*anim_ptr == 1) {
                if ((missing[num] & 0x1) != 0) {
                    *weapon_ptr = 0;
                }
            } else if (*anim_ptr == 0) {
                if (*weapon_ptr == 0 && (missing[num] & 0x8) != 0) {
                    *weapon_ptr = 1;
                }
            }
        }
    }
}

// 0x41DAE0
int name_resolve_path(tig_art_id_t aid, char* path)
{
    MesFileEntry mes_file_entry;
    int armor_type;
    int body_type;
    int anim;
    int shield;
    int weapon;
    char gender_code;
    const char* body_type_str;
    const char* armor_type_str;
    char shield_code;
    char weapon_code;

    if (!name_initialized) {
        return TIG_ERR_IO;
    }

    aid = name_normalize_aid(aid);

    switch (tig_art_type(aid)) {
    case TIG_ART_TYPE_TILE:
        if (!a_name_tile_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_WALL:
        if (!a_name_wall_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_CRITTER:
        armor_type = tig_art_critter_id_armor_get(aid);
        if (armor_type >= TIG_ART_ARMOR_TYPE_COUNT) {
            return TIG_ERR_GENERIC;
        }

        body_type = tig_art_critter_id_body_type_get(aid);
        if (armor_type == TIG_ART_ARMOR_TYPE_PLATE
            || armor_type == TIG_ART_ARMOR_TYPE_PLATE_CLASSIC) {
            gender_code = name_gender_codes[2];
        } else {
            gender_code = name_gender_codes[tig_art_critter_id_gender_get(aid)];
        }

        anim = tig_art_id_anim_get(aid);
        body_type_str = name_body_type_strs[body_type];
        if (anim == TIG_ART_ANIM_EXPLODE) {
            armor_type_str = "XX";
            gender_code = name_gender_codes[2];
        } else {
            armor_type_str = name_armor_type_strs[armor_type];
        }

        shield = tig_art_critter_id_shield_get(aid);
        shield_code = name_shielding_codes[shield];

        weapon = tig_art_critter_id_weapon_get(aid);
        if (weapon == TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD
            && shield == 1) {
            weapon_code = name_weapon_type_codes[TIG_ART_WEAPON_TYPE_SWORD];
        } else {
            weapon_code = name_weapon_type_codes[weapon];
        }

        sprintf(path,
            "art\\critter\\%s%c\\%s%c%s%c%c%c.art",
            body_type_str,
            gender_code,
            body_type_str,
            gender_code,
            armor_type_str,
            shield_code,
            weapon_code,
            'a' + anim);
        return TIG_OK;
    case TIG_ART_TYPE_PORTAL:
        if (!a_name_portal_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_SCENERY:
        mes_file_entry.num = 1000 * tig_art_scenery_id_type_get(aid) + tig_art_num_get(aid);
        if (!mes_search(name_scenery_mes_file, &mes_file_entry)) {
            return TIG_ERR_GENERIC;
        }
        sprintf(path, "art\\scenery\\%s", mes_file_entry.str);
        return TIG_OK;
    case TIG_ART_TYPE_INTERFACE:
        mes_file_entry.num = tig_art_num_get(aid);
        if (!mes_search(name_interface_mes_file, &mes_file_entry)) {
            return TIG_ERR_GENERIC;
        }
        sprintf(path, "art\\interface\\%s", mes_file_entry.str);
        return TIG_OK;
    case TIG_ART_TYPE_ITEM:
        if (!a_name_item_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_CONTAINER:
        mes_file_entry.num = 1000 * tig_art_container_id_type_get(aid) + tig_art_num_get(aid);
        if (!mes_search(name_container_mes_file, &mes_file_entry)) {
            return TIG_ERR_GENERIC;
        }
        sprintf(path, "art\\container\\%s", mes_file_entry.str);
        return TIG_OK;
    case TIG_ART_TYPE_LIGHT:
        if (!a_name_light_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_ROOF:
        if (!a_name_roof_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_FACADE:
        if (!a_name_facade_aid_to_fname(aid, path)) {
            return TIG_ERR_GENERIC;
        }
        return TIG_OK;
    case TIG_ART_TYPE_MONSTER:
        mes_file_entry.num = tig_art_monster_id_specie_get(aid);
        if (!mes_search(name_monster_mes_file, &mes_file_entry)) {
            return TIG_ERR_GENERIC;
        }

        anim = tig_art_id_anim_get(aid);
        armor_type = tig_art_critter_id_armor_get(aid);
        armor_type_str = anim != TIG_ART_ANIM_EXPLODE ? name_armor_type_strs[armor_type] : "XX";
        shield = tig_art_critter_id_shield_get(aid);
        shield_code = name_shielding_codes[shield];
        weapon = tig_art_critter_id_weapon_get(aid);
        if (weapon == TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD
            && shield == 1) {
            weapon_code = name_weapon_type_codes[TIG_ART_WEAPON_TYPE_SWORD];
        } else {
            weapon_code = name_weapon_type_codes[weapon];
        }

        sprintf(path,
            "art\\monster\\%s\\%s%s%c%c%c.art",
            mes_file_entry.str,
            mes_file_entry.str,
            armor_type_str,
            shield_code,
            weapon_code,
            'a' + anim);
        return TIG_OK;
    case TIG_ART_TYPE_UNIQUE_NPC:
        mes_file_entry.num = tig_art_num_get(aid);
        if (!mes_search(name_unique_npc_mes_file, &mes_file_entry)) {
            return TIG_ERR_GENERIC;
        }

        anim = tig_art_id_anim_get(aid);
        shield = tig_art_critter_id_shield_get(aid);
        shield_code = name_shielding_codes[shield];
        weapon = tig_art_critter_id_weapon_get(aid);
        if (weapon == TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD
            && shield == 1) {
            weapon_code = name_weapon_type_codes[TIG_ART_WEAPON_TYPE_SWORD];
        } else {
            weapon_code = name_weapon_type_codes[weapon];
        }

        sprintf(path,
            "art\\unique_npc\\%s\\%s%c%c%c.art",
            mes_file_entry.str,
            mes_file_entry.str,
            shield_code,
            weapon_code,
            'a' + anim);
        return TIG_OK;
    case TIG_ART_TYPE_EYE_CANDY:
        mes_file_entry.num = tig_art_num_get(aid);
        if (!mes_search(name_eye_candy_mes_file, &mes_file_entry)) {
            return TIG_ERR_GENERIC;
        }
        sprintf(path,
            "art\\eye_candy\\%s_%c.art",
            mes_file_entry.str,
            name_eye_candy_type_codes[tig_art_eye_candy_id_type_get(aid)]);
        return TIG_OK;
    default:
        return TIG_ERR_GENERIC;
    }
}

// 0x41DFC0
tig_art_id_t sub_41DFC0(int type, int* extra)
{
    tig_art_id_t aid = TIG_ART_ID_INVALID;

    switch (type) {
    case TIG_ART_TYPE_TILE:
        tig_art_tile_id_create(0, 0, 0, 0, 1, 1, 1, 0, &aid);
        break;
    case TIG_ART_TYPE_WALL:
        tig_art_wall_id_create(0, 0, 0, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_CRITTER:
        tig_art_critter_id_create(0, 0, 0, 0, 0, 3, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_PORTAL:
        tig_art_portal_id_create(0, 0, 0, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_SCENERY: {
        int v1;

        if (extra != NULL) {
            v1 = extra[0];
        } else {
            v1 = 0;
        }

        tig_art_scenery_id_create(0, v1, 0, 0, 0, &aid);
        break;
    }
    case TIG_ART_TYPE_INTERFACE:
        tig_art_interface_id_create(0, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_ITEM: {
        int v1;
        int armor_coverage;
        int destroyed;
        int disposition;
        int subtype;

        if (extra != NULL) {
            v1 = extra[0];
            armor_coverage = extra[1];
            destroyed = extra[2];
            disposition = extra[3];
            subtype = extra[4];
        } else {
            v1 = 0;
            armor_coverage = 0;
            destroyed = 0;
            disposition = 1;
            subtype = 0;
        }

        tig_art_item_id_create(0,
            disposition,
            0,
            destroyed,
            subtype,
            v1,
            armor_coverage,
            0,
            &aid);
        break;
    }
    case TIG_ART_TYPE_CONTAINER: {
        int v1;

        if (extra != NULL) {
            v1 = extra[0];
        } else {
            v1 = 0;
        }

        tig_art_container_id_create(0, v1, 0, 0, 0, &aid);
        break;
    }
    case TIG_ART_TYPE_LIGHT:
        tig_art_light_id_create(0, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_MONSTER:
        tig_art_monster_id_create(0, 0, 0, 0, 3, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_UNIQUE_NPC:
        tig_art_unique_npc_id_create(0, 0, 0, 3, 0, 0, 0, &aid);
        break;
    case TIG_ART_TYPE_EYE_CANDY:
        tig_art_eye_candy_id_create(0, 0, 0, 0, 0, 0, 4, &aid);
        break;
    }

    if (aid != TIG_ART_ID_INVALID) {
        if (tig_art_exists(aid) != TIG_OK || sub_502E00(aid) != TIG_OK) {
            aid = sub_41E200(aid);
        }
    }

    return aid;
}

// 0x41E200
tig_art_id_t sub_41E200(tig_art_id_t art_id)
{
    if (art_id == TIG_ART_ID_INVALID) {
        return TIG_ART_ID_INVALID;
    }

    for (;;) {
        switch (tig_art_type(art_id)) {
        case TIG_ART_TYPE_TILE: {
            int num = tig_art_tile_id_num1_get(art_id) + 1;
            int type = tig_art_tile_id_type_get(art_id);
            int flippable = tig_art_tile_id_flippable_get(art_id);

            if (tig_art_tile_id_create(num, num, 0, 0, type, flippable, flippable, 0, &art_id) == TIG_OK) {
                break;
            }

            // TODO: Check.
            if (type == 0) {
                if (flippable) {
                    flippable = 0;
                    num = 0;
                }
            } else {
                if (!flippable) {
                    flippable = 1;
                    type = 0;
                    num = 0;
                } else {
                    flippable = 0;
                    num = 0;
                }
            }

            if (tig_art_tile_id_create(num, num, 0, 0, type, flippable, flippable, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_WALL: {
            int num = tig_art_wall_id_num_get(art_id);
            int p_piece = tig_art_wall_id_p_piece_get(art_id);
            int variation = tig_art_wall_id_variation_get(art_id);
            int palette = tig_art_id_palette_get(art_id);
            int damaged = tig_art_id_damaged_get(art_id);

            // TODO: Probably wrong, check.
            if (damaged == 0) {
                if (tig_art_wall_id_create(num, p_piece, variation, 0, palette, 1024, &art_id) == TIG_OK) {
                    break;
                }
            } else if (damaged == 1024) {
                if (tig_art_wall_id_create(num, p_piece, variation, 0, palette, 128, &art_id) == TIG_OK) {
                    break;
                }
            }

            if (tig_art_wall_id_create(num, p_piece, variation, 0, palette + 1, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_wall_id_create(num, p_piece, variation + 1, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_wall_id_create(num, p_piece + 1, 0, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_wall_id_create(num + 1, 0, 0, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_CRITTER: {
            int palette = tig_art_id_palette_get(art_id);
            int gender = tig_art_critter_id_gender_get(art_id);
            int body_type = tig_art_critter_id_body_type_get(art_id);
            int armor = tig_art_critter_id_armor_get(art_id);

            if (tig_art_critter_id_create(gender, body_type, armor, 0, 0, 3, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_critter_id_create(gender + 1, body_type, armor, 0, 0, 3, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_critter_id_create(0, body_type + 1, armor, 0, 0, 3, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_critter_id_create(0, 0, armor + 1, 0, 0, 3, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_PORTAL: {
            int palette = tig_art_id_palette_get(art_id);
            int type = tig_art_portal_id_type_get(art_id);
            int num = tig_art_num_get(art_id);
            int damaged = tig_art_id_damaged_get(art_id);

            if (damaged == 0) {
                if (tig_art_portal_id_create(num, type, 512, 0, 0, palette, &art_id) == TIG_OK) {
                    break;
                }
            }

            if (tig_art_portal_id_create(num, type, 0, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_portal_id_create(num, type, 0, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_portal_id_create(num, type + 1, 0, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_portal_id_create(num + 1, 0, 0, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_SCENERY: {
            int type = tig_art_scenery_id_type_get(art_id);
            int palette = tig_art_id_palette_get(art_id);
            int num = tig_art_num_get(art_id);

            if (tig_art_scenery_id_create(num, type, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_scenery_id_create(num + 1, type, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_INTERFACE: {
            int palette = tig_art_id_palette_get(art_id);
            int num = tig_art_num_get(art_id);

            if (tig_art_interface_id_create(num, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_interface_id_create(num + 1, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_ITEM: {
            int palette = tig_art_id_palette_get(art_id);
            int num = tig_art_num_get(art_id);
            int subtype = tig_art_item_id_subtype_get(art_id);
            int type = tig_art_item_id_type_get(art_id);
            int destroyed = tig_art_item_id_destroyed_get(art_id);
            int disposition = tig_art_item_id_disposition_get(art_id);
            int armor_coverage = tig_art_item_id_armor_coverage_get(art_id);

            if (tig_art_item_id_create(num, disposition, 0, destroyed, subtype, type, armor_coverage, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_item_id_create(num + 1, disposition, 0, destroyed, subtype, type, armor_coverage, 0, &art_id) == TIG_OK) {
                break;
            }

            if (type == TIG_ART_ITEM_TYPE_ARMOR
                && armor_coverage == TIG_ART_ARMOR_COVERAGE_TORSO) {
                if (tig_art_item_id_create(0, disposition, 0, destroyed, subtype + 1, type, armor_coverage, 0, &art_id) == TIG_OK) {
                    break;
                }
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_CONTAINER: {
            int type = tig_art_container_id_type_get(art_id);
            int palette = tig_art_id_palette_get(art_id);
            int num = tig_art_num_get(art_id);

            if (tig_art_container_id_create(num, type, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_container_id_create(num + 1, type, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_LIGHT: {
            int num = tig_art_num_get(art_id);

            if (tig_art_light_id_create(num + 1, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_MONSTER: {
            int palette = tig_art_id_palette_get(art_id);
            int num = tig_art_monster_id_specie_get(art_id);

            if (tig_art_monster_id_create(num, 0, 0, 0, 3, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_monster_id_create(num + 1, 0, 0, 0, 3, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_UNIQUE_NPC: {
            int palette = tig_art_id_palette_get(art_id);
            int num = tig_art_num_get(art_id);

            if (tig_art_unique_npc_id_create(num, 0, 0, 3, 0, 0, palette + 1, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_unique_npc_id_create(num + 1, 0, 0, 3, 0, 0, 0, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        case TIG_ART_TYPE_EYE_CANDY: {
            int num = tig_art_num_get(art_id);
            int type = tig_art_eye_candy_id_type_get(art_id);
            int palette = tig_art_id_palette_get(art_id);

            if (tig_art_eye_candy_id_create(num, 0, 0, 0, type, palette + 1, 4, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_eye_candy_id_create(num, 0, 0, 0, type + 1, 0, 4, &art_id) == TIG_OK) {
                break;
            }

            if (tig_art_eye_candy_id_create(num + 1, 0, 0, 0, 0, 0, 4, &art_id) == TIG_OK) {
                break;
            }

            return TIG_ART_ID_INVALID;
        }
        default:
            break;
        }

        if (art_id == TIG_ART_ID_INVALID) {
            return TIG_ART_ID_INVALID;
        }

        if (tig_art_exists(art_id) == TIG_OK
            && sub_502E00(art_id) == TIG_OK) {
            return art_id;
        }

        if (art_id == TIG_ART_ID_INVALID) {
            return TIG_ART_ID_INVALID;
        }
    }
}

// 0x41E960
int sub_41E960(int a1)
{
    // TODO: Incomplete.
}

// 0x41F090
bool sub_41F090(int a1, int* a2, int* a3)
{
    TigWindowData window_data;

    dword_739E58 = a1;
    dword_739E50 = a2;
    dword_739E54 = -1;

    window_data.rect.x = 40;
    window_data.rect.y = 40;
    window_data.rect.width = 100;
    window_data.rect.height = 100;
    window_data.flags = TIG_WINDOW_ALWAYS_ON_TOP | TIG_WINDOW_MODAL | TIG_WINDOW_MESSAGE_FILTER;
    window_data.message_filter = sub_41F190;
    window_data.background_color = tig_color_make(30, 30, 30);

    if (tig_window_create(&window_data, &dword_739E48) != TIG_OK) {
        return false;
    }

    sub_41F240();

    dword_739E4C = 0;
    while (dword_739E4C == 0) {
        tig_ping();
        tig_window_display();
    }

    tig_window_destroy(dword_739E48);

    if (dword_739E4C != 1) {
        return false;
    }

    *a3 = dword_739E54;

    return true;
}

// 0x41F190
bool sub_41F190(TigMessage* msg)
{
    int v1;

    if (msg->type == TIG_MESSAGE_KEYBOARD
        && msg->data.keyboard.pressed) {
        switch (msg->data.keyboard.key) {
        case SDLK_A:
            if (dword_739E54 != -1) {
                dword_739E54 = sub_41E960(dword_739E54);
                sub_41F240();
            }
            break;
        case SDLK_Z:
            if (dword_739E54 == -1) {
                dword_739E54 = sub_41DFC0(dword_739E58, dword_739E50);
                sub_41F240();
            } else {
                v1 = sub_41E200(dword_739E54);
                if (v1 != -1) {
                    dword_739E54 = v1;
                }
                sub_41F240();
            }
            break;
        case SDLK_ESCAPE:
            dword_739E4C = -1;
            break;
        case SDLK_RETURN:
            dword_739E4C = 1;
            break;
        }
    }

    return true;
}

// 0x41F240
void sub_41F240()
{
    TigRect dst_rect;
    TigRect src_rect;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_spec;

    dst_rect.x = 10;
    dst_rect.y = 10;
    dst_rect.width = 80;
    dst_rect.height = 80;

    tig_window_fill(dword_739E48, &dst_rect, tig_color_make(0, 0, 60));

    if (dword_739E54 != TIG_ART_ID_INVALID) {
        if (tig_art_frame_data(dword_739E54, &art_frame_data) == TIG_OK) {
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.height;

            if (art_frame_data.width > 80 || art_frame_data.height > 80) {
                if (80.0 / (double)art_frame_data.height < 80.0 / (double)art_frame_data.width) {
                    art_frame_data.width = (int)((double)art_frame_data.width * (80.0 / (double)art_frame_data.height));
                } else {
                    art_frame_data.height = (int)((double)art_frame_data.height * (80.0 / (double)art_frame_data.width));
                }
            } else {
                dst_rect.width = art_frame_data.width;
                dst_rect.height = art_frame_data.height;
            }

            dst_rect.x += (80 - dst_rect.width) / 2;
            dst_rect.y += (80 - dst_rect.height) / 2;

            art_blit_spec.flags = TIG_ART_BLT_PALETTE_ORIGINAL;
            art_blit_spec.art_id = dword_739E54;
            art_blit_spec.src_rect = &src_rect;
            art_blit_spec.dst_rect = &dst_rect;
            tig_window_blit_art(dword_739E48, &art_blit_spec);
        }
    }
}
