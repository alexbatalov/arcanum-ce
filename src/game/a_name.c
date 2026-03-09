#include "game/a_name.h"

#include <stdio.h>

#include "game/mes.h"
#include "game/obj_private.h"

#define TF_BLOCK 0x01
#define TF_SINKABLE 0x02
#define TF_FLYABLE 0x04
#define TF_SLIPPERY 0x08
#define TF_NATURAL 0x10
#define TF_SOUNDPROOF 0x20

typedef uint32_t WallStructureFlags;

#define WS_NOWINDOWS 0x01
#define WS_NODOORS 0x02
#define WS_FENCE 0x04

typedef struct WallStructure {
    /* 0000 */ WallStructureFlags flags;
    /* 0004 */ int interior;
    /* 0008 */ int interior_param;
    /* 000C */ int exterior;
    /* 0010 */ int exterior_param;
    /* 0014 */ tig_art_id_t tile_art_id;
    /* 0018 */ tig_art_id_t roof_art_id;
    /* 001C */ int wall_proto;
} WallStructure;

static bool build_tile_file_name(const char* name1, const char* name2, int a3, int a4, char* fname);
static bool sub_4EB0C0(int num, int type, int flippable, char** name_ptr);
static bool a_name_tile_fname_to_aid(const char* name, tig_art_id_t* art_id_ptr);
static bool count_tile_names(void);
static bool load_tile_names(void);
static bool load_tile_edges(void);
static bool sub_4EB770(char* name, int* a2, int* a3);
static bool sub_4EB7D0(const char* name, int* index_ptr);
static bool sub_4EB860(int a1, int a2, bool* a3, int* a4);
static bool sub_4EB8D0(int* a1, int a2, int a3, bool* a4);
static tig_art_id_t sub_4EB970(tig_art_id_t a, tig_art_id_t b);
static uint8_t a_name_tile_id_flags(tig_art_id_t aid);
static int8_t sub_4EBE90(int a1, int a2, int a3, int a4, int a5, int a6);
static bool sub_4EC020(void);
static bool sub_4EC0C0(void);
static int sub_4EC160(void);
static bool build_facade_file_name(int num, char* fname);
static bool sub_4EC4B0(void);
static char* sub_4EC8F0(tig_art_id_t aid);
static int sub_4EC940(const char* fname);
static void init_wall_names(void);
static void sub_4ECB80(mes_file_handle_t wallproto_mes_file, char* str, int index);
static int sub_4ECC00(int index);
static void init_wall_structures(void);
static void parse_wall_structure(char* str, int index);
static bool build_wall_file_name(const char* name, int piece, int damage, int variation, char* fname);
static int sub_4ED030(const char* str);
static void sub_4ED180(int index, WallStructure* wallstructure);
static bool build_roof_file_name(int index, char* buffer);
static bool load_roof_data(void);

// 0x603AE0
static char** outdoor_non_flippable_tile_names;

// 0x603AE4
static bool dword_603AE4;

// 0x603AE8
static uint8_t* outdoor_flippable_tile_flags;

// 0x603AEC
static int* indoor_non_flippable_tile_sounds;

// 0x603AF0
static int* outdoor_non_flippable_tile_sounds;

// 0x603AF4
static uint8_t* indoor_non_flippable_tile_flags;

// 0x603AF8
static int* dword_603AF8;

// 0x603AFC
static uint8_t* indoor_flippable_tile_flags;

// 0x603B00
static mes_file_handle_t tilename_mes_file;

// 0x603B04
static char** indoor_flippable_tile_names;

// 0x603B08
static int num_indoor_flippable_names;

// 0x603B0C
static int* outdoor_flippable_tile_sounds;

// 0x603B10
static char** indoor_non_flippable_tile_names;

// 0x603B14
static char** outdoor_flippable_tile_names;

// 0x603B18
static int num_outdoor_non_flippable_names;

// 0x603B1C
static int num_outdoor_flippable_names;

// 0x603B20
static int* indoor_flippable_tile_sounds;

// 0x603B24
static uint8_t* outdoor_non_flippable_tile_flags;

// 0x603B28
static int num_indoor_non_flippable_names;

// 0x603B2C
static mes_file_handle_t item_paper_mes_file;

// 0x603B30
static mes_file_handle_t item_inven_mes_file;

// 0x603B34
static mes_file_handle_t item_ground_mes_file;

// 0x603B38
static mes_file_handle_t item_schematic_mes_file;

// 0x603B3C
static mes_file_handle_t facadename_mes_file;

// 0x603B40
static char** facade_names;

// 0x603B44
static int num_facade_names;

// 0x603B48
static bool facade_initialized;

// 0x603B4C
static mes_file_handle_t portal_mes_file;

// 0x603B50
static int num_wall_structures;

// 0x603B54
static int num_wall_file_names;

// 0x603B58
static WallStructure* wall_structures;

// 0x603B5C
static mes_file_handle_t wall_structure_mes_file;

// 0x603B60
static int* wall_proto_file_names;

// 0x603B64
static mes_file_handle_t wallname_mes_file;

// 0x603B68
static char** wall_file_names;

// 0x603B6C
static bool light_initialized;

// 0x603B70
static mes_file_handle_t light_mes_file;

// 0x603B74
static char** roof_file_names;

// 0x603B78
static mes_file_handle_t roofname_mes_file;

// 0x603B7C
static int num_roof_file_names;

// 0x603B80
static int roof_initialized;

// 0x687660
static uint8_t* dword_687660[7];

// 0x687680
static size_t dword_687680[7];

// 0x4EAC80
bool a_name_tile_init(void)
{
    if (!mes_load("art\\tile\\tilename.mes", &tilename_mes_file)) {
        return false;
    }

    if (!count_tile_names()) {
        mes_unload(tilename_mes_file);
        return false;
    }

    if (!load_tile_names()) {
        mes_unload(tilename_mes_file);
        return false;
    }

    if (!load_tile_edges()) {
        mes_unload(tilename_mes_file);
        return false;
    }

    dword_687680[0] = 16 * (num_outdoor_flippable_names + 4);
    dword_687680[1] = 16 * (num_outdoor_non_flippable_names + 4);
    dword_687680[2] = 16 * (num_indoor_flippable_names + 4);
    dword_687680[3] = 16 * (num_indoor_non_flippable_names + 4);
    dword_687680[4] = 16 * (num_outdoor_flippable_names * num_outdoor_flippable_names + 4);
    dword_687680[5] = 16 * (num_outdoor_non_flippable_names * num_outdoor_non_flippable_names + 4);
    dword_687680[6] = 16 * (num_outdoor_flippable_names * num_outdoor_non_flippable_names + 4);

    return true;
}

// 0x4EAD50
void a_name_tile_exit(void)
{
    mes_unload(tilename_mes_file);

    FREE(dword_603AF8);
    FREE(outdoor_flippable_tile_names);
    FREE(outdoor_non_flippable_tile_names);
    FREE(indoor_flippable_tile_names);
    FREE(indoor_non_flippable_tile_names);
    FREE(outdoor_flippable_tile_flags);
    FREE(outdoor_non_flippable_tile_flags);
    FREE(indoor_flippable_tile_flags);
    FREE(indoor_non_flippable_tile_flags);
    FREE(outdoor_flippable_tile_sounds);
    FREE(outdoor_non_flippable_tile_sounds);
    FREE(indoor_flippable_tile_sounds);
    FREE(indoor_non_flippable_tile_sounds);

    if (dword_603AE4) {
        int index;

        for (index = 0; index < 7; index++) {
            FREE(dword_687660[index]);
        }

        dword_603AE4 = false;
    }
}

// 0x4EAE90
bool a_name_tile_aid_to_fname(tig_art_id_t aid, char* fname)
{
    int num1;
    int num2;
    int v1;
    int v2;
    int type;
    int flippable1;
    int flippable2;
    char* name1;
    char* name2;

    if (tig_art_type(aid) != TIG_ART_TYPE_TILE) {
        tig_debug_println("Asking for tile name with non tile art ID.");
        fname[0] = '\0';
        return false;
    }

    num1 = tig_art_tile_id_num1_get(aid);
    num2 = tig_art_tile_id_num2_get(aid);
    v1 = sub_503700(aid);
    v2 = sub_5037B0(aid);
    type = tig_art_tile_id_type_get(aid);
    flippable1 = tig_art_tile_id_flippable1_get(aid);
    flippable2 = tig_art_tile_id_flippable2_get(aid);

    if (!sub_4EB0C0(num1, type, flippable1, &name1)
        || !sub_4EB0C0(num2, type, flippable2, &name2)) {
        return false;
    }

    return build_tile_file_name(name1, name2, v1, v2, fname);
}

// 0x4EAF70
bool build_tile_file_name(const char* name1, const char* name2, int a3, int a4, char* fname)
{
    // 0x5BB4E4
    static const char off_5BB4E4[] = "06b489237ea5dc10";

    int v1;
    int v2;

    if (a4 >= 8) {
        a4 -= 8;
    }

    if (a3 == 15 || SDL_strcasecmp(name1, name2) == 0) {
        sprintf(fname,
            "art\\tile\\%sbse%c%c.art",
            name1,
            off_5BB4E4[a3],
            a4 + 'a');
        return true;
    }

    if (a3 == 0) {
        sprintf(fname,
            "art\\tile\\%sbse%c%c.art",
            name2,
            off_5BB4E4[0],
            a4 + 'a');
        return true;
    }

    if (!sub_4EB7D0(name1, &v1)) {
        sprintf(fname,
            "art\\tile\\%sbse%c%c.art",
            name1,
            off_5BB4E4[a3],
            a4 + 'a');
        return true;
    }

    if (!sub_4EB7D0(name2, &v2)) {
        sprintf(fname,
            "art\\tile\\%sbse%c%c.art",
            name2,
            off_5BB4E4[15 - a3],
            a4 + 'a');
        return true;
    }

    if (v1 < v2) {
        sprintf(fname,
            "art\\tile\\%s%s%c%c.art",
            name1,
            name2,
            off_5BB4E4[a3],
            a4 + 'a');
    } else {
        sprintf(fname,
            "art\\tile\\%s%s%c%c.art",
            name2,
            name1,
            off_5BB4E4[15 - a3],
            a4 + 'a');
    }
    return true;
}

// 0x4EB0C0
bool sub_4EB0C0(int num, int type, int flippable, char** name_ptr)
{
    if (flippable) {
        if (type) {
            if (num < num_outdoor_flippable_names) {
                *name_ptr = outdoor_flippable_tile_names[num];
                return true;
            }
        } else {
            if (num < num_indoor_flippable_names) {
                *name_ptr = indoor_flippable_tile_names[num];
                return true;
            }
        }
    } else {
        if (type) {
            if (num < num_outdoor_non_flippable_names) {
                *name_ptr = outdoor_non_flippable_tile_names[num];
                return true;
            }
        } else {
            if (num < num_indoor_non_flippable_names) {
                *name_ptr = indoor_non_flippable_tile_names[num];
                return true;
            }
        }
    }
    return false;
}

// 0x4EB160
bool a_name_tile_fname_to_aid(const char* name, tig_art_id_t* art_id_ptr)
{
    int index;

    for (index = 0; index < num_outdoor_flippable_names; index++) {
        if (SDL_strcasecmp(outdoor_flippable_tile_names[index], name) == 0) {
            tig_art_tile_id_create(index, index, 15, 0, 1, 1, 1, 0, art_id_ptr);
            return true;
        }
    }

    for (index = 0; index < num_outdoor_non_flippable_names; index++) {
        if (SDL_strcasecmp(outdoor_non_flippable_tile_names[index], name) == 0) {
            tig_art_tile_id_create(index, index, 15, 0, 1, 0, 0, 0, art_id_ptr);
            return true;
        }
    }

    for (index = 0; index < num_indoor_flippable_names; index++) {
        if (SDL_strcasecmp(indoor_flippable_tile_names[index], name) == 0) {
            tig_art_tile_id_create(index, index, 15, 0, 0, 1, 1, 0, art_id_ptr);
            return true;
        }
    }

    for (index = 0; index < num_indoor_non_flippable_names; index++) {
        if (SDL_strcasecmp(indoor_non_flippable_tile_names[index], name) == 0) {
            tig_art_tile_id_create(index, index, 15, 0, 0, 0, 0, 0, art_id_ptr);
            return true;
        }
    }

    return false;
}

// 0x4EB270
bool count_tile_names(void)
{
    num_outdoor_flippable_names = mes_entries_count_in_range(tilename_mes_file, 0, 99);
    num_outdoor_non_flippable_names = mes_entries_count_in_range(tilename_mes_file, 100, 199);
    num_indoor_flippable_names = mes_entries_count_in_range(tilename_mes_file, 200, 299);
    num_indoor_non_flippable_names = mes_entries_count_in_range(tilename_mes_file, 300, 399);
    return true;
}

// 0x4EB2E0
bool load_tile_names(void)
{
    MesFileEntry mes_file_entry;
    uint8_t flags;
    char* pch;
    int sound_type;
    int index;

    outdoor_flippable_tile_names = (char**)MALLOC(sizeof(char*) * num_outdoor_flippable_names);
    outdoor_non_flippable_tile_names = (char**)MALLOC(sizeof(char*) * num_outdoor_non_flippable_names);
    indoor_flippable_tile_names = (char**)MALLOC(sizeof(char*) * num_indoor_flippable_names);
    indoor_non_flippable_tile_names = (char**)MALLOC(sizeof(char*) * num_indoor_non_flippable_names);

    outdoor_flippable_tile_flags = (uint8_t*)MALLOC(sizeof(uint8_t*) * num_outdoor_flippable_names);
    outdoor_non_flippable_tile_flags = (uint8_t*)MALLOC(sizeof(uint8_t*) * num_outdoor_non_flippable_names);
    indoor_flippable_tile_flags = (uint8_t*)MALLOC(sizeof(uint8_t*) * num_indoor_flippable_names);
    indoor_non_flippable_tile_flags = (uint8_t*)MALLOC(sizeof(uint8_t*) * num_indoor_non_flippable_names);

    outdoor_flippable_tile_sounds = (int*)MALLOC(sizeof(int*) * num_outdoor_flippable_names);
    outdoor_non_flippable_tile_sounds = (int*)MALLOC(sizeof(char*) * num_outdoor_non_flippable_names);
    indoor_flippable_tile_sounds = (int*)MALLOC(sizeof(char*) * num_indoor_flippable_names);
    indoor_non_flippable_tile_sounds = (int*)MALLOC(sizeof(char*) * num_indoor_non_flippable_names);

    mes_file_entry.num = 0;
    if (!mes_search(tilename_mes_file, &mes_file_entry)) {
        return false;
    }

    index = 0;
    do {
        flags = 0;

        pch = strchr(mes_file_entry.str, '/');
        if (pch != NULL) {
            *pch++ = '\0';
            while (*pch != '\0' && *pch != ' ') {
                switch (*pch) {
                case 's':
                    flags |= TF_SINKABLE;
                    break;
                case 'b':
                    flags |= TF_BLOCK;
                    break;
                case 'f':
                    flags |= TF_BLOCK | TF_FLYABLE;
                    break;
                case 'i':
                    flags |= TF_SLIPPERY;
                    break;
                case 'n':
                    flags |= TF_NATURAL;
                    break;
                case 'p':
                    flags |= TF_SOUNDPROOF;
                    break;
                }
                pch++;
            }

            sound_type = atoi(pch);
        } else {
            if (strlen(mes_file_entry.str) < 3) {
                return false;
            }

            mes_file_entry.str[3] = '\0';

            sound_type = atoi(mes_file_entry.str + 3);
        }

        if (mes_file_entry.num < 100) {
            outdoor_flippable_tile_flags[index] = flags;
            outdoor_flippable_tile_names[index] = mes_file_entry.str;
            outdoor_flippable_tile_sounds[index] = sound_type;
            index++;
        } else if (mes_file_entry.num < 200) {
            if (mes_file_entry.num == 100) {
                index = 0;
            }

            outdoor_non_flippable_tile_flags[index] = flags;
            outdoor_non_flippable_tile_names[index] = mes_file_entry.str;
            outdoor_non_flippable_tile_sounds[index] = sound_type;
            index++;
        } else if (mes_file_entry.num < 300) {
            if (mes_file_entry.num == 200) {
                index = 0;
            }

            indoor_flippable_tile_flags[index] = flags;
            indoor_flippable_tile_names[index] = mes_file_entry.str;
            indoor_flippable_tile_sounds[index] = sound_type;
            index++;
        } else if (mes_file_entry.num < 400) {
            if (mes_file_entry.num == 300) {
                index = 0;
            }

            indoor_non_flippable_tile_flags[index] = flags;
            indoor_non_flippable_tile_names[index] = mes_file_entry.str;
            indoor_non_flippable_tile_sounds[index] = sound_type;
            index++;
        }
    } while (mes_find_next(tilename_mes_file, &mes_file_entry) && mes_file_entry.num < 400);

    return true;
}

// 0x4EB5E0
bool load_tile_edges(void)
{
    MesFileEntry mes_file_entry;
    bool* v1;
    int cnt;
    int v2;
    int v3;
    int v4;

    mes_file_entry.num = 400;
    if (!mes_search(tilename_mes_file, &mes_file_entry)) {
        return false;
    }

    cnt = num_outdoor_non_flippable_names + num_outdoor_flippable_names;
    v1 = (bool*)MALLOC(sizeof(*v1) * cnt * cnt);

    for (v2 = 0; v2 < cnt; v2++) {
        for (v3 = 0; v3 < cnt; v3++) {
            v1[cnt * v2 + v3] = false;
        }
    }

    do {
        if (!sub_4EB770(mes_file_entry.str, &v2, &v3)) {
            FREE(v1);
            return false;
        }

        v1[cnt * v3 + v2] = true;
        v1[cnt * v2 + v3] = true;
    } while (mes_find_next(tilename_mes_file, &mes_file_entry));

    dword_603AF8 = MALLOC(sizeof(*dword_603AF8) * cnt * cnt);

    for (v2 = 0; v2 < cnt; v2++) {
        for (v3 = 0; v3 < cnt; v3++) {
            if (!sub_4EB860(v2, v3, v1, &v4)) {
                FREE(v1);
                return false;
            }
        }
    }

    FREE(v1);

    return true;
}

// 0x4EB770
bool sub_4EB770(char* name, int* a2, int* a3)
{
    char ch;
    bool v1;

    if (strlen(name) != 6) {
        return false;
    }

    ch = name[3];
    name[3] = '\0';
    v1 = sub_4EB7D0(name, a2);
    name[3] = ch;

    if (!v1) {
        return false;
    }

    return sub_4EB7D0(name + 3, a3);
}

// 0x4EB7D0
bool sub_4EB7D0(const char* name, int* index_ptr)
{
    int index = 0;

    for (index = 0; index < num_outdoor_flippable_names; index++) {
        if (SDL_strcasecmp(outdoor_flippable_tile_names[index], name) == 0) {
            *index_ptr = index;
            return true;
        }
    }

    for (index = 0; index < num_outdoor_non_flippable_names; index++) {
        if (SDL_strcasecmp(outdoor_non_flippable_tile_names[index], name) == 0) {
            *index_ptr = num_outdoor_flippable_names + index;
            return true;
        }
    }

    return false;
}

// 0x4EB860
bool sub_4EB860(int a1, int a2, bool* a3, int* a4)
{
    bool rc;
    int* v1;

    if (a1 == a2) {
        *a4 = a1;
        return true;
    }

    v1 = (int*)MALLOC(sizeof(int) * (num_outdoor_non_flippable_names + num_outdoor_flippable_names));
    v1[0] = a1;
    rc = sub_4EB8D0(v1, 0, a2, a3);
    *a4 = v1[1];
    FREE(v1);

    return rc;
}

// 0x4EB8D0
bool sub_4EB8D0(int* a1, int a2, int a3, bool* a4)
{
    int cnt;
    int index;
    int offset;
    int v1;

    cnt = num_outdoor_non_flippable_names + num_outdoor_flippable_names;

    offset = 0;
    for (index = 0; index < cnt; index++) {
        if (a4[offset + a1[a2]]) {
            for (v1 = 0; v1 <= a2; v1++) {
                if (index == a1[v1]) {
                    break;
                }
            }

            if (v1 > a2) {
                a1[a2 + 1] = index;
                if (index == a3) {
                    return true;
                }

                if (sub_4EB8D0(a1, a2 + 1, a3, a4)) {
                    return true;
                }
            }
        }

        offset += cnt;
    }

    return false;
}

// 0x4EB970
tig_art_id_t sub_4EB970(tig_art_id_t a, tig_art_id_t b)
{
    int v1;
    int v2;
    int v3;
    tig_art_id_t art_id;

    if (!tig_art_tile_id_type_get(a)) {
        return a;
    }

    if (!tig_art_tile_id_type_get(b)) {
        return b;
    }

    v1 = tig_art_tile_id_num1_get(a);
    if (!tig_art_tile_id_flippable1_get(a)) {
        v1 += num_outdoor_flippable_names;
    }

    v2 = tig_art_tile_id_num1_get(b);
    if (!tig_art_tile_id_flippable1_get(b)) {
        v2 += num_outdoor_flippable_names;
    }

    v3 = dword_603AF8[v2 + v1 * (num_outdoor_flippable_names + num_outdoor_non_flippable_names)];
    if (v3 < num_outdoor_flippable_names) {
        tig_art_tile_id_create(v3, v3, 15, 0, 1, 1, 1, 0, &art_id);
    } else {
        v3 -= num_outdoor_flippable_names;
        tig_art_tile_id_create(v3, v3, 15, 0, 1, 0, 0, 0, &art_id);
    }

    return art_id;
}

// 0x4EBA30
bool sub_4EBA30(tig_art_id_t a, tig_art_id_t b)
{
    tig_art_id_t c;
    int num;
    int flippable;

    c = sub_4EB970(a, b);
    num = tig_art_tile_id_num1_get(c);
    flippable = tig_art_tile_id_flippable1_get(c);

    return (num == tig_art_tile_id_num1_get(b)
               && flippable == tig_art_tile_id_flippable1_get(b))
        || (num == tig_art_tile_id_num1_get(a)
            && flippable == tig_art_tile_id_flippable1_get(a));
}

// 0x4EBAB0
bool a_name_tile_is_blocking(tig_art_id_t aid)
{
    return aid != TIG_ART_ID_INVALID
        ? (a_name_tile_id_flags(aid) & TF_BLOCK) != 0
        : false;
}

// 0x4EBAD0
uint8_t a_name_tile_id_flags(tig_art_id_t aid)
{
    int type;
    int num;
    int flippable;

    type = tig_art_tile_id_type_get(aid);
    num = tig_art_tile_id_num1_get(aid);
    flippable = tig_art_tile_id_flippable1_get(aid);

    if (type) {
        if (flippable) {
            return outdoor_flippable_tile_flags[num];
        } else {
            return outdoor_non_flippable_tile_flags[num];
        }
    } else {
        if (flippable) {
            return indoor_flippable_tile_flags[num];
        } else {
            return indoor_non_flippable_tile_flags[num];
        }
    }
}

// 0x4EBB30
bool a_name_tile_is_sinkable(tig_art_id_t aid)
{
    return aid != TIG_ART_ID_INVALID
        ? (a_name_tile_id_flags(aid) & TF_SINKABLE) != 0
        : false;
}

// 0x4EBB80
bool a_name_tile_is_slippery(tig_art_id_t aid)
{
    return aid != TIG_ART_ID_INVALID
        ? (a_name_tile_id_flags(aid) & TF_SLIPPERY) != 0
        : false;
}

// 0x4EBBA0
bool a_name_tile_is_natural(tig_art_id_t aid)
{
    return aid != TIG_ART_ID_INVALID
        ? (a_name_tile_id_flags(aid) & TF_NATURAL) != 0
        : false;
}

// 0x4EBBC0
bool a_name_tile_is_soundproof(tig_art_id_t aid)
{
    return aid != TIG_ART_ID_INVALID
        ? (a_name_tile_id_flags(aid) & TF_SOUNDPROOF) != 0
        : false;
}

// 0x4EBBE0
int a_name_tile_sound(tig_art_id_t aid)
{
    int type;
    int num;
    int flippable;

    type = tig_art_tile_id_type_get(aid);
    num = tig_art_tile_id_num1_get(aid);
    flippable = tig_art_tile_id_flippable1_get(aid);

    if (type) {
        if (flippable) {
            return outdoor_flippable_tile_sounds[num];
        } else {
            return outdoor_non_flippable_tile_sounds[num];
        }
    } else {
        if (flippable) {
            return indoor_flippable_tile_sounds[num];
        } else {
            return indoor_non_flippable_tile_sounds[num];
        }
    }
}

// 0x4EBC40
void sub_4EBC40(void)
{
    int index;
    int v1;
    int v2;
    int v3;

    for (index = 0; index < 7; index++) {
        dword_687660[index] = (uint8_t*)MALLOC(dword_687680[index]);
    }

    if (!sub_4EC020()) {
        for (v1 = 0; v1 < 16; v1++) {
            for (index = 0; index < num_outdoor_flippable_names; index++) {
                dword_687660[0][v1 * num_outdoor_flippable_names + index] = sub_4EBE90(index, index, v1, 1, 1, 1);
            }

            for (index = 0; index < num_outdoor_flippable_names; index++) {
                dword_687660[1][v1 * num_outdoor_non_flippable_names + index] = sub_4EBE90(index, index, v1, 1, 0, 0);
            }

            for (index = 0; index < num_indoor_flippable_names; index++) {
                dword_687660[2][v1 * num_indoor_flippable_names + index] = sub_4EBE90(index, index, v1, 0, 1, 1);
            }

            for (index = 0; index < num_indoor_non_flippable_names; index++) {
                dword_687660[3][v1 * num_indoor_non_flippable_names + index] = sub_4EBE90(index, index, v1, 0, 0, 0);
            }

            for (v2 = 0; v2 < num_outdoor_flippable_names; v2++) {
                for (v3 = 0; v3 < num_outdoor_flippable_names; v3++) {
                    dword_687660[4][(v1 * num_outdoor_flippable_names + v3) * num_outdoor_flippable_names + v2] = sub_4EBE90(v2, v3, v1, 1, 1, 1);
                }
            }

            for (v2 = 0; v2 < num_outdoor_non_flippable_names; v2++) {
                for (v3 = 0; v3 < num_outdoor_non_flippable_names; v3++) {
                    dword_687660[5][(v1 * num_outdoor_non_flippable_names + v3) * num_outdoor_non_flippable_names + v2] = sub_4EBE90(v2, v3, v1, 1, 0, 0);
                }
            }

            for (v2 = 0; v2 < num_outdoor_flippable_names; v2++) {
                for (v3 = 0; v3 < num_outdoor_non_flippable_names; v3++) {
                    dword_687660[6][(v1 * num_outdoor_non_flippable_names + v3) * num_outdoor_flippable_names + v2] = sub_4EBE90(v2, v3, v1, 1, 1, 0);
                }
            }
        }

        sub_4EC0C0();
    }

    dword_603AE4 = true;
}

// 0x4EBE90
int8_t sub_4EBE90(int a1, int a2, int a3, int a4, int a5, int a6)
{
    int8_t index;
    tig_art_id_t art_id;

    for (index = 8; index > 0; index--) {
        if (tig_art_tile_id_create(a1, a2, a3, index - 1, a4, a5, a6, 0, &art_id) == TIG_OK
            && tig_art_exists(art_id) == TIG_OK) {
            break;
        }
    }

    return index;
}

// 0x4EBEF0
int sub_4EBEF0(int a1, int a2, int a3, int a4, int a5, int a6)
{
    if (!dword_603AE4) {
        return 1;
    }

    if (a1 == a2 && a5 == a6) {
        if (a4) {
            if (a5) {
                return dword_687660[0][a3 * num_outdoor_flippable_names + a1];
            } else {
                return dword_687660[1][a3 * num_outdoor_non_flippable_names + a1];
            }
        } else {
            if (a5) {
                return dword_687660[2][a3 * num_indoor_flippable_names + a1];
            } else {
                return dword_687660[3][a3 * num_indoor_non_flippable_names + a1];
            }
        }
    } else {
        if (a5) {
            if (a6) {
                return dword_687660[4][num_outdoor_flippable_names * (a2 + a3 * num_outdoor_flippable_names) + a1];
            } else {
                return dword_687660[6][num_outdoor_flippable_names * (a2 + a3 * num_outdoor_non_flippable_names) + a1];
            }
        } else {
            if (a6) {
                return dword_687660[6][num_outdoor_flippable_names * (a1 + a3 * num_outdoor_non_flippable_names) + a2];
            } else {
                return dword_687660[5][num_outdoor_flippable_names * (a2 + a3 * num_outdoor_flippable_names) + a1];
            }
        }
    }
}

// 0x4EC020
bool sub_4EC020(void)
{
    TigFile* stream;
    int v1;
    int v2;
    int index;

    stream = tig_file_fopen("art\\tile\\tilevariant.dat", "rb");
    if (stream == NULL) {
        return false;
    }

    v1 = sub_4EC160();
    if (tig_file_fread(&v2, sizeof(v2), 1, stream) != 1 || v2 != v1) {
        tig_file_fclose(stream);
        return false;
    }

    for (index = 0; index < 7; index++) {
        if (tig_file_fread(dword_687660[index], 1, dword_687680[index], stream) < dword_687680[index]) {
            break;
        }
    }

    tig_file_fclose(stream);

    return index == 7;
}

// 0x4EC0C0
bool sub_4EC0C0(void)
{
    int cnt;
    TigFile* stream;
    int index;

    cnt = sub_4EC160();

    tig_file_mkdir("art\\tile");
    stream = tig_file_fopen("art\\tile\\tilevariant.dat", "wb");
    if (stream == NULL) {
        return false;
    }

    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        tig_file_fclose(stream);
        return false;
    }

    for (index = 0; index < 7; index++) {
        if (tig_file_fwrite(dword_687660[index], 1, dword_687680[index], stream) < dword_687680[index]) {
            break;
        }
    }

    tig_file_fclose(stream);

    return index == 7;
}

// 0x4EC160
int sub_4EC160(void)
{
    TigFileList list;
    int cnt;

    tig_file_list_create(&list, "art\\tile\\*.art");
    cnt = list.count;
    tig_file_list_destroy(&list);

    return cnt;
}

// 0x4EC190
bool a_name_item_init(void)
{
    if (!mes_load("art\\item\\item_ground.mes", &item_ground_mes_file)) {
        return false;
    }

    if (!mes_load("art\\item\\item_inven.mes", &item_inven_mes_file)) {
        mes_unload(item_ground_mes_file);
        return false;
    }

    if (!mes_load("art\\item\\item_paper.mes", &item_paper_mes_file)) {
        mes_unload(item_inven_mes_file);
        mes_unload(item_ground_mes_file);
        return false;
    }

    if (!mes_load("art\\item\\item_schematic.mes", &item_schematic_mes_file)) {
        mes_unload(item_paper_mes_file);
        mes_unload(item_inven_mes_file);
        mes_unload(item_ground_mes_file);
        return false;
    }

    return true;
}

// 0x4EC250
void a_name_item_exit(void)
{
    mes_unload(item_ground_mes_file);
    mes_unload(item_inven_mes_file);
    mes_unload(item_paper_mes_file);
    mes_unload(item_schematic_mes_file);
}

// 0x4EC290
bool a_name_item_aid_to_fname(tig_art_id_t aid, char* fname)
{
    MesFileEntry mes_file_entry;
    int type;
    int subtype;
    int armor_coverage;
    int disposition;
    mes_file_handle_t mes_file;

    subtype = tig_art_item_id_subtype_get(aid);
    type = tig_art_item_id_type_get(aid);

    mes_file_entry.num = tig_art_num_get(aid) + 20 * (subtype + 50 * type);
    if (type == TIG_ART_ITEM_TYPE_ARMOR) {
        armor_coverage = tig_art_item_id_armor_coverage_get(aid);
        if (armor_coverage != TIG_ART_ARMOR_COVERAGE_TORSO) {
            mes_file_entry.num += 20 * (5 * armor_coverage + 10);
        }
    }

    disposition = tig_art_item_id_disposition_get(aid);
    switch (disposition) {
    case TIG_ART_ITEM_DISPOSITION_GROUND:
        mes_file = item_ground_mes_file;
        break;
    case TIG_ART_ITEM_DISPOSITION_PAPERDOLL:
        mes_file = item_paper_mes_file;
        break;
    case TIG_ART_ITEM_DISPOSITION_SCHEMATIC:
        mes_file = item_schematic_mes_file;
        break;
    default:
        mes_file = item_inven_mes_file;
        break;
    }

    if (!mes_search(mes_file, &mes_file_entry)) {
        return false;
    }

    sprintf(fname, "art\\item\\%s", mes_file_entry.str);

    return true;
}

// 0x4EC370
bool a_name_facade_init(void)
{
    if (!sub_4EC4B0()) {
        return false;
    }

    facade_initialized = true;

    return true;
}

// 0x4EC390
void a_name_facade_exit(void)
{
    if (facade_initialized) {
        mes_unload(facadename_mes_file);
        FREE(facade_names);
        facade_initialized = false;
    }
}

// 0x4EC3F0
bool a_name_facade_aid_to_fname(tig_art_id_t aid, char* fname)
{
    int num;

    if (tig_art_type(aid) != TIG_ART_TYPE_FACADE) {
        tig_debug_println("Asking for facade name with non facade art ID.");
        fname[0] = '\0';
        return false;
    }

    num = tig_art_facade_id_num_get(aid);
    return build_facade_file_name(num, fname);
}

// 0x4EC430
bool build_facade_file_name(int num, char* fname)
{
    sprintf(fname, "art\\Facade\\%s.art", facade_names[num]);
    return true;
}

// 0x4EC4B0
bool sub_4EC4B0(void)
{
    MesFileEntry mes_file_entry;
    int index;

    num_facade_names = 0;
    facade_names = NULL;

    if (!mes_load("art\\facade\\facadename.mes", &facadename_mes_file)) {
        return false;
    }

    num_facade_names = mes_entries_count(facadename_mes_file);
    if (num_facade_names == 0 || num_facade_names >= 512) {
        mes_unload(facadename_mes_file);
        return false;
    }

    mes_file_entry.num = 0;
    if (!mes_search(facadename_mes_file, &mes_file_entry)) {
        num_facade_names = 0;
        mes_unload(facadename_mes_file);
        return false;
    }

    facade_names = (char**)MALLOC(sizeof(char*) * num_facade_names);

    index = 0;
    do {
        facade_names[index++] = mes_file_entry.str;
    } while (mes_find_next(facadename_mes_file, &mes_file_entry));

    return true;
}

// 0x4EC5A0
bool a_name_portal_init(void)
{
    MesFileEntry mes_file_entry;
    char* pch;

    if (!mes_load("art\\portal\\portal.mes", &portal_mes_file)) {
        return false;
    }

    mes_file_entry.num = 0;
    mes_get_msg(portal_mes_file, &mes_file_entry);
    do {
        pch = strchr(mes_file_entry.str, ' ');
        // FIXME: Unsafe dereference.
        *pch = '\0';
    } while (mes_find_next(portal_mes_file, &mes_file_entry));

    return true;
}

// 0x4EC610
void a_name_portal_exit(void)
{
    mes_unload(portal_mes_file);
}

// 0x4EC620
bool a_name_portal_aid_to_fname(tig_art_id_t aid, char* fname)
{
    sprintf(fname, "art\\portal\\%s", sub_4EC8F0(aid));

    if (tig_art_id_damaged_get(aid)) {
        fname[strlen(fname) - 6] = 'D';
    }

    return true;
}

// 0x4EC670
tig_art_id_t a_name_portal_aid_from_wall_aid(tig_art_id_t wall_art_id, ObjectID* oid)
{
    int p_piece;
    int type;
    WallStructure wallstructure;
    char path[TIG_MAX_PATH];
    char* fname;
    int portal_num;
    MesFileEntry mes_file_entry;
    int rotation;
    int palette;
    tig_art_id_t portal_art_id;

    p_piece = tig_art_wall_id_p_piece_get(wall_art_id);

    switch (p_piece) {
    case 22:
    case 25:
    case 26:
    case 29:
    case 30:
    case 31:
    case 32:
        type = TIG_ART_PORTAL_TYPE_DOOR;
        break;
    case 10:
    case 13:
    case 14:
    case 17:
    case 18:
    case 19:
        type = TIG_ART_PORTAL_TYPE_WINDOW;
        break;
    default:
        return TIG_ART_ID_INVALID;
    }

    sub_4ED180(tig_art_wall_id_num_get(wall_art_id), &wallstructure);

    if ((wallstructure.flags & WS_NODOORS) != 0
        && type == TIG_ART_PORTAL_TYPE_DOOR) {
        return TIG_ART_ID_INVALID;
    }

    if ((wallstructure.flags & WS_NOWINDOWS) != 0
        && type == TIG_ART_PORTAL_TYPE_WINDOW) {
        return TIG_ART_ID_INVALID;
    }

    oid->type = OID_TYPE_NULL;

    if (!a_name_wall_aid_to_fname(wall_art_id, path)) {
        return TIG_ART_ID_INVALID;
    }

    fname = &(path[strlen(path) - 12]);

    if (type == TIG_ART_PORTAL_TYPE_WINDOW) {
        fname[3] = 'F';
    } else {
        fname[3] = 'E';
    }

    fname[6] = 'U';

    portal_num = sub_4EC940(fname);
    if (portal_num == -1) {
        return TIG_ART_ID_INVALID;
    }

    mes_file_entry.num = portal_num;
    mes_get_msg(portal_mes_file, &mes_file_entry);

    // `a_name_portal_init` splits string into two chunks.
    *oid = sub_4E6540(atoi(mes_file_entry.str + strlen(mes_file_entry.str) + 1));

    rotation = tig_art_id_rotation_get(wall_art_id);
    palette = tig_art_id_palette_get(wall_art_id);

    if (type == TIG_ART_PORTAL_TYPE_WINDOW) {
        portal_num -= 1001;
    }

    tig_art_portal_id_create(portal_num, type, 0, 0, rotation, palette, &portal_art_id);

    return portal_art_id;
}

// 0x4EC830
tig_art_id_t a_name_portal_aid_busted_set(tig_art_id_t aid)
{
    if (tig_art_portal_id_type_get(aid) != TIG_ART_PORTAL_TYPE_WINDOW) {
        return TIG_ART_ID_INVALID;
    }

    if (!tig_art_id_damaged_get(aid)) {
        aid = tig_art_id_damaged_set(aid, 0x200);
        aid = tig_art_id_frame_set(aid, 0);
    }

    return aid;
}

// 0x4EC8F0
char* sub_4EC8F0(tig_art_id_t aid)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = tig_art_num_get(aid);
    if (tig_art_portal_id_type_get(aid) == TIG_ART_PORTAL_TYPE_WINDOW) {
        mes_file_entry.num += 1001;
    }

    mes_get_msg(portal_mes_file, &mes_file_entry);

    return mes_file_entry.str;
}

// 0x4EC940
int sub_4EC940(const char* fname)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = 0;
    mes_get_msg(portal_mes_file, &mes_file_entry);
    do {
        if (SDL_strcasecmp(mes_file_entry.str, fname) == 0) {
            return mes_file_entry.num;
        }
    } while (mes_find_next(portal_mes_file, &mes_file_entry));

    return -1;
}

// 0x4EC9B0
bool a_name_wall_init(void)
{
    init_wall_names();
    init_wall_structures();

    if (num_wall_file_names == 0 || num_wall_structures == 0) {
        a_name_wall_exit();
        return false;
    }

    return true;
}

// 0x4EC9E0
void a_name_wall_exit(void)
{
    if (num_wall_structures > 0) {
        mes_unload(wall_structure_mes_file);
    }

    if (num_wall_file_names > 0) {
        mes_unload(wallname_mes_file);
    }

    if (wall_file_names != NULL) {
        FREE(wall_file_names);
    }

    if (wall_proto_file_names != NULL) {
        FREE(wall_proto_file_names);
    }

    if (wall_structures != NULL) {
        FREE(wall_structures);
    }

    num_wall_file_names = 0;
    num_wall_structures = 0;
}

// 0x4ECA60
void init_wall_names(void)
{
    MesFileEntry mes_file_entry;
    mes_file_handle_t wallproto_mes_file;
    int index;

    num_wall_file_names = 0;
    wall_file_names = NULL;
    wall_proto_file_names = NULL;

    if (!mes_load("art\\wall\\wallname.mes", &wallname_mes_file)) {
        return;
    }

    num_wall_file_names = mes_entries_count(wallname_mes_file);

    mes_file_entry.num = 0;
    if (!mes_search(wallname_mes_file, &mes_file_entry)) {
        num_wall_file_names = 0;
        mes_unload(wallname_mes_file);
        return;
    }

    if (!mes_load("art\\wall\\wallproto.mes", &wallproto_mes_file)) {
        num_wall_file_names = 0;
        mes_unload(wallname_mes_file);
        return;
    }

    wall_file_names = (char**)MALLOC(sizeof(char*) * num_wall_file_names);
    wall_proto_file_names = (int*)MALLOC(sizeof(int) * num_wall_file_names);

    index = 0;
    do {
        sub_4ECB80(wallproto_mes_file, mes_file_entry.str, index++);
    } while (mes_find_next(wallname_mes_file, &mes_file_entry));

    mes_unload(wallproto_mes_file);
}

// 0x4ECB80
void sub_4ECB80(mes_file_handle_t wallproto_mes_file, char* str, int index)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = atoi(&(str[3]));
    mes_get_msg(wallproto_mes_file, &mes_file_entry);
    wall_proto_file_names[index] = atoi(mes_file_entry.str);
    str[3] = '\0';
    wall_file_names[index] = str;
}

// 0x4ECC00
int sub_4ECC00(int index)
{
    return wall_proto_file_names[index];
}

// 0x4ECC10
void init_wall_structures(void)
{
    MesFileEntry mes_file_entry;
    int index;

    num_wall_structures = 0;
    wall_structures = NULL;

    if (!mes_load("art\\wall\\structure.mes", &wall_structure_mes_file)) {
        return;
    }

    mes_file_entry.num = 0;
    if (!mes_search(wall_structure_mes_file, &mes_file_entry)) {
        return;
    }

    do {
        index = num_wall_structures++;
        wall_structures = (WallStructure*)REALLOC(wall_structures, sizeof(WallStructure) * num_wall_structures);
        wall_structures[index].flags = 0;
        wall_structures[index].interior = 4;
        wall_structures[index].interior_param = 0;
        wall_structures[index].exterior = 0;
        wall_structures[index].exterior_param = 0;
        wall_structures[index].tile_art_id = TIG_ART_ID_INVALID;
        wall_structures[index].roof_art_id = TIG_ART_ID_INVALID;
        wall_structures[index].wall_proto = 0;
        parse_wall_structure(mes_file_entry.str, index);
    } while (mes_find_next(wall_structure_mes_file, &mes_file_entry) && mes_file_entry.num < 1000);
}

// 0x4ECEB0
bool a_name_wall_aid_to_fname(tig_art_id_t art_id, char* path)
{
    int num;
    int rotation;
    int p_piece;
    int damage;
    int variation;
    int new_damage;
    int v1;

    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return false;
    }

    num = tig_art_wall_id_num_get(art_id);
    rotation = tig_art_id_rotation_get(art_id);
    p_piece = tig_art_wall_id_p_piece_get(art_id);
    damage = tig_art_id_damaged_get(art_id);
    variation = tig_art_wall_id_variation_get(art_id);

    switch (rotation) {
    case 2:
    case 3:
    case 6:
    case 7:
        new_damage = 0;
        if ((damage & 0x400) != 0) {
            new_damage |= 0x80;
        }
        if ((damage & 0x80) != 0) {
            new_damage |= 0x400;
        }
        damage = new_damage;
        break;
    }

    if ((damage & 0x400) != 0) {
        new_damage = 0x400;
        if (p_piece == 7) {
            p_piece = 0;
        }
    } else if ((damage & 0x80) != 0) {
        new_damage = 0x80;
        if (p_piece == 8) {
            p_piece = 0;
        }
    } else {
        new_damage = 0;
    }

    if (num >= num_wall_structures) {
        return false;
    }

    if (rotation / 2 == 0 || rotation / 2 == 3) {
        v1 = wall_structures[num].interior;
    } else {
        v1 = wall_structures[num].exterior;
    }

    if (v1 >= num_wall_file_names) {
        return false;
    }

    return build_wall_file_name(wall_file_names[v1], p_piece, new_damage, variation, path);
}

// 0x4ECFC0
bool build_wall_file_name(const char* name, int piece, int damage, int variation, char* fname)
{
    // 0x5BB6AC
    static const char off_5BB6AC[] = {
        'U',
        'L',
        'R'
    };

    // 0x5BB6B0
    static const char* off_5BB6B0[] = {
        "bse",
        "lfc",
        "bse",
        "bcl",
        "bcr",
        "tcl",
        "tcr",
        "uec",
        "lec",
        "w3l",
        "w3a",
        "w3r",
        "w4l",
        "w4a",
        "w4b",
        "w4r",
        "w5l",
        "w5a",
        "w5b",
        "w5c",
        "w5r",
        "d3l",
        "d3a",
        "d3r",
        "d4l",
        "d4a",
        "d4b",
        "d4r",
        "d6l",
        "d6a",
        "d6b",
        "d6c",
        "d6d",
        "d6r",
        "p3l",
        "p3a",
        "p3r",
        "p4l",
        "p4a",
        "p4b",
        "p4r",
        "p5l",
        "p5a",
        "p5b",
        "p5c",
        "p5r",
    };

    int index;

    if ((damage & 0x400) != 0) {
        index = 1;
    } else if ((damage & 0x80) != 0) {
        if (piece >= 2 && piece <= 6) {
            index = 1;
        } else {
            index = 2;
        }
    } else {
        index = 0;
    }

    sprintf(fname,
        "art\\wall\\%s%s%c%c.art",
        name,
        off_5BB6B0[piece],
        off_5BB6AC[index],
        variation + '0');

    return true;
}

// 0x4ED070
tig_art_id_t a_name_wall_aid_damage_set(tig_art_id_t art_id, unsigned int flags)
{
    int p_piece;
    unsigned int damage;
    int rotation;
    unsigned int new_damage;

    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return art_id;
    }

    p_piece = tig_art_wall_id_p_piece_get(art_id);
    damage = tig_art_id_damaged_get(art_id);
    damage |= flags;
    art_id = tig_art_id_damaged_set(art_id, damage);
    rotation = tig_art_id_rotation_get(art_id);

    switch (rotation) {
    case 2:
    case 3:
    case 6:
    case 7:
        new_damage = 0;
        if ((damage & 0x400) != 0) {
            new_damage |= 0x80;
        }
        if ((damage & 0x80) != 0) {
            new_damage |= 0x400;
        }
        damage = new_damage;
        break;
    }

    switch (p_piece) {
    case 0:
        if (damage == (0x400 | 0x80)) {
            return TIG_ART_ID_INVALID;
        }
        return art_id;
    case 7:
    case 9:
    case 12:
    case 16:
    case 21:
    case 24:
    case 28:
    case 34:
    case 37:
    case 41:
        if ((damage & 0x80) != 0) {
            return TIG_ART_ID_INVALID;
        }
        return art_id;
    case 8:
    case 11:
    case 15:
    case 20:
    case 23:
    case 27:
    case 33:
    case 36:
    case 40:
    case 45:
        if ((damage & 0x400) != 0) {
            return TIG_ART_ID_INVALID;
        }
        return art_id;
    case 10:
    case 13:
    case 14:
    case 17:
    case 18:
    case 19:
    case 22:
    case 25:
    case 26:
    case 29:
    case 30:
    case 31:
    case 32:
    case 35:
    case 38:
    case 39:
    case 42:
    case 43:
    case 44:
        if (damage != 0) {
            return TIG_ART_ID_INVALID;
        }
        return art_id;
    default:
        return art_id;
    }
}

// 0x4ED030
int sub_4ED030(const char* str)
{
    int index;

    for (index = 0; index < num_wall_file_names; index++) {
        if (SDL_strcasecmp(wall_file_names[index], str) == 0) {
            return index;
        }
    }

    return -1;
}

// 0x4ED180
void sub_4ED180(int index, WallStructure* wallstructure)
{
    *wallstructure = wall_structures[index];
}

// 0x4ED1A0
int a_name_num_wall_structures(void)
{
    return num_wall_structures;
}

// 0x4ED1B0
char* a_name_wall_get_structure(int index)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = index + 1000;
    if (!mes_search(wall_structure_mes_file, &mes_file_entry)) {
        return NULL;
    }

    return mes_file_entry.str;
}

// 0x4ECD10
void parse_wall_structure(char* str, int index)
{
    char* tok;

    tok = strtok(str, " ");
    wall_structures[index].interior_param = atoi(tok + 3);

    tok[3] = '\0';
    wall_structures[index].interior = sub_4ED030(tok);

    tok = strtok(NULL, " ");
    wall_structures[index].exterior_param = atoi(tok + 3);

    tok[3] = '\0';
    wall_structures[index].exterior = sub_4ED030(tok);

    tok = strtok(NULL, " ");
    if (SDL_strcasecmp(tok, "nul") != 0) {
        a_name_tile_fname_to_aid(tok, &(wall_structures[index].tile_art_id));
    } else {
        wall_structures[index].tile_art_id = TIG_ART_ID_INVALID;
    }

    tok = strtok(NULL, " ");
    if (SDL_strcasecmp(tok, "nul") != 0) {
        a_name_roof_fname_to_aid(tok, &(wall_structures[index].roof_art_id));
    } else {
        wall_structures[index].roof_art_id = TIG_ART_ID_INVALID;
    }

    wall_structures[index].wall_proto = sub_4ECC00(wall_structures[index].exterior);

    tok = strtok(NULL, " ");
    while (tok != NULL) {
        if (SDL_strcasecmp(tok, "/nowindows") == 0) {
            wall_structures[index].flags |= WS_NOWINDOWS;
        } else if (SDL_strcasecmp(tok, "/nodoors") == 0) {
            wall_structures[index].flags |= WS_NODOORS;
        } else if (SDL_strcasecmp(tok, "/fence") == 0) {
            wall_structures[index].flags |= WS_FENCE;
        }
    }
}

// 0x4ED1E0
bool a_name_light_init(void)
{
    if (light_initialized) {
        return true;
    }

    if (!mes_load("art\\light\\light.mes", &light_mes_file)) {
        return false;
    }

    light_initialized = true;

    return true;
}

// 0x4ED220
void a_name_light_exit(void)
{
    if (light_initialized) {
        mes_unload(light_mes_file);
        light_initialized = false;
    }
}

// 0x4ED250
bool a_name_light_aid_to_fname(tig_art_id_t aid, char* fname)
{
    MesFileEntry mes_file_entry;

    if (!light_initialized) {
        return false;
    }

    mes_file_entry.num = tig_art_num_get(aid);
    if (!mes_search(light_mes_file, &mes_file_entry)) {
        return false;
    }

    if (sub_504790(aid)) {
        sprintf(fname, "art\\light\\%s_s%d.art", mes_file_entry.str, sub_504700(aid) / 8);
    } else {
        sprintf(fname, "art\\light\\%s.art", mes_file_entry.str);
    }

    return true;
}

// 0x4ED2F0
bool a_name_roof_init(void)
{
    if (!load_roof_data()) {
        return false;
    }

    roof_initialized = true;

    return true;
}

// 0x4ED310
void a_name_roof_exit(void)
{
    if (roof_initialized) {
        mes_unload(roofname_mes_file);
        FREE(roof_file_names);
        roof_initialized = false;
    }
}

// 0x4ED370
bool a_name_roof_aid_to_fname(tig_art_id_t aid, char* fname)
{
    if (tig_art_type(aid) != TIG_ART_TYPE_ROOF) {
        return false;
    }

    return build_roof_file_name(tig_art_num_get(aid), fname);
}

// 0x4ED3A0
bool build_roof_file_name(int index, char* fname)
{
    if (index >= num_roof_file_names) {
        return false;
    }

    sprintf(fname, "art\\roof\\%s.art", roof_file_names[index]);

    return true;
}

// 0x4ED3E0
bool a_name_roof_fname_to_aid(const char* fname, tig_art_id_t* aid_ptr)
{
    int index;

    for (index = 0; index < num_roof_file_names; index++) {
        if (SDL_strcasecmp(fname, roof_file_names[index]) == 0) {
            tig_art_roof_id_create(index, 0, 0, 0, aid_ptr);
            return true;
        }
    }

    return false;
}

// 0x4ED440
bool load_roof_data(void)
{
    MesFileEntry mes_file_entry;
    int index;

    num_roof_file_names = 0;
    roof_file_names = NULL;

    if (!mes_load("art\\roof\\roofname.mes", &roofname_mes_file)) {
        return false;
    }

    num_roof_file_names = mes_entries_count(roofname_mes_file);
    if (num_roof_file_names == 0) {
        mes_unload(roofname_mes_file);
        return false;
    }

    mes_file_entry.num = 0;
    if (!mes_search(roofname_mes_file, &mes_file_entry)) {
        mes_unload(roofname_mes_file);
        return false;
    }

    roof_file_names = MALLOC(sizeof(char*) * num_roof_file_names);
    index = 0;
    do {
        roof_file_names[index++] = mes_file_entry.str;
    } while (mes_find_next(roofname_mes_file, &mes_file_entry));

    return true;
}
