#include "ui/wmap_ui.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/antiteleport.h"
#include "game/area.h"
#include "game/critter.h"
#include "game/gamelib.h"
#include "game/gsound.h"
#include "game/hrp.h"
#include "game/location.h"
#include "game/magictech.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/obj.h"
#include "game/path.h"
#include "game/player.h"
#include "game/sector.h"
#include "game/teleport.h"
#include "game/townmap.h"
#include "game/ui.h"
#include "ui/anim_ui.h"
#include "ui/intgame.h"
#include "ui/textedit_ui.h"
#include "ui/types.h"

typedef enum WmapUiMode {
    WMAP_UI_MODE_WORLD,
    WMAP_UI_MODE_CONTINENT,
    WMAP_UI_MODE_TOWN,
    WMAP_UI_MODE_COUNT,
} WmapUiMode;

typedef enum WmapNoteType {
    WMAP_NOTE_TYPE_NOTE,
    WMAP_NOTE_TYPE_SKULL,
    WMAP_NOTE_TYPE_CHEST,
    WMAP_NOTE_TYPE_QUEST,
    WMAP_NOTE_TYPE_CROSS,
    WMAP_NOTE_TYPE_LOC,
    WMAP_NOTE_TYPE_WAYPOINT,
    WMAP_NOTE_TYPE_NEW_LOC,
    WMAP_NOTE_TYPE_COUNT,
} WmapNoteType;

typedef enum WmapRouteType {
    WMAP_ROUTE_TYPE_WORLD,
    WMAP_ROUTE_TYPE_TOWN,
    WMAP_ROUTE_TYPE_COUNT,
} WmapRouteType;

typedef enum WmapUiState {
    /**
     * The UI is stationary.
     */
    WMAP_UI_STATE_NORMAL,

    /**
     * NOTE: Not implemented in the original code. Looks like the idea was to
     * allow moving/updating individual waypoints on the path.
     */
    WMAP_UI_STATE_1,

    /**
     * Moving along the route.
     */
    WMAP_UI_STATE_MOVING,

    /**
     * NOTE: Not implemented in the original code. The idea was to allow
     * creating/editing custom map notes.
     */
    WMAP_UI_STATE_EDITING,

    /**
     * Navigating the map by pressing and holding directional pad in the center
     * of the map.
     */
    WMAP_UI_STATE_NAVIGATING,
} WmapUiState;

typedef struct WmapCoords {
    int x;
    int y;
} WmapCoords;

typedef struct WmapNoteTypeInfo {
    // NOTE: Rare case - during initialization the value at offset 0 is art num,
    // which is replaced with interface art id (see `wmap_ui_init`).
    union {
        int num;
        tig_art_id_t art_id;
    } data;
    /* 0004 */ int width;
    /* 0008 */ int height;
    /* 000C */ tig_color_t color;
    /* 0010 */ tig_font_handle_t font;
    /* 0014 */ bool enabled;
} WmapNoteTypeInfo;

typedef struct WmapNote {
    /* 0000 */ int id;
    /* 0004 */ unsigned int flags;
    /* 0008 */ WmapCoords coords;
    /* 0010 */ int64_t field_10;
    /* 0018 */ TigRect field_18;
    /* 0028 */ WmapNoteType type;
    /* 002C */ char str[250];
    /* 0128 */ TigVideoBuffer* video_buffer;
} WmapNote;

typedef struct WmapTile {
    /* 0000 */ unsigned int flags;
    /* 0004 */ TigVideoBuffer* video_buffer;
    /* 0008 */ TigRect rect;
    /* 0018 */ void* field_18;
    /* 001C */ int field_1C;
} WmapTile;

typedef struct WmapInfo {
    /* 0000 */ unsigned int flags;
    /* 0004 */ TigRect rect;
    /* 0014 */ TigRect field_14;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
    /* 0030 */ int field_30;
    /* 0034 */ int field_34;
    /* 0038 */ int field_38;
    /* 003C */ WmapCoords field_3C;
    /* 0044 */ bool navigatable;
    /* 0048 */ void (*refresh)();
    /* 004C */ void (*refresh_rect)(TigRect* rect);
    /* 0050 */ void (*field_50)(int direction, int, int, int);
    /* 0054 */ int field_54;
    /* 0058 */ int map_width;
    /* 005C */ int map_height;
    /* 0060 */ int field_60;
    /* 0064 */ int field_64;
    /* 0068 */ char field_68[TIG_MAX_PATH];
    /* 016C */ int tile_width;
    /* 0170 */ int tile_height;
    /* 0174 */ int num_tiles;
    /* 0178 */ int num_hor_tiles;
    /* 017C */ int num_vert_tiles;
    /* 0180 */ WmapTile* tiles;
    /* 0184 */ int num_loaded_tiles;
    /* 0188 */ WmapNote* notes;
    /* 018C */ int field_18C;
    /* 0190 */ int* num_notes;
    /* 0194 */ int field_194;
    /* 0198 */ int field_198;
    /* 019C */ int field_19C;
    /* 01A0 */ int field_1A0;
    /* 01A4 */ char str[260];
    /* 02A8 */ TigRect field_2A8;
    /* 02B8 */ TigVideoBuffer* video_buffer;
    /* 02BC */ void (*field_2BC)(int x, int y, WmapCoords* coords);
} WmapInfo;

typedef struct WmapRouteWaypoint {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
    /* 0008 */ WmapCoords coords;
    /* 0010 */ int64_t loc;
    /* 0018 */ int start;
    /* 001C */ int steps;
} WmapRouteWaypoint;

typedef struct WmapRoute {
    /* 0000 */ WmapRouteWaypoint waypoints[30];
    /* 03C0 */ int length;
    /* 03C4 */ int field_3C4;
    /* 03C8 */ int field_3C8;
    /* 03CC */ int field_3CC;
} WmapRoute;

// 0x5C9160
static WmapNoteTypeInfo wmap_note_type_info[WMAP_NOTE_TYPE_COUNT] = {
    /*     WMAP_NOTE_TYPE_NOTE */ { 142, 0, 0, 0, 0, true },
    /*    WMAP_NOTE_TYPE_SKULL */ { 144, 0, 0, 0, 0, true },
    /*    WMAP_NOTE_TYPE_CHEST */ { 141, 0, 0, 0, 0, true },
    /*    WMAP_NOTE_TYPE_QUEST */ { 143, 0, 0, 0, 0, true },
    /*    WMAP_NOTE_TYPE_CROSS */ { 204, 0, 0, 0, 0, true },
    /*      WMAP_NOTE_TYPE_LOC */ { 205, 0, 0, 0, 0, true },
    /* WMAP_NOTE_TYPE_WAYPOINT */ { 206, 0, 0, 0, 0, true },
    /*  WMAP_NOTE_TYPE_NEW_LOC */ { 814, 0, 0, 0, 0, true },
};

static void sub_560010();
static void wmap_ui_town_notes_save();
static void wmap_ui_destroy();
static bool wmap_ui_town_note_save(WmapNote* note, TigFile* stream);
static bool wmap_ui_rect_write(TigRect* rect, TigFile* stream);
static bool wmap_ui_town_note_load(WmapNote* note, TigFile* stream);
static bool wmap_ui_rect_read(TigRect* rect, TigFile* stream);
static void wmap_ui_open_internal();
static bool wmap_load_worldmap_info();
static void sub_560EE0();
static void sub_560EF0();
static bool wmap_ui_create();
static void sub_561430(int64_t location);
static void sub_561490(int64_t location, WmapCoords* coords);
static void sub_5614C0(int x, int y);
static bool is_default_wmap();
static bool wmap_ui_state_set(WmapUiState state);
static void sub_561800(WmapCoords* coords, int64_t* loc_ptr);
static bool wmap_ui_teleport(int64_t loc);
static bool wmap_ui_message_filter(TigMessage* msg);
static bool sub_5627B0(WmapCoords* coords);
static bool sub_5627F0(int64_t loc);
static void sub_562800(int id);
static void wmap_ui_draw_coords(WmapCoords* coords);
static void wmap_ui_navigate(int x, int y);
static void sub_562AF0(int x, int y);
static void wmap_ui_mode_set(WmapUiMode mode);
static bool wmap_load_townmap_info();
static void sub_562F90(WmapTile* a1);
static bool wmTileArtLock(int tile);
static void wmTileArtUnlock(int tile);
static void wmTileArtUnload(int tile);
static bool wmTileArtLockMode(WmapUiMode mode, int tile);
static bool wmTileArtLoad(const char* path, TigVideoBuffer** video_buffer_ptr, TigRect* rect);
static bool wmTileArtUnlockMode(WmapUiMode mode, int tile);
static void wmTileArtUnloadMode(WmapUiMode mode, int tile);
static void sub_563270();
static void sub_5632A0(int direction, int a2, int a3, int a4);
static void sub_563300(int direction, int a2, int a3, int a4);
static void sub_563590(WmapCoords* a1, bool a2);
static void wmap_ui_handle_scroll();
static void wmap_ui_scroll_with_kb(int direction);
static void wmap_ui_scroll_internal(int direction, int scale);
static void sub_563AC0(int x, int y, WmapCoords* coords);
static void sub_563B10(int x, int y, WmapCoords* coords);
static void sub_563C00(int x, int y, WmapCoords* coords);
static bool sub_563C60(WmapNote* note);
static void sub_563D50(WmapNote* note);
static void sub_563D80(int a1, int a2);
static WmapNote* sub_563D90(int id);
static bool find_note_by_coords(WmapCoords* coords, int* id_ptr);
static bool find_note_by_coords_mode(WmapCoords* coords, int* id_ptr, WmapUiMode mode);
static bool sub_563F00(WmapCoords* coords, int64_t* a2);
static void sub_563F90(WmapCoords* coords);
static void sub_564030(WmapNote* note);
static void sub_564070(bool a1);
static void wmap_ui_textedit_on_enter(TextEdit* textedit);
static bool wmap_note_add(WmapNote* note);
static bool wmap_note_add_mode(WmapNote* note, WmapUiMode mode);
static bool wmap_note_remove(WmapNote* note);
static void sub_5642E0(int id, WmapUiMode mode);
static void sub_5642F0(WmapNote* note);
static void wmap_ui_textedit_on_change(TextEdit* textedit);
static void sub_564360(int id);
static bool sub_5643C0(const char* str);
static bool sub_5643E0(WmapCoords* coords);
static bool sub_564780(WmapCoords* coords, int* idx_ptr);
static void sub_564830(int a1, WmapCoords* coords);
static void sub_564840(int a1);
static void sub_5648E0(int a1, int a2, bool a3);
static void sub_564940();
static void sub_564970(WmapRouteWaypoint* wp);
static void sub_5649C0();
static void sub_5649D0(int a1);
static bool sub_5649F0(int64_t loc);
static void sub_564A70(int64_t pc_obj, int64_t loc);
static void sub_564E30(WmapCoords* coords, int64_t* loc_ptr);
static int64_t sub_564EE0(WmapCoords* a1, WmapCoords* a2, DateTime* datetime);
static void wmap_ui_town_notes_load();
static void sub_5650C0();
static int wmap_ui_compass_arrow_frame_calc(WmapCoords* a, WmapCoords* b);
static void wmap_ui_compass_arrow_frame_set(int frame);
static int wmap_ui_compass_arrow_frame_get();
static bool sub_565140();
static void sub_565170(WmapCoords* coords);
static void sub_565230();
static void sub_5656B0(int x, int y, WmapCoords* coords);
static void wmap_world_refresh_rect(TigRect* rect);
static bool sub_565CF0(WmapNote* note);
static void sub_565D00(WmapNote* note, TigRect* a2, TigRect* a3);
static void wmap_note_vbid_lock(WmapNote* note);
static void sub_565F00(TigVideoBuffer* video_buffer, TigRect* rect);
static void wmap_town_refresh_rect(TigRect* rect);
static void sub_566A80(WmapInfo* a1, TigRect* a2, TigRect* a3);
static void sub_566D10(int type, WmapCoords* coords, TigRect* a3, TigRect* a4, WmapInfo* wmap_info);

// 0x5C9220
static int wmap_ui_spell = -1;

// 0x5C9A68
static TigRect wmap_ui_canvas_frame = { 150, 52, 501, 365 };

// 0x5C9A78
static TigRect stru_5C9A78 = { 662, 248, 130, 130 };

// 0x5C9A88
static TigRect stru_5C9A88 = { 391, 403, 10, 12 };

// 0x5C9A98
static TigRect wmap_ui_lat_long_frame[] = {
    { 707, 200, 60, 18 },
    { 707, 232, 60, 18 },
};

// 0x5C9AB8
static TigRect wmap_ui_pc_lens_frame = { 25, 65, 89, 89 };

// 0x5C9AC8
static TigRect wmap_ui_note_bounds = { 0, 0, 200, 50 };

// 0x5C9AD8
static int dword_5C9AD8 = -1;

// 0x5C9B00
static tig_window_handle_t wmap_ui_window = TIG_WINDOW_HANDLE_INVALID;

// 0x5C9B08
static TigRect wmap_ui_window_frame = { 0, 41, 800, 400 };

// 0x5C9B18
static int dword_5C9B18 = -1;

// 0x5C9B20
static TextEdit wmap_ui_textedit = {
    0,
    "",
    50,
    wmap_ui_textedit_on_enter,
    wmap_ui_textedit_on_change,
    NULL,
};

// 0x5C9B38
static TigRect wmap_ui_nav_cvr_frame = { 294, 0, 0, 0 };

// 0x5C9B48
static tig_art_id_t wmap_ui_nav_cvr_art_id = TIG_ART_ID_INVALID;

// 0x5C9B50
static UiButtonInfo wmap_ui_zoom_button_info[2] = {
    { 20, 206, 811, TIG_BUTTON_HANDLE_INVALID },
    { 19, 308, 812, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C9B70
static UiButtonInfo wmap_ui_navigate_button_info = { 375, 388, 139, TIG_BUTTON_HANDLE_INVALID };

// 0x5C9B80
static UiButtonInfo wmap_ui_travel_button_info = { 698, 353, 813, TIG_BUTTON_HANDLE_INVALID };

// 0x5C9B90
static TigButtonFlags wmap_ui_travel_button_flags = TIG_BUTTON_MOMENTARY;

// 0x5C9B94
static int wmap_ui_scroll_speed_x = 5;

// 0x5C9B98
static int wmap_ui_scroll_speed_y = 5;

// 0x64E030
static tig_color_t dword_64E030;

// 0x64E034
static tig_color_t dword_64E034;

// 0x64E03C
static tig_color_t dword_64E03C;

// 0x64E048
static WmapRoute wmap_ui_routes[WMAP_ROUTE_TYPE_COUNT];

// 0x64E7E8
static WmapCoords stru_64E7E8;

// 0x64E7F0
static mes_file_handle_t wmap_ui_worldmap_mes_file;

// 0x64E7F4
static TigVideoBuffer* dword_64E7F4;

// 0x64E7F8
static TownMapInfo wmap_ui_tmi;

// 0x64E828
static uint8_t byte_64E828[5000];

// 0x64FBB0
static TigRect wmap_ui_compass_base_bounds;

// 0x64FBC8
static PcLens wmap_ui_pc_lens;

// 0x64FBD8
static WmapNote wmap_ui_world_notes[200];

// 0x65E968
static int dword_65E968;

// 0x65E970
static tig_color_t dword_65E970;

// 0x65E974
static tig_color_t dword_65E974;

// 0x65E978
static WmapNote wmap_ui_town_notes[200];

// 0x66D6F8
static int dword_66D6F8;

// 0x66D6FC
static mes_file_handle_t wmap_ui_worldmap_info_mes_file;

// 0x66D708
static TigRect stru_66D708;

// 0x66D718
static WmapNote stru_66D718;

// 0x66D848
static tig_sound_handle_t wmap_ui_music_handle;

// 0x66D850
static int64_t qword_66D850;

// 0x66D858
static int wmap_ui_offset_x;

// 0x66D85C
static int wmap_ui_offset_y;

// 0x66D860
static bool wmap_ui_initialized;

// 0x66D864
static bool wmap_ui_created;

// 0x66D868
static WmapUiMode wmap_ui_mode;

// 0x66D86C
static int wmap_ui_num_world_notes;

// 0x66D870
static int wmap_ui_num_town_notes;

// 0x66D874
static int wmap_ui_townmap;

// 0x66D878
static int dword_66D878;

// 0x66D87C
static int dword_66D87C;

// 0x66D880
static int dword_66D880;

// 0x66D888
static int64_t wmap_ui_obj;

// 0x66D890
static int wmap_note_type_icon_max_width;

// 0x66D894
static int wmap_note_type_icon_max_height;

// 0x66D898
static bool wmap_ui_textedit_focused;

// 0x66D89C
static WmapNoteType dword_66D89C;

// 0x66D8A0
static int wmap_ui_note_width;

// 0x66D8A4
static int wmap_ui_note_height;

// 0x66D8A8
static bool wmap_ui_navigating;

// 0x66D8AC
static int wmap_ui_state;

// 0x66D8B0
static int wmap_ui_compass_arrow_num_frames;

// 0x66D8B4
static int wmap_ui_nav_cvr_width;

// 0x66D8B8
static int wmap_ui_nav_cvr_height;

// 0x66D8BC
static char wmap_ui_module_name[256];

// 0x66D9BC
static bool wmap_ui_info_loaded;

// 0x66D9C0
static const char* wmap_ui_action;

// 0x66D9C4
static bool wmap_ui_encounter;

// 0x66D9C8
static bool wmap_ui_selecting;

// 0x66D9CC
static int dword_66D9CC;

// 0x66D9D0
static tig_timestamp_t dword_66D9D0;

// 0x66D9D4
static WmapNote* dword_66D9D4;

// NOTE: Rare case - this array is put behind uninitialized stuff because it
// obtains references to many internal state variables.
//
// 0x5C9228
static WmapInfo wmap_ui_mode_info[WMAP_UI_MODE_COUNT] = {
    {
        6,
        { 150, 52, 501, 365 },
        { 150, 52, 501, 365 },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        sub_563270,
        wmap_world_refresh_rect,
        sub_5632A0,
        -1,
        0,
        0,
        0,
        0,
        { 0 },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        wmap_ui_world_notes,
        200,
        &wmap_ui_num_world_notes,
        200,
        -1,
        0,
        0,
        { 0 },
        { 0 },
        NULL,
        sub_563AC0,
    },
    {
        0,
        { 218, 52, 365, 365 },
        { 218, 52, 365, 365 },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        sub_565230,
        NULL,
        NULL,
        -1,
        0,
        0,
        0,
        0,
        { 0 },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        wmap_ui_world_notes,
        200,
        &wmap_ui_num_world_notes,
        200,
        -1,
        0,
        0,
        { 0 },
        { 0 },
        NULL,
        sub_563B10,
    },
    {
        4,
        { 150, 52, 501, 365 },
        { 150, 52, 501, 365 },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        sub_563270,
        wmap_town_refresh_rect,
        sub_563300,
        -1,
        0,
        0,
        0,
        0,
        { 0 },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        wmap_ui_town_notes,
        200,
        &wmap_ui_num_town_notes,
        200,
        -1,
        0,
        0,
        { 0 },
        { 0 },
        NULL,
        sub_563C00,
    }
};

// 0x55F8D0
bool wmap_ui_init(GameInitInfo* init_info)
{
    MesFileEntry mes_file_entry;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    tig_art_id_t art_id;
    int index;

    (void)init_info;

    if (!mes_load("mes\\WorldMap.mes", &wmap_ui_worldmap_mes_file)) {
        tig_debug_printf("wmap_ui_init: ERROR: failed to load msg file: mes\\WorldMap.mes!");
        exit(EXIT_FAILURE);
    }

    mes_file_entry.num = 499; // "Action"
    if (mes_search(wmap_ui_worldmap_mes_file, &mes_file_entry)) {
        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
        wmap_ui_action = mes_file_entry.str;
    }

    wmap_ui_mode = WMAP_UI_MODE_WORLD;
    wmap_ui_state = WMAP_UI_STATE_NORMAL;

    if (tig_art_interface_id_create(217, 0, 0, 0, &wmap_ui_nav_cvr_art_id) != TIG_OK
        || tig_art_frame_data(wmap_ui_nav_cvr_art_id, &art_frame_data) != TIG_OK) {
        tig_debug_printf("WMapUI: ERROR: Can't build button cover art info!\n");
        return false;
    }

    wmap_ui_nav_cvr_width = art_frame_data.width;
    wmap_ui_nav_cvr_height = art_frame_data.height;
    dword_65E970 = tig_color_make(50, 160, 50);
    dword_65E974 = tig_color_make(160, 50, 50);
    dword_64E030 = tig_color_make(50, 50, 235);
    dword_64E034 = tig_color_make(50, 50, 110);

    for (index = 0; index < WMAP_UI_MODE_COUNT; index++) {
        if (wmap_ui_mode_info[index].rect.y > wmap_ui_window_frame.y) {
            wmap_ui_mode_info[index].rect.y -= wmap_ui_window_frame.y;
        }
    }

    wmap_ui_canvas_frame.y -= wmap_ui_window_frame.y;
    wmap_ui_lat_long_frame[0].y -= wmap_ui_window_frame.y;
    wmap_ui_lat_long_frame[1].y -= wmap_ui_window_frame.y;
    stru_5C9A78.y -= wmap_ui_window_frame.y;
    wmap_ui_pc_lens_frame.y -= wmap_ui_window_frame.y;
    dword_64E03C = tig_color_make(0, 0, 255);

    for (index = 0; index < WMAP_NOTE_TYPE_COUNT; index++) {
        wmap_note_type_info[index].enabled = true;
        tig_art_interface_id_create(wmap_note_type_info[index].data.num, 0, 0, 0, &art_id);
        wmap_note_type_info[index].data.art_id = art_id;

        if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
            return false;
        }

        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
            wmap_note_type_info[index].width = art_frame_data.width;
            wmap_note_type_info[index].height = art_frame_data.height;

            if (art_frame_data.width > wmap_note_type_icon_max_width) {
                wmap_note_type_icon_max_width = art_frame_data.width;
            }

            if (art_frame_data.height > wmap_note_type_icon_max_height) {
                wmap_note_type_icon_max_height = art_frame_data.height;
            }
        }
    }

    wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].color = tig_color_make(255, 255, 255);
    wmap_note_type_info[WMAP_NOTE_TYPE_SKULL].color = tig_color_make(255, 0, 0);
    wmap_note_type_info[WMAP_NOTE_TYPE_CHEST].color = tig_color_make(255, 255, 0);
    wmap_note_type_info[WMAP_NOTE_TYPE_QUEST].color = tig_color_make(0, 255, 0);
    wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].color = tig_color_make(150, 150, 150);
    wmap_note_type_info[WMAP_NOTE_TYPE_LOC].color = tig_color_make(250, 250, 250);

    mes_file_entry.num = 655;
    if (mes_search(wmap_ui_worldmap_mes_file, &mes_file_entry)) {
        char* str;
        int r, g, b;

        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
        tig_str_parse_set_separator(',');
        str = mes_file_entry.str;
        tig_str_parse_value(&str, &r);
        tig_str_parse_value(&str, &g);
        tig_str_parse_value(&str, &b);
        wmap_note_type_info[WMAP_NOTE_TYPE_LOC].color = tig_color_make(r, g, b);
    }

    wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].color = tig_color_make(150, 150, 150);

    stru_66D708.x = 0;
    stru_66D708.y = wmap_ui_note_bounds.y;
    stru_66D708.width = wmap_ui_note_bounds.width + wmap_note_type_icon_max_width + 3;
    stru_66D708.height = wmap_ui_note_bounds.height;

    wmap_ui_note_bounds.x = wmap_note_type_icon_max_width + 3;

    if (tig_art_interface_id_create(217, 0, 0, 0, &art_id) != TIG_OK
        || tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    wmap_ui_nav_cvr_frame.y = 382 - wmap_ui_window_frame.y;
    wmap_ui_nav_cvr_frame.width = art_frame_data.width;
    wmap_ui_nav_cvr_frame.height = art_frame_data.height;

    if (tig_art_interface_id_create(196, 0, 0, 0, &art_id) != TIG_OK
        || tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    wmap_ui_compass_base_bounds.x = 0;
    wmap_ui_compass_base_bounds.y = 0;
    wmap_ui_compass_base_bounds.width = art_frame_data.width;
    wmap_ui_compass_base_bounds.height = art_frame_data.height;

    if (tig_art_interface_id_create(197, 0, 0, 0, &art_id) != TIG_OK
        || tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        return false;
    }

    wmap_ui_compass_arrow_num_frames = art_anim_data.num_frames;

    for (index = 0; index < WMAP_ROUTE_TYPE_COUNT; index++) {
        wmap_ui_routes[index].length = 0;
        wmap_ui_routes[index].field_3C4 = 1;
        wmap_ui_routes[index].field_3C8 = 1;
    }

    wmap_ui_initialized = true;
    return true;
}

// 0x55FF80
void wmap_ui_exit()
{
    int mode;
    int tile;

    wmap_ui_mode = WMAP_UI_MODE_WORLD;
    wmap_ui_state = WMAP_UI_STATE_NORMAL;

    for (mode = 0; mode < WMAP_UI_MODE_COUNT; mode++) {
        if (wmap_ui_mode_info[mode].tiles != NULL) {
            for (tile = 0; tile < wmap_ui_mode_info[mode].num_tiles; tile++) {
                if (wmap_ui_mode_info[mode].tiles[tile].field_18 != NULL) {
                    FREE(wmap_ui_mode_info[mode].tiles[tile].field_18);
                }
            }

            FREE(wmap_ui_mode_info[mode].tiles);
            wmap_ui_mode_info[mode].tiles = NULL;

            wmap_ui_mode_info[mode].num_tiles = 0;
        }
    }

    wmap_ui_destroy();
    sub_560010();
    mes_unload(wmap_ui_worldmap_mes_file);
    wmap_ui_initialized = false;
}

// 0x560010
void sub_560010()
{
    wmap_ui_town_notes_save();
    wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_tiles = 0;
    if (wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tiles != NULL) {
        FREE(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tiles);
        wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tiles = NULL;
    }
}

// 0x560040
void wmap_ui_town_notes_save()
{
    int index;
    bool success = true;
    char path[TIG_MAX_PATH];
    TigFile* stream;

    if (dword_66D878 == 0) {
        return;
    }

    if (dword_66D878 >= townmap_count()) {
        dword_66D878 = 0;
        return;
    }

    sprintf(path, "%s\\%s.tmn", "Save\\Current", townmap_name(dword_66D878));

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("wmap_ui_town_notes_save(): Error creating file %s\n", path);
        return;
    }

    if (tig_file_fwrite(&wmap_ui_num_town_notes, sizeof(wmap_ui_num_town_notes), 1, stream) == 1) {
        if (tig_file_fwrite(&(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_194), sizeof(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_194), 1, stream) == 1) {
            for (index = 0; index < wmap_ui_num_town_notes; index++) {
                if (!wmap_ui_town_note_save(&(wmap_ui_town_notes[index]), stream)) {
                    success = false;
                    break;
                }
            }
        } else {
            // FIXME: Should set error?
        }
    } else {
        success = false;
    }

    tig_file_fclose(stream);

    if (!success) {
        tig_debug_printf("wmap_ui_town_notes_save(): Error writing data!\n");
    }
}

// 0x560150
void wmap_ui_destroy()
{
    int mode;
    int tile;

    for (mode = 0; mode < WMAP_UI_MODE_COUNT; mode++) {
        wmap_ui_mode = mode;

        if ((wmap_ui_mode_info[mode].flags & 0x1) != 0) {
            if (tig_video_buffer_destroy(wmap_ui_mode_info[mode].video_buffer) == TIG_OK) {
                wmap_ui_mode_info[mode].flags &= ~0x1;
            } else {
                tig_debug_printf("WMapUI: Destroy: ERROR: Video Buffer Destroy FAILED!\n");
            }
        }

        for (tile = 0; tile < wmap_ui_mode_info[mode].num_tiles; tile++) {
            wmTileArtUnload(tile);
        }
    }

    wmap_ui_mode = WMAP_UI_MODE_WORLD;
    dword_66D87C = 0;
    wmap_ui_module_name[0] = '\0';

    if (wmap_ui_info_loaded) {
        mes_unload(wmap_ui_worldmap_info_mes_file);
    }

    wmap_ui_info_loaded = false;
}

// 0x560200
void wmap_ui_reset()
{
    int index;

    if (wmap_ui_initialized) {
        wmap_ui_close();

        for (index = 0; index < WMAP_UI_MODE_COUNT; index++) {
            if (wmap_ui_mode_info[index].num_notes != NULL) {
                *wmap_ui_mode_info[index].num_notes = 0;
            }
            wmap_ui_mode_info[index].field_198 = -1;
        }

        for (index = 0; index < WMAP_ROUTE_TYPE_COUNT; index++) {
            wmap_ui_routes[index].length = 0;
            wmap_ui_routes[index].field_3C4 = 1;
            wmap_ui_routes[index].field_3C8 = 1;
        }

        dword_5C9AD8 = -1;
        wmap_ui_mode = WMAP_UI_MODE_WORLD;
        wmap_ui_state = WMAP_UI_STATE_NORMAL;
        dword_65E968 = 0;
        wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length = 0;
        wmap_ui_encounter = false;
    }
}

// 0x560280
bool wmap_ui_save(TigFile* stream)
{
    int mode;
    int note;
    int num_notes;
    int cnt;

    if (stream == NULL) {
        return false;
    }

    for (mode = 0; mode < WMAP_UI_MODE_COUNT; mode++) {
        if ((wmap_ui_mode_info[mode].flags & 0x2) != 0) {
            if (wmap_ui_mode_info[mode].num_notes != NULL) {
                num_notes = *wmap_ui_mode_info[mode].num_notes;
            } else {
                num_notes = 0;
            }

            cnt = num_notes;
            for (note = 0; note < num_notes; note++) {
                if ((wmap_ui_mode_info[mode].notes[note].flags & 0x2) != 0) {
                    cnt--;
                }
            }

            if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
                return false;
            }

            for (note = 0; note < num_notes; note++) {
                if ((wmap_ui_mode_info[mode].notes[note].flags & 0x2) == 0) {
                    if (!wmap_ui_town_note_save(&(wmap_ui_mode_info[mode].notes[note]), stream)) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

// 0x560350
bool wmap_ui_town_note_save(WmapNote* note, TigFile* stream)
{
    int size;

    if (stream == NULL) return false;

    if (tig_file_fwrite(&(note->id), sizeof(note->id), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(note->flags), sizeof(note->flags), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(note->coords.x), sizeof(note->coords.x), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(note->coords.y), sizeof(note->coords.y), 1, stream) != 1) return false;
    if (!wmap_ui_rect_write(&(note->field_18), stream)) return false;
    if (tig_file_fwrite(&(note->type), sizeof(note->type), 1, stream) != 1) return false;

    size = (int)strlen(note->str);
    if (tig_file_fwrite(&size, sizeof(size), 1, stream) != 1) return false;
    if (tig_file_fwrite(note->str, 1, size, stream) != size) return false;

    return true;
}

// 0x560440
bool wmap_ui_rect_write(TigRect* rect, TigFile* stream)
{
    if (tig_file_fwrite(&(rect->x), sizeof(rect->x), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(rect->y), sizeof(rect->y), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(rect->width), sizeof(rect->width), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(rect->height), sizeof(rect->height), 1, stream) != 1) return false;

    return true;
}

// 0x5604B0
bool wmap_ui_load(GameLoadInfo* load_info)
{
    int mode;
    int cnt;
    int note;
    int64_t loc;
    int64_t sector_id;

    if (load_info->stream == NULL) {
        return false;
    }

    for (mode = 0; mode < WMAP_UI_MODE_COUNT; mode++) {
        if ((wmap_ui_mode_info[mode].flags & 0x2) != 0) {
            tig_debug_printf("WMapUI: Reading Saved Notes.\n");

            if (tig_file_fread(&cnt, sizeof(cnt), 1, load_info->stream) != 1) {
                return false;
            }

            tig_debug_printf("WMapUI: Reading Saved Notes: %d.\n", cnt);

            if (wmap_ui_mode_info[mode].num_notes == NULL) {
                tig_debug_printf("WMapUI: Load: ERROR: Note Data Doesn't Match!\n");
                exit(EXIT_FAILURE);
            }

            *wmap_ui_mode_info[mode].num_notes = cnt;

            for (note = 0; note < cnt; note++) {
                tig_debug_printf("WMapUI: Reading Individual Note: %d.\n", note);

                if (!wmap_ui_town_note_load(&(wmap_ui_mode_info[mode].notes[note]), load_info->stream)) {
                    return false;
                }
            }
        }
    }

    loc = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    sector_id = sector_id_from_loc(loc);
    wmap_ui_notify_sector_changed(player_get_local_pc_obj(), sector_id);

    return true;
}

// 0x5605C0
bool wmap_ui_town_note_load(WmapNote* note, TigFile* stream)
{
    int size;

    if (stream == NULL) return false;

    if (tig_file_fread(&(note->id), sizeof(note->id), 1, stream) != 1) return false;
    if (tig_file_fread(&(note->flags), sizeof(note->flags), 1, stream) != 1) return false;
    if (tig_file_fread(&(note->coords.x), sizeof(note->coords.x), 1, stream) != 1) return false;
    if (tig_file_fread(&(note->coords.y), sizeof(note->coords.y), 1, stream) != 1) return false;
    if (!wmap_ui_rect_read(&(note->field_18), stream)) return false;
    if (tig_file_fread(&(note->type), sizeof(note->type), 1, stream) != 1) return false;
    if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) return false;

    memset(note->str, 0, sizeof(note->str));
    if (tig_file_fread(note->str, 1, size, stream) != size) return false;

    return true;
}

// 0x5606B0
bool wmap_ui_rect_read(TigRect* rect, TigFile* stream)
{
    if (tig_file_fread(&(rect->x), sizeof(rect->x), 1, stream) != 1) return false;
    if (tig_file_fread(&(rect->y), sizeof(rect->y), 1, stream) != 1) return false;
    if (tig_file_fread(&(rect->width), sizeof(rect->width), 1, stream) != 1) return false;
    if (tig_file_fread(&(rect->height), sizeof(rect->height), 1, stream) != 1) return false;

    return true;
}

// 0x560720
void wmap_ui_encounter_start()
{
    wmap_ui_encounter = true;
    anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_END_RANDOM_ENCOUNTER, 0, 4000);
}

// 0x560740
bool wmap_ui_is_encounter()
{
    return wmap_ui_encounter;
}

// 0x560750
void wmap_ui_encounter_end()
{
    wmap_ui_encounter = false;
}

// 0x560760
void wmap_ui_open()
{
    wmap_ui_spell = -1;
    wmap_ui_selecting = false;
    wmap_ui_obj = OBJ_HANDLE_NULL;
    wmap_ui_open_internal();
}

// 0x560790
void wmap_ui_select(int64_t obj, int spell)
{
    wmap_ui_obj = obj;
    dword_65E968 = 0;
    wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length = 0;
    wmap_ui_selecting = true;
    dword_66D880 = 0;
    wmap_ui_spell = spell;
    wmap_ui_open_internal();
}

// 0x5607E0
void wmap_ui_open_internal()
{
    int64_t pc_obj;
    int64_t loc;
    int64_t sector_id;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    if (wmap_ui_created) {
        wmap_ui_close();
        return;
    }

    pc_obj = player_get_local_pc_obj();

    if (pc_obj == OBJ_HANDLE_NULL
        || critter_is_dead(pc_obj)
        || critter_is_unconscious(pc_obj)) {
        return;
    }

    if (wmap_ui_is_encounter()) {
        mes_file_entry.num = 602; // "You cannot access the World Map during an encounter."
        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_FEEDBACK;
        ui_message.str = mes_file_entry.str;
        sub_550750(&ui_message);

        return;
    }

    loc = obj_field_int64_get(pc_obj, OBJ_F_LOCATION);
    sector_id = sector_id_from_loc(loc);
    if (sector_is_blocked(sector_id)) {
        mes_file_entry.num = 605; // "The World Map is not available."
        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_FEEDBACK;
        ui_message.str = mes_file_entry.str;
        sub_550750(&ui_message);

        return;
    }

    if (combat_critter_is_combat_mode_active(pc_obj)) {
        if (!combat_can_exit_combat_mode(pc_obj)) {
            mes_file_entry.num = 600; // "You cannot access the World Map during combat."
            mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);

            return;
        }
        combat_critter_deactivate_combat_mode(pc_obj);
    }

    if (!wmap_load_worldmap_info()) {
        if (wmap_ui_selecting) {
            sub_452650(pc_obj);
            return;
        }

        mes_file_entry.num = 605; // "The World Map is not available."
        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_FEEDBACK;
        ui_message.str = mes_file_entry.str;
        sub_550750(&ui_message);

        return;
    }

    if (wmap_ui_selecting) {
        wmap_ui_townmap = TOWNMAP_NONE;
    } else {
        sub_560EF0();
    }

    if (wmap_ui_created) {
        wmap_ui_close();
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_WMAP)) {
        return;
    }

    sub_424070(player_get_local_pc_obj(), 4, false, true);

    if (!wmap_ui_create()) {
        wmap_ui_close();
        intgame_mode_set(INTGAME_MODE_MAIN);

        if (wmap_ui_selecting) {
            sub_452650(pc_obj);
            return;
        }

        mes_file_entry.num = 605; // "The World Map is not available."
        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_FEEDBACK;
        ui_message.str = mes_file_entry.str;
        sub_550750(&ui_message);

        return;
    }

    if (wmap_ui_townmap != TOWNMAP_NONE) {
        wmap_ui_mode_set(WMAP_UI_MODE_TOWN);
    } else if (tig_net_is_active()) {
        wmap_ui_close();
    }

    wmap_ui_mode_info[wmap_ui_mode].refresh();

    ui_toggle_primary_button(UI_PRIMARY_BUTTON_WORLDMAP, false);
}

// 0x560AA0
bool wmap_load_worldmap_info()
{
    const char* name;
    char path[TIG_MAX_PATH];
    MesFileEntry mes_file_entry;
    char* str;
    int worldmap;
    int idx;
    WmapTile* v1;
    WmapInfo* wmap_info;
    int map_keyed_to;

    name = gamelib_current_module_name_get();
    if (*name == '\0') {
        return false;
    }

    if (strcmp(wmap_ui_module_name, name) == 0) {
        return false;
    }

    strcpy(wmap_ui_module_name, name);

    tig_str_parse_set_separator(',');

    sprintf(path, "WorldMap\\WorldMap.mes");
    if (mes_load(path, &wmap_ui_worldmap_info_mes_file)) {
        mes_file_entry.num = 20;
        mes_get_msg(wmap_ui_worldmap_info_mes_file, &mes_file_entry);
        str = mes_file_entry.str;
        tig_str_parse_value(&str, &wmap_ui_offset_x);
        tig_str_parse_value(&str, &wmap_ui_offset_y);

        mes_file_entry.num = 50;
        if (map_get_worldmap(map_current_map(), &worldmap) && worldmap != -1) {
            mes_file_entry.num += worldmap;
        }
        mes_get_msg(wmap_ui_worldmap_info_mes_file, &mes_file_entry);
        str = mes_file_entry.str;
        tig_str_parse_value(&str, &wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles);
        tig_str_parse_value(&str, &wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles);

        if (wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles == NULL) {
            wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_tiles = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles;
            wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles = (WmapTile*)CALLOC(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_tiles, sizeof(WmapTile));

            // FIXME: Meaningless, calloc already zeroes it out.
            for (idx = 0; idx < wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_tiles; idx++) {
                wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles[idx].flags = 0;
            }
        }

        tig_str_parse_str_value(&str, wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_68);

        if (!wmTileArtLock(0)) {
            tig_debug_printf("wmap_load_worldmap_info: ERROR: wmTileArtLockMode failed!\n");
            exit(EXIT_FAILURE);
        }

        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles->rect.width;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles->rect.height;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].map_width = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].map_height = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles;
        wmTileArtUnlock(0);
        wmTileArtUnload(0);

        tig_str_parse_named_str_value(&str, "ZoomedName:", wmap_ui_mode_info[WMAP_UI_MODE_WORLD].str);

        dword_66D87C = 0;
        if (tig_str_parse_named_value(&str, "MapKeyedTo:", &map_keyed_to)) {
            dword_66D87C = map_keyed_to;
        }

        for (idx = 0; idx < wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_tiles; idx++) {
            v1 = &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles[idx]);
            if ((v1->flags & 0x02) == 0) {
                v1->flags = 0x02;
                v1->rect.width = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width;
                v1->rect.height = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height;
                v1->field_18 = NULL;
                v1->field_1C = 0;
            }
        }
    } else {
        wmap_ui_offset_x = 0;
        wmap_ui_offset_y = 0;

        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles = 8;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles = 8;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width = 250;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height = 250;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_68[0] = '\0';
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].map_width = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].map_height = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles;
        wmap_ui_mode_info[WMAP_UI_MODE_WORLD].str[0] = '\0';
    }

    for (idx = 0; idx < WMAP_UI_MODE_COUNT; idx++) {
        wmap_info = &(wmap_ui_mode_info[idx]);
        if (wmap_info->rect.width > 0) {
            wmap_info->field_24 = wmap_info->rect.width / 2;
            wmap_info->field_28 = wmap_info->rect.height / 2;
            wmap_info->field_2C = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width - wmap_info->field_24;
            wmap_info->field_30 = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles * wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height - wmap_info->field_28;
        }

        if (wmap_info->num_notes != NULL && idx != 2) {
            *wmap_info->num_notes = 0;
        }

        wmap_info->field_198 = -1;
    }

    dword_5C9AD8 = -1;
    wmap_ui_note_width = wmap_note_type_info[WMAP_NOTE_TYPE_LOC].width;
    wmap_ui_note_height = wmap_note_type_info[WMAP_NOTE_TYPE_LOC].height;
    qword_66D850 = 320;
    sub_560EE0();

    wmap_ui_info_loaded = true;

    return true;
}

// 0x560EE0
void sub_560EE0()
{
}

// 0x560EF0
void sub_560EF0()
{
    dword_66D880 = 0;
    wmap_ui_townmap = townmap_get(sector_id_from_loc(obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION)));
    if (map_current_map() == dword_66D87C) {
        if (wmap_ui_townmap == TOWNMAP_NONE) {
            dword_66D880 = 1;
        }
    }
}

// 0x560F40
void wmap_ui_close()
{
    int64_t pc_obj;
    int index;

    if (!wmap_ui_created) {
        return;
    }

    pc_obj = player_get_local_pc_obj();
    if (pc_obj != OBJ_HANDLE_NULL
        && (obj_field_int32_get(pc_obj, OBJ_F_FLAGS) & OF_OFF) != 0) {
        critter_toggle_on_off(pc_obj);
        sub_4AA580(pc_obj);
    }

    if (intgame_mode_set(INTGAME_MODE_MAIN) && wmap_ui_created) {
        sub_564070(false);
        wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
        intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
        ambient_lighting_enable();
        intgame_button_destroy(&wmap_ui_navigate_button_info);
        intgame_big_window_unlock();

        wmap_ui_window = TIG_WINDOW_HANDLE_INVALID;
        for (index = 0; index < WMAP_NOTE_TYPE_COUNT; index++) {
            tig_font_destroy(wmap_note_type_info[index].font);
        }
        tig_video_buffer_destroy(dword_64E7F4);
        wmap_ui_created = 0;
        wmap_ui_obj = OBJ_HANDLE_NULL;
        wmap_ui_spell = -1;
        wmap_ui_destroy();
        sub_560010();
    }
}

// 0x560F90
int wmap_ui_is_created()
{
    return wmap_ui_created;
}

// 0x560FA0
bool wmap_ui_create()
{
    TigVideoBufferCreateInfo vb_create_info;
    TigArtAnimData art_anim_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigFont font_desc;
    int64_t loc;
    tig_art_id_t art_id;
    int index;
    WmapUiMode mode;

    if (wmap_ui_created) {
        return true;
    }

    if (map_current_map() != map_by_type(MAP_TYPE_START_MAP)) {
        int area;

        if (!map_get_area(map_current_map(), &area)) {
            tig_debug_printf("WMapUI: wmap_ui_create: ERROR: Failed to find matching WorldMap Position/area!\n");
            return false;
        }

        if (area == -1) {
            return false;
        }

        loc = area_get_location(area);
    } else {
        loc = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    }

    if (!wmap_load_worldmap_info()) {
        if (wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_68[0] == '\0'
            && wmap_ui_townmap == TOWNMAP_NONE) {
            MesFileEntry mes_file_entry;
            UiMessage ui_message;

            mes_file_entry.num = 605;
            mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
            return false;
        }

        if (is_default_wmap()) {
            wmap_ui_mode_info[wmap_ui_mode].field_60 = 2000 - wmap_ui_mode_info[wmap_ui_mode].rect.width;
            wmap_ui_mode_info[wmap_ui_mode].field_64 = 2000 - wmap_ui_mode_info[wmap_ui_mode].rect.height;
        } else {
            int64_t limit_x;
            int64_t limit_y;

            location_limits_get(&limit_x, &limit_y);
            wmap_ui_mode_info[wmap_ui_mode].field_60 = (int)(limit_x / 64) - wmap_ui_mode_info[wmap_ui_mode].rect.width;
            wmap_ui_mode_info[wmap_ui_mode].field_64 = (int)(limit_y / 64) - wmap_ui_mode_info[wmap_ui_mode].rect.height;
        }

        if (is_default_wmap()) {
            dword_66D6F8 = 2000;
        } else {
            int64_t limit_x;
            int64_t limit_y;

            location_limits_get(&limit_x, &limit_y);
            dword_66D6F8 = (int)(limit_x / 64);
        }

        if (dword_66D9CC == 0) {
            dword_66D9CC = 1;
        }
    }

    wmap_ui_textedit_focused = false;

    tig_art_interface_id_create(138, 0, 0, 0, &art_id);

    if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        tig_debug_printf("wmap_ui_create: ERROR: tig_art_anim_data failed!\n");
        exit(EXIT_SUCCESS); // FIXME: Should be EXIT_FAILURE.
    }

    src_rect.x = wmap_ui_window_frame.x;
    src_rect.y = 0;
    src_rect.width = wmap_ui_window_frame.width;
    src_rect.height = wmap_ui_window_frame.height;

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = wmap_ui_window_frame.width;
    dst_rect.height = wmap_ui_window_frame.height;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    if (!intgame_big_window_lock(wmap_ui_message_filter, &wmap_ui_window)) {
        // FIXME: Message is misleading.
        tig_debug_printf("wmap_ui_create: ERROR: tig_art_anim_data failed!\n");
        exit(EXIT_SUCCESS); // FIXME: Should be EXIT_FAILURE.
    }

    tig_window_blit_art(wmap_ui_window, &art_blit_info);

    font_desc.flags = TIG_FONT_NO_ALPHA_BLEND | TIG_FONT_SHADOW;
    tig_art_interface_id_create(840, 0, 0, 0, &(font_desc.art_id));

    for (index = 0; index < WMAP_NOTE_TYPE_COUNT; index++) {
        font_desc.color = wmap_note_type_info[index].color;
        tig_font_create(&font_desc, &(wmap_note_type_info[index].font));
    }

    for (index = 0; index < 2; index++) {
        intgame_button_create_ex(wmap_ui_window,
            &wmap_ui_window_frame,
            &(wmap_ui_zoom_button_info[index]),
            TIG_BUTTON_MOMENTARY);
    }

    intgame_button_create_ex(wmap_ui_window,
        &wmap_ui_window_frame,
        &wmap_ui_navigate_button_info,
        TIG_BUTTON_LATCH);
    intgame_button_create_ex(wmap_ui_window,
        &wmap_ui_window_frame,
        &wmap_ui_travel_button_info,
        wmap_ui_travel_button_flags | TIG_BUTTON_HIDDEN);

    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    vb_create_info.width = wmap_note_type_icon_max_width + 203;
    vb_create_info.height = 50;
    vb_create_info.background_color = dword_64E03C;
    vb_create_info.color_key = dword_64E03C;

    if (tig_video_buffer_create(&vb_create_info, &dword_64E7F4) != TIG_OK) {
        tig_debug_printf("wmap_ui_create: ERROR: tig_video_buffer_create failed!\n");
        exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
    }

    wmap_ui_pc_lens.window_handle = wmap_ui_window;
    wmap_ui_pc_lens.rect = &wmap_ui_pc_lens_frame;
    tig_art_interface_id_create(198, 0, 0, 0, &(wmap_ui_pc_lens.art_id));
    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &wmap_ui_pc_lens);

    for (index = area_count() - 1; index > 0; index--) {
        if (area_is_known(player_get_local_pc_obj(), index)) {
            sub_562800(index);
        }
    }

    sub_561430(loc);
    sub_563590(&(wmap_ui_mode_info[wmap_ui_mode].field_3C), false);

    mode = wmap_ui_mode;
    wmap_ui_mode = -1;
    wmap_ui_mode_set(mode);

    dword_5C9B18 = -1;
    wmap_ui_compass_arrow_frame_set(1);
    wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].field_3C8 = dword_66D880 != 0;
    wmap_ui_created = true;

    return true;
}

// 0x561430
void sub_561430(int64_t location)
{
    WmapCoords coords;
    int x;
    int y;

    sub_561490(location, &coords);

    x = coords.x;
    if (x > 99999 || x == 0) {
        x = 200;
    }

    y = coords.y;
    if (y > 99999 || y == 0) {
        y = 180;
    }

    sub_5614C0(x, y);
}

// 0x561490
void sub_561490(int64_t location, WmapCoords* coords)
{
    coords->x = dword_66D6F8 - ((location >> 6) & 0x3FFFFFF);
    coords->y = (location >> 32) >> 6;
}

// 0x5614C0
void sub_5614C0(int x, int y)
{
    wmap_ui_mode_info[wmap_ui_mode].field_3C.x = x;
    wmap_ui_mode_info[wmap_ui_mode].field_3C.y = y;
}

// 0x5614F0
bool is_default_wmap()
{
    if (wmap_ui_mode != WMAP_UI_MODE_WORLD) {
        return false;
    }

    if (map_current_map() == map_by_type(MAP_TYPE_START_MAP)) {
        return false;
    }

    return true;
}

// 0x5615D0
bool wmap_ui_state_set(WmapUiState state)
{
    bool refresh = false;
    WmapInfo* wmap_info;
    tig_art_id_t art_id;
    int64_t pc_obj;
    int64_t loc;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);
    if (wmap_ui_state != state) {
        switch (state) {
        case WMAP_UI_STATE_NORMAL:
            wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].field_3C8 = dword_66D880 != 0;
            break;
        case WMAP_UI_STATE_MOVING:
            if (!sub_565140()) {
                return false;
            }

            intgame_pc_lens_do(PC_LENS_MODE_BLACKOUT, &wmap_ui_pc_lens);
            ambient_lighting_disable();
            pc_obj = player_get_local_pc_obj();
            if (pc_obj != OBJ_HANDLE_NULL
                && (obj_field_int32_get(pc_obj, OBJ_F_FLAGS) & OF_OFF) == 0) {
                critter_toggle_on_off(pc_obj);
                ai_stop_attacking(pc_obj);
            }
            gsound_stop_all(25);
            wmap_ui_music_handle = gsound_play_music_indefinitely("sound\\music\\arcanum.mp3", 25);
            break;
        case WMAP_UI_STATE_EDITING:
            dword_5C9AD8 = -1;
            tig_art_interface_id_create(21, 0, 0, 0, &art_id);
            tig_mouse_cursor_set_art_id(art_id);
            refresh = true;
            break;
        }

        switch (wmap_ui_state) {
        case WMAP_UI_STATE_1:
            sub_564940();
            refresh = true;
            break;
        case WMAP_UI_STATE_MOVING:
            tig_sound_destroy(wmap_ui_music_handle);
            pc_obj = player_get_local_pc_obj();
            if (pc_obj != OBJ_HANDLE_NULL
                && (obj_field_int32_get(pc_obj, OBJ_F_FLAGS) & OF_OFF) != 0) {
                critter_toggle_on_off(pc_obj);
                sub_4AA580(pc_obj);
            }
            timeevent_clear_all_typed(TIMEEVENT_TYPE_WORLDMAP);
            wmap_ui_state = state;
            intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &wmap_ui_pc_lens);
            sub_561800(&(wmap_info->field_3C), &loc);
            wmap_ui_teleport(loc);
            sub_5649F0(loc);
            break;
        case WMAP_UI_STATE_EDITING:
            sub_564070(1);
            tig_art_interface_id_create(0, 0, 0, 0, &art_id);
            tig_mouse_cursor_set_art_id(art_id);
            break;
        case WMAP_UI_STATE_NAVIGATING:
            tig_button_state_change(wmap_ui_navigate_button_info.button_handle, TIG_BUTTON_STATE_RELEASED);
            break;
        }

        wmap_ui_state = state;

        if (refresh) {
            sub_5649C0();
            wmap_ui_mode_info[wmap_ui_mode].refresh();
        }
    }

    return true;
}

// 0x561800
void sub_561800(WmapCoords* coords, int64_t* loc_ptr)
{
    int x;
    int y;

    x = ((dword_66D6F8 - coords->x) << 6) + 32;
    y = (coords->y << 6) + 32;
    *loc_ptr = location_make(x, y);
}

// 0x561860
bool wmap_ui_teleport(int64_t loc)
{
    TeleportData teleport_data;

    sector_flush(0);

    teleport_data.flags = TELEPORT_0x0020;
    teleport_data.obj = player_get_local_pc_obj();
    teleport_data.loc = loc;
    teleport_data.map = map_by_type(MAP_TYPE_START_MAP);
    return teleport_do(&teleport_data);
}

// TODO: Review, split.
//
// 0x5618B0
bool wmap_ui_message_filter(TigMessage* msg)
{
    WmapInfo* wmap_info;
    MesFileEntry mes_file_entry;
    char str[48];
    UiMessage ui_message;
    bool inside = false;

    // Convert mouse position from screen coordinate system to centered 800x600
    // area.
    if (msg->type == TIG_MESSAGE_MOUSE) {
        TigRect rect = { 0, 0, 800, 600 };
        hrp_apply(&rect, GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);
        msg->data.mouse.x -= rect.x;
        msg->data.mouse.y -= rect.y;
    }

    wmap_ui_handle_scroll();

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    if (msg->type == TIG_MESSAGE_MOUSE) {
        switch (msg->data.mouse.event) {
        case TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN:
            if (msg->data.mouse.x < wmap_info->field_14.x
                || msg->data.mouse.y < wmap_info->field_14.y
                || msg->data.mouse.x >= wmap_info->field_14.x + wmap_info->field_14.width
                || msg->data.mouse.y >= wmap_info->field_14.y + wmap_info->field_14.height) {
                if (wmap_ui_state == WMAP_UI_STATE_NAVIGATING) {
                    wmap_ui_navigate(msg->data.mouse.x, msg->data.mouse.y);
                    return true;
                }
                return false;
            }

            if (wmap_info->field_2BC != NULL) {
                wmap_info->field_2BC(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);
            }

            switch (wmap_ui_mode) {
            case WMAP_UI_MODE_WORLD:
                switch (wmap_ui_state) {
                case WMAP_UI_STATE_NORMAL: {
                    if (wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].field_3C8) {
                        if (sub_564780(&stru_64E7E8, &dword_5C9AD8)) {
                            sub_5649D0(wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[dword_5C9AD8].field_4);
                            wmap_ui_state_set(WMAP_UI_STATE_1);
                            wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].field_3C8 = 0;
                            return true;
                        }

                        if (sub_5627B0(&stru_64E7E8) && sub_5643E0(&stru_64E7E8)) {
                            dword_5C9AD8 = wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length - 1;
                            wmap_ui_state_set(WMAP_UI_STATE_1);
                            wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].field_3C8 = 0;
                            return true;
                        }

                        return true;
                    }

                    if (!wmap_ui_selecting) {
                        return true;
                    }

                    wmap_info->field_2BC(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);

                    int64_t loc;
                    sub_561800(&stru_64E7E8, &loc);

                    int area = area_get_nearest_known_area(loc, player_get_local_pc_obj(), qword_66D850);
                    if (area <= 0) {
                        return true;
                    }

                    loc = area_get_location(area);
                    if (loc == 0 || wmap_ui_spell == -1) {
                        return true;
                    }

                    int64_t pc_obj = player_get_local_pc_obj();
                    if (antiteleport_check(pc_obj, loc)) {
                        if (player_is_local_pc_obj(wmap_ui_obj)) {
                            sub_4507B0(wmap_ui_obj, wmap_ui_spell);
                        }

                        if (area_get_last_known_area(pc_obj) == area) {
                            area_reset_last_known_area(pc_obj);
                        }

                        wmap_ui_teleport(loc);
                        wmap_ui_close();
                    } else {
                        mes_file_entry.num = 640;
                        mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
                        sub_460610(mes_file_entry.str);
                    }

                    return true;
                }
                case WMAP_UI_STATE_1:
                    if (!dword_66D880) {
                        return true;
                    }

                    sub_562AF0(msg->data.mouse.x, msg->data.mouse.y);

                    if (!sub_5627B0(&stru_64E7E8)) {
                        return true;
                    }

                    sub_564830(dword_5C9AD8, &stru_64E7E8);

                    return true;
                case WMAP_UI_STATE_EDITING: {
                    int id;
                    if (find_note_by_coords(&stru_64E7E8, &id)) {
                        sub_564360(id);
                    } else {
                        sub_563F90(&stru_64E7E8);
                    }
                    return true;
                }
                case WMAP_UI_STATE_NAVIGATING:
                    wmap_ui_navigate(msg->data.mouse.x, msg->data.mouse.y);
                    return true;
                default:
                    return true;
                }
            case WMAP_UI_MODE_TOWN:
                switch (wmap_ui_state) {
                case WMAP_UI_STATE_NORMAL:
                    if (wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].field_3C8) {
                        if (sub_564780(&stru_64E7E8, &dword_5C9AD8)) {
                            sub_5649D0(wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].waypoints[dword_5C9AD8].field_4);
                            wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].field_3C8 = 0;
                            return true;
                        }

                        if (sub_5627B0(&stru_64E7E8) && sub_5643E0(&stru_64E7E8)) {
                            dword_5C9AD8 = wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].length - 1;
                            wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].field_3C8 = 0;
                            return true;
                        }
                    }
                    return true;
                case WMAP_UI_STATE_NAVIGATING:
                    wmap_ui_navigate(msg->data.mouse.x, msg->data.mouse.y);
                    return true;
                default:
                    return true;
                }
            default:
                return true;
            }
        case TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP:
            switch (wmap_ui_mode) {
            case WMAP_UI_MODE_WORLD:
                wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].field_3C8 = dword_66D880 != 0;
                if (wmap_ui_state == WMAP_UI_STATE_1 || wmap_ui_state == WMAP_UI_STATE_NAVIGATING) {
                    wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
                }
                break;
            case WMAP_UI_MODE_CONTINENT:
                if (wmap_ui_townmap == TOWNMAP_NONE
                    && msg->data.mouse.x >= wmap_info->field_14.x
                    && msg->data.mouse.y >= wmap_info->field_14.y
                    && msg->data.mouse.x < wmap_info->field_14.x + wmap_info->field_14.width
                    && msg->data.mouse.y < wmap_info->field_14.y + wmap_info->field_14.height
                    && wmap_ui_state == WMAP_UI_STATE_NORMAL) {
                    sub_563B10(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);
                    wmap_ui_mode_set(WMAP_UI_MODE_WORLD);
                    sub_563590(&stru_64E7E8, 0);
                    sub_563270();
                }
                break;
            case WMAP_UI_MODE_TOWN:
                wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].field_3C8 = 1;
                if (wmap_ui_state == WMAP_UI_STATE_1 || wmap_ui_state == WMAP_UI_STATE_NAVIGATING) {
                    wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
                }
                break;
            }

            if (intgame_pc_lens_check_pt_unscale(msg->data.mouse.x, msg->data.mouse.y)) {
                wmap_ui_close();
                return true;
            }

            return false;
        case TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP: {
            if (wmap_ui_state == WMAP_UI_STATE_EDITING) {
                if (wmap_ui_mode == WMAP_UI_MODE_WORLD) {
                    wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
                    return true;
                }
                return false;
            }

            WmapRouteType route_type = wmap_ui_mode == WMAP_UI_MODE_TOWN
                ? WMAP_ROUTE_TYPE_TOWN
                : WMAP_ROUTE_TYPE_WORLD;
            if (dword_5C9AD8 == -1) {
                dword_5C9AD8 = wmap_ui_routes[route_type].length - 1;
            }

            if (dword_5C9AD8 >= 0) {
                sub_564840(dword_5C9AD8);
                if (dword_5C9AD8 >= wmap_ui_routes[route_type].length) {
                    dword_5C9AD8 = wmap_ui_routes[route_type].length - 1;
                }
            }

            if (wmap_ui_state == WMAP_UI_STATE_MOVING) {
                wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
            }

            return true;
        }
        case TIG_MESSAGE_MOUSE_MOVE:
            if (msg->data.mouse.x >= wmap_info->field_14.x
                && msg->data.mouse.y >= wmap_info->field_14.y
                && msg->data.mouse.x < wmap_info->field_14.x + wmap_info->field_14.width
                && msg->data.mouse.y < wmap_info->field_14.y + wmap_info->field_14.height) {
                if (wmap_ui_mode == WMAP_UI_MODE_WORLD || wmap_ui_mode == WMAP_UI_MODE_CONTINENT) {
                    wmap_info->field_2BC(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);
                }

                if (wmap_ui_mode != WMAP_UI_MODE_TOWN) {
                    wmap_ui_draw_coords(&stru_64E7E8);
                }

                inside = true;
            }

            switch (wmap_ui_mode) {
            case WMAP_UI_MODE_WORLD:
                switch (wmap_ui_state) {
                case WMAP_UI_STATE_NORMAL:
                    if (inside) {
                        wmap_info->field_2BC(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);

                        int id;
                        if (find_note_by_coords(&stru_64E7E8, &id)) {
                            if (wmap_info->field_198 != id) {
                                WmapNote* note = sub_563D90(id);
                                if (note->id < 200) {
                                    wmap_info->field_198 = id;
                                    sub_550770(-1, area_get_description(note->id));
                                }
                            }
                            return true;
                        }
                    }

                    if (wmap_info->field_198 != -1) {
                        wmap_info->field_198 = -1;
                        sub_550720();
                    }

                    return true;
                case WMAP_UI_STATE_1:
                    if (wmap_info) {
                        sub_562AF0(msg->data.mouse.x, msg->data.mouse.y);
                        if (sub_5627B0(&stru_64E7E8)) {
                            sub_564830(dword_5C9AD8, &stru_64E7E8);
                        }
                    }

                    return true;
                default:
                    return true;
                }
            case WMAP_UI_MODE_CONTINENT: {
                if (inside) {
                    wmap_info->field_2BC(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);

                    int id;
                    if (find_note_by_coords(&stru_64E7E8, &id)) {
                        if (wmap_info->field_198 != id) {
                            WmapNote* note = sub_563D90(id);
                            if (note->id < 200) {
                                wmap_info->field_198 = id;
                                sub_550770(-1, area_get_description(note->id));
                            }
                        }
                    } else {
                        if (wmap_info->field_198 != -1) {
                            wmap_info->field_198 = -1;
                            sub_550720();
                        }
                    }
                }

                return true;
            }
            case WMAP_UI_MODE_TOWN:
                switch (wmap_ui_state) {
                case WMAP_UI_STATE_NORMAL:
                    if (inside) {
                        wmap_info->field_2BC(msg->data.mouse.x, msg->data.mouse.y, &stru_64E7E8);

                        int id;
                        if (find_note_by_coords(&stru_64E7E8, &id)) {
                            if (wmap_info->field_198 != id) {
                                WmapNote* note = sub_563D90(id);
                                sub_550770(-1, note->str);
                            }
                        }
                    } else {
                        if (wmap_info->field_198 != -1) {
                            wmap_info->field_198 = -1;
                            sub_550720();
                        }
                    }

                    return true;
                case WMAP_UI_STATE_1:
                    if (inside) {
                        sub_562AF0(msg->data.mouse.x, msg->data.mouse.y);
                        if (!sub_5627B0(&stru_64E7E8)) {
                            return true;
                        }
                    }

                    return true;
                default:
                    return true;
                }
            default:
                return true;
            }
        }
    }

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_PRESSED:
            if (msg->data.button.button_handle == wmap_ui_navigate_button_info.button_handle) {
                if (wmap_ui_state != WMAP_UI_STATE_MOVING && wmap_ui_state_set(WMAP_UI_STATE_NAVIGATING)) {
                    wmap_ui_navigate(msg->data.button.x, msg->data.button.y);
                }
                return true;
            }
            return false;
        case TIG_BUTTON_STATE_RELEASED:
            if (msg->data.button.button_handle == wmap_ui_zoom_button_info[0].button_handle) {
                if (wmap_ui_townmap == TOWNMAP_NONE) {
                    wmap_ui_mode_set(WMAP_UI_MODE_WORLD);
                    return true;
                }

                if (!wmap_ui_selecting) {
                    wmap_ui_mode_set(WMAP_UI_MODE_TOWN);
                    return true;
                }

                return true;
            }

            if (msg->data.button.button_handle == wmap_ui_zoom_button_info[1].button_handle) {
                wmap_ui_mode_set(WMAP_UI_MODE_CONTINENT);
                return true;
            }

            if (msg->data.button.button_handle == wmap_ui_navigate_button_info.button_handle) {
                if (wmap_info->navigatable && !wmap_ui_navigating) {
                    sub_563590(&(wmap_info->field_3C), true);
                } else {
                    wmap_ui_navigating = false;
                }
                return true;
            }

            if (msg->data.button.button_handle == wmap_ui_travel_button_info.button_handle) {
                if (wmap_ui_state == WMAP_UI_STATE_EDITING) {
                    wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
                }

                if (wmap_ui_state == WMAP_UI_STATE_NORMAL) {
                    if (dword_66D880) {
                        WmapRouteType route_type = wmap_ui_mode == WMAP_UI_MODE_TOWN
                            ? WMAP_ROUTE_TYPE_TOWN
                            : WMAP_ROUTE_TYPE_WORLD;
                        if (wmap_ui_routes[route_type].length > 0) {
                            dword_5C9AD8 = -1;

                            if (wmap_ui_state_set(WMAP_UI_STATE_MOVING)) {
                                TimeEvent timeevent;
                                DateTime datetime;

                                timeevent.type = TIMEEVENT_TYPE_WORLDMAP;
                                sub_45A950(&datetime, 50);
                                timeevent_add_delay(&timeevent, &datetime);

                                return true;
                            }
                        }
                    } else {
                        if (wmap_ui_mode == WMAP_UI_MODE_TOWN && wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].length > 0) {
                            sub_433640(player_get_local_pc_obj(),
                                wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].waypoints[0].loc);

                            for (int idx = 1; idx < wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].length; idx++) {
                                sub_433A00(player_get_local_pc_obj(),
                                    wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].waypoints[idx].loc,
                                    tig_net_is_active()
                                        && !tig_net_is_host());
                            }

                            sub_436D20(0x80000, 0);
                            wmap_ui_close();
                        }
                    }
                    return true;
                } else if (wmap_ui_state == WMAP_UI_STATE_MOVING) {
                    wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
                }

                return true;
            }

            return false;
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            mes_file_entry.num = -1;

            if (msg->data.button.button_handle == wmap_ui_zoom_button_info[0].button_handle) {
                mes_file_entry.num = 501; // "Map View"
            } else if (msg->data.button.button_handle == wmap_ui_zoom_button_info[1].button_handle) {
                mes_file_entry.num = 502; // "Continent View"
            } else if (msg->data.button.button_handle == wmap_ui_navigate_button_info.button_handle) {
                mes_file_entry.num = 510; // "Scroll Map"
            } else if (msg->data.button.button_handle == wmap_ui_travel_button_info.button_handle) {
                mes_file_entry.num = 512; // "Toggle Walking Waypoint Path"
            } else {
                return false;
            }

            if (mes_search(wmap_ui_worldmap_mes_file, &mes_file_entry)) {
                mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
                sprintf(str, "%s\n%s", wmap_ui_action, mes_file_entry.str);
                ui_message.type = UI_MSG_TYPE_FEEDBACK;
                ui_message.str = str;
                sub_550750(&ui_message);
            } else {
                tig_debug_printf("WmapUI: ERROR: Hover Text for button is Unreachable!\n");
            }
            return true;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            mes_file_entry.num = -1;

            if (msg->data.button.button_handle == wmap_ui_zoom_button_info[0].button_handle) {
                mes_file_entry.num = 501;
                sub_550720();
                return true;
            }

            if (msg->data.button.button_handle == wmap_ui_zoom_button_info[1].button_handle) {
                mes_file_entry.num = 502;
                sub_550720();
                return true;
            }

            if (msg->data.button.button_handle == wmap_ui_navigate_button_info.button_handle) {
                mes_file_entry.num = 510;
                sub_550720();
                return true;
            }

            if (msg->data.button.button_handle == wmap_ui_travel_button_info.button_handle) {
                mes_file_entry.num = 512;
                sub_550720();
                return true;
            }

            return false;
        default:
            return false;
        }
    case TIG_MESSAGE_TEXT_INPUT:
        return textedit_ui_process_message(msg);
    case TIG_MESSAGE_KEYBOARD:
        if (wmap_ui_textedit_focused) {
            return textedit_ui_process_message(msg);
        }

        switch (msg->data.keyboard.scancode) {
        case SDL_SCANCODE_Z:
            if (msg->data.keyboard.pressed) {
                return false;
            }

            if (wmap_ui_state == WMAP_UI_STATE_MOVING) {
                wmap_ui_mode_set(WMAP_UI_MODE_TOWN);
                return true;
            }

            gsound_play_sfx(0, 1);

            if (wmap_ui_mode != 1) {
                wmap_ui_mode_set(WMAP_UI_MODE_CONTINENT);
                return true;
            }

            if (wmap_ui_townmap == TOWNMAP_NONE) {
                wmap_ui_mode_set(WMAP_UI_MODE_WORLD);
                return true;
            }

            return true;
        case SDL_SCANCODE_HOME:
            if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
                gsound_play_sfx(0, 1);
                sub_563590(&(wmap_info->field_3C), true);
            }
            return true;
        case SDL_SCANCODE_LEFT:
            if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
                gsound_play_sfx(0, 1);
                wmap_ui_scroll_with_kb(6);
            }
            return true;
        case SDL_SCANCODE_UP:
            if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
                gsound_play_sfx(0, 1);
                wmap_ui_scroll_with_kb(0);
            }
            return true;
        case SDL_SCANCODE_RIGHT:
            if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
                gsound_play_sfx(0, 1);
                wmap_ui_scroll_with_kb(2);
            }
            return true;
        case SDL_SCANCODE_DOWN:
            if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
                gsound_play_sfx(0, 1);
                wmap_ui_scroll_with_kb(4);
            }
            return true;
        case SDL_SCANCODE_DELETE:
            if (msg->data.keyboard.pressed) {
                return false;
            }

            // TODO: Incomplete.
            return true;
        case SDL_SCANCODE_D:
            if (!msg->data.keyboard.pressed
                && gamelib_cheat_level_get() >= 3
                && wmap_ui_mode == WMAP_UI_MODE_WORLD) {
                gsound_play_sfx(0, 1);

                for (int area = area_count() - 1; area > 0; area--) {
                    area_set_known(player_get_local_pc_obj(), area);
                    sub_562800(area);
                }

                wmap_info->refresh();
            }
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

// 0x5627B0
bool sub_5627B0(WmapCoords* coords)
{
    int64_t loc;

    if (wmap_ui_mode == WMAP_UI_MODE_TOWN) {
        return 1;
    }

    sub_561800(coords, &loc);
    return sub_5627F0(loc);
}

// 0x5627F0
bool sub_5627F0(int64_t loc)
{
    (void)loc;

    return true;
}

// 0x562800
void sub_562800(int id)
{
    WmapNote note;
    const char* name;

    note.id = id;
    note.flags = 0x2;
    note.field_10 = 0;
    note.type = WMAP_NOTE_TYPE_LOC;

    name = area_get_name(id);
    if (name != NULL) {
        strncpy(note.str, name, sizeof(note.str));
        sub_561490(area_get_location(id), &(note.coords));
        sub_563C60(&note);
    }
}

// 0x562880
void wmap_ui_draw_coords(WmapCoords* coords)
{
    int x;
    int y;
    int index;
    int64_t limit_x;
    int64_t limit_y;
    char str[32];
    TigRect src_rect;
    TigRect dst_rect;

    x = wmap_ui_offset_x + coords->x;
    y = wmap_ui_offset_y + coords->y;

    for (index = 0; index < 2; index++) {
        if (tig_video_buffer_fill(dword_64E7F4, &stru_66D708, dword_64E03C) == TIG_OK) {
            tig_window_fill(wmap_ui_window,
                &wmap_ui_lat_long_frame[index],
                tig_color_make(0, 0, 0));

            if (is_default_wmap()) {
                limit_x = 2000;
            } else {
                location_limits_get(&limit_x, &limit_y);
                limit_x /= 64;
            }

            if (index == 0) {
                sprintf(str, "%d W", (int)(limit_x - x));
            } else {
                sprintf(str, "%d S", y);
            }

            tig_font_push(wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].font);
            if (tig_font_write(dword_64E7F4, str, &stru_66D708, &dst_rect) == TIG_OK) {
                dst_rect.x = wmap_ui_lat_long_frame[index].x;
                dst_rect.y = wmap_ui_lat_long_frame[index].y;

                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.width = dst_rect.width;
                src_rect.height = dst_rect.height;

                tig_window_copy_from_vbuffer(wmap_ui_window,
                    &dst_rect,
                    dword_64E7F4,
                    &src_rect);
            }
            tig_font_pop();
        }
    }
}

// 0x562A20
void wmap_ui_navigate(int x, int y)
{
    if (!wmap_ui_mode_info[wmap_ui_mode].navigatable) {
        return;
    }

    if (x >= stru_5C9A88.x
        && y >= stru_5C9A88.y
        && x < stru_5C9A88.x + stru_5C9A88.width
        && y < stru_5C9A88.y + stru_5C9A88.height) {
        return;
    }

    wmap_ui_navigating = true;

    if (y < stru_5C9A88.y) {
        wmap_ui_scroll_internal(0, 8);
    } else if (y > stru_5C9A88.y + stru_5C9A88.height) {
        wmap_ui_scroll_internal(4, 8);
    }

    if (x < stru_5C9A88.x) {
        wmap_ui_scroll_internal(6, 8);
    } else if (x > stru_5C9A88.x + stru_5C9A88.width) {
        wmap_ui_scroll_internal(2, 8);
    }
}

// 0x562AF0
void sub_562AF0(int x, int y)
{
    if (y < wmap_ui_mode_info[wmap_ui_mode].field_14.y + 23) {
        wmap_ui_scroll_internal(0, 10);
    } else if (y > wmap_ui_mode_info[wmap_ui_mode].field_14.y + wmap_ui_mode_info[wmap_ui_mode].field_14.height - 23) {
        wmap_ui_scroll_internal(4, 10);
    }

    if (x < wmap_ui_mode_info[wmap_ui_mode].field_14.x + 23) {
        wmap_ui_scroll_internal(6, 10);
    } else if (x > wmap_ui_mode_info[wmap_ui_mode].field_14.x + wmap_ui_mode_info[wmap_ui_mode].field_14.width - 23) {
        wmap_ui_scroll_internal(2, 10);
    }
}

// 0x562B70
void wmap_ui_mode_set(WmapUiMode mode)
{
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    int v2;
    tig_art_id_t art_id;
    TigArtAnimData art_anim_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;

    wmap_ui_state_set(WMAP_UI_STATE_NORMAL);

    if (wmap_ui_mode == mode) {
        return;
    }

    switch (mode) {
    case WMAP_UI_MODE_CONTINENT:
        if (wmap_ui_mode_info[WMAP_UI_MODE_WORLD].str[0] == '\0') {
            mes_file_entry.num = 605;
            mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
            return;
        }
        break;
    case WMAP_UI_MODE_TOWN:
        if (wmap_ui_townmap == TOWNMAP_NONE) {
            return;
        }
        break;
    }

    wmap_ui_mode = mode;

    switch (mode) {
    case WMAP_UI_MODE_CONTINENT:
        if (wmap_ui_travel_button_info.button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_hide(wmap_ui_travel_button_info.button_handle);
        }
        break;
    case WMAP_UI_MODE_TOWN:
        if (wmap_ui_travel_button_info.button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_show(wmap_ui_travel_button_info.button_handle);
        }

        if (!wmap_load_townmap_info()) {
            wmap_ui_mode_set(WMAP_UI_MODE_WORLD);
            return;
        }
        break;
    default:
        if (dword_66D880) {
            if (wmap_ui_travel_button_info.button_handle != TIG_BUTTON_HANDLE_INVALID) {
                tig_button_show(wmap_ui_travel_button_info.button_handle);
            }
        } else {
            if (wmap_ui_travel_button_info.button_handle != TIG_BUTTON_HANDLE_INVALID) {
                tig_button_hide(wmap_ui_travel_button_info.button_handle);
            }
        }
        break;
    }

    if (wmap_ui_window != TIG_WINDOW_HANDLE_INVALID) {
        v2 = mode;
        if (!dword_66D880) {
            v2 = 2;
        }

        if (wmap_ui_mode_info[v2].field_54 != -1) {
            tig_art_interface_id_create(wmap_ui_mode_info[v2].field_54, 0, 0, 0, &art_id);

            if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK
                && tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.width = art_frame_data.width;
                src_rect.height = art_frame_data.height;

                dst_rect.x = stru_5C9A78.x;
                dst_rect.y = stru_5C9A78.y;
                dst_rect.width = art_frame_data.width;
                dst_rect.height = art_frame_data.height;

                art_blit_info.flags = 0;
                art_blit_info.art_id = art_id;
                art_blit_info.src_rect = &src_rect;
                art_blit_info.dst_rect = &dst_rect;
                tig_window_blit_art(wmap_ui_window, &art_blit_info);
            }
        }

        wmap_ui_mode_info[wmap_ui_mode].refresh();
        wmap_ui_draw_coords(&stru_64E7E8);
    }
}

// 0x562D80
bool wmap_load_townmap_info()
{
    WmapInfo* wmap_info;
    int x;
    int y;
    int index;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);
    wmap_ui_routes[WMAP_ROUTE_TYPE_TOWN].length = 0;

    if (!townmap_info(wmap_ui_townmap, &wmap_ui_tmi)) {
        wmap_ui_townmap = TOWNMAP_NONE;
        return false;
    }

    townmap_loc_to_coords(&wmap_ui_tmi,
        obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION),
        &x,
        &y);

    wmap_info->field_3C.x = x;
    wmap_info->field_3C.y = y;
    wmap_info->field_34 = 0;
    wmap_info->field_38 = 0;

    strcpy(wmap_info->field_68, townmap_name(wmap_ui_townmap));
    wmap_info->num_hor_tiles = wmap_ui_tmi.num_hor_tiles;
    wmap_info->num_vert_tiles = wmap_ui_tmi.num_vert_tiles;
    wmap_info->num_tiles = wmap_ui_tmi.num_hor_tiles * wmap_ui_tmi.num_vert_tiles;
    wmap_info->tiles = (WmapTile*)CALLOC(sizeof(*wmap_info->tiles), wmap_info->num_tiles);

    if (!wmTileArtLockMode(2, 0)) {
        tig_debug_printf("wmap_load_townmap_info: ERROR: wmTileArtLockMode failed!\n");
        exit(EXIT_FAILURE);
    }

    wmap_info->tile_width = wmap_info->tiles[0].rect.width;
    wmap_info->tile_height = wmap_info->tiles[0].rect.height;
    wmap_info->map_width = wmap_info->tile_width * wmap_info->num_hor_tiles;
    wmap_info->map_height = wmap_info->tile_height * wmap_info->num_vert_tiles;

    wmTileArtUnlockMode(2, 0);
    wmTileArtUnloadMode(2, 0);

    for (index = 0; index < wmap_info->num_tiles; index++) {
        sub_562F90(&(wmap_info->tiles[index]));
        wmap_info->tiles[index].rect.width = wmap_info->tile_width;
        wmap_info->tiles[index].rect.height = wmap_info->tile_height;
    }

    wmap_info->field_2C = wmap_info->num_hor_tiles * wmap_info->tile_width - wmap_info->field_24;
    wmap_info->field_60 = wmap_info->num_hor_tiles * wmap_info->tile_width - wmap_info->rect.width;
    wmap_info->field_30 = wmap_info->num_vert_tiles * wmap_info->tile_height - wmap_info->field_28;
    wmap_info->field_64 = wmap_info->num_vert_tiles * wmap_info->tile_height - wmap_info->rect.height;

    sub_563590(&(wmap_info->field_3C), false);

    return true;
}

// 0x562F90
void sub_562F90(WmapTile* a1)
{
    a1->flags = 0;
    a1->field_18 = NULL;
    a1->field_1C = 0;
}

// 0x562FA0
bool wmTileArtLock(int tile)
{
    return wmTileArtLockMode(wmap_ui_mode, tile);
}

// 0x562FC0
void wmTileArtUnlock(int tile)
{
    wmTileArtUnlockMode(wmap_ui_mode, tile);
}

// 0x562FE0
void wmTileArtUnload(int tile)
{
    wmTileArtUnloadMode(wmap_ui_mode, tile);
}

// 0x563000
bool wmTileArtLockMode(WmapUiMode mode, int tile)
{
    char path[TIG_MAX_PATH];

    if ((wmap_ui_mode_info[mode].tiles[tile].flags & 0x1) == 0) {
        if (mode == 2) {
            snprintf(path, sizeof(path),
                "townmap\\%s\\%s%06d.bmp",
                wmap_ui_mode_info[mode].field_68,
                wmap_ui_mode_info[mode].field_68,
                tile);
            if (!tig_file_exists(path, NULL)) {
                snprintf(path, sizeof(path),
                    "townmap\\%s\\%s%06d.bmp",
                    wmap_ui_mode_info[mode].field_68,
                    wmap_ui_mode_info[mode].field_68,
                    0);
            }
        } else {
            snprintf(path, sizeof(path),
                "%s%03d",
                wmap_ui_mode_info[mode].field_68,
                tile + 1);
        }

        if (wmTileArtLoad(path, &(wmap_ui_mode_info[mode].tiles[tile].video_buffer), &(wmap_ui_mode_info[mode].tiles[tile].rect))) {
            wmap_ui_mode_info[mode].tiles[tile].flags |= 0x1;
            wmap_ui_mode_info[mode].num_loaded_tiles++;
        } else {
            tig_debug_printf("WMapUI: Blit: ERROR: Bmp Load Failed!\n");
        }
    }

    return true;
}

// 0x5630F0
bool wmTileArtLoad(const char* path, TigVideoBuffer** video_buffer_ptr, TigRect* rect)
{
    TigBmp bmp;
    TigVideoBufferData video_buffer_data;

    if (wmap_ui_mode != WMAP_UI_MODE_TOWN) {
        snprintf(bmp.name, sizeof(bmp.name), "WorldMap\\%s.bmp", path);
        path = bmp.name;
    }

    if (tig_video_buffer_load_from_bmp(path, video_buffer_ptr, 0x1) != TIG_OK) {
        tig_debug_printf("WMapUI: Pic [%s] FAILED to load!\n", path);
        return false;
    }

    if (tig_video_buffer_data(*video_buffer_ptr, &video_buffer_data) != TIG_OK) {
        return false;
    }

    rect->x = 0;
    rect->y = 0;
    rect->width = video_buffer_data.width;
    rect->height = video_buffer_data.height;

    return true;
}

// 0x563200
bool wmTileArtUnlockMode(WmapUiMode mode, int tile)
{
    (void)mode;
    (void)tile;

    return true;
}

// 0x563210
void wmTileArtUnloadMode(WmapUiMode mode, int tile)
{
    if ((wmap_ui_mode_info[mode].tiles[tile].flags & 0x1) != 0) {
        if (tig_video_buffer_destroy(wmap_ui_mode_info[mode].tiles[tile].video_buffer) == TIG_OK) {
            wmap_ui_mode_info[mode].tiles[tile].flags &= ~0x1;
            wmap_ui_mode_info[mode].num_loaded_tiles--;
        } else {
            tig_debug_printf("WMapUI: Destroy: ERROR: Video Buffer Destroy FAILED!\n");
        }
    }
}

// 0x563270
void sub_563270()
{
    if (wmap_ui_mode_info[wmap_ui_mode].refresh_rect != NULL) {
        wmap_ui_mode_info[wmap_ui_mode].refresh_rect(&(wmap_ui_mode_info[wmap_ui_mode].rect));
    }
}

// 0x5632A0
void sub_5632A0(int direction, int a2, int a3, int a4)
{
    (void)a2;
    (void)a3;
    (void)a4;

    switch (direction) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        if (wmap_ui_mode_info[wmap_ui_mode].refresh_rect != NULL) {
            wmap_ui_mode_info[wmap_ui_mode].refresh_rect(&(wmap_ui_mode_info[wmap_ui_mode].rect));
        }
        break;
    default:
        tig_debug_printf("WMapUI: Error: Refresh Scroll Direction out of bounds!\n");
        break;
    }
}

// 0x563300
void sub_563300(int direction, int a2, int a3, int a4)
{
    WmapInfo* wmap_info;
    TigRect one;
    TigRect two;
    TigRect three;
    int dx;
    int dy;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);
    one = wmap_info->rect;
    two = wmap_ui_nav_cvr_frame;
    three = one;

    switch (direction) {
    case 0:
        dx = 0;
        dy = a4;
        one.height = a4;
        three.width = 0;
        break;
    case 1:
        dx = -a4;
        dy = a4;
        one.height = a4;
        two.x = wmap_ui_nav_cvr_frame.x - a4;
        three.x += three.width - a4;
        three.y += dy;
        three.width = dy;
        three.height -= dy;
        break;
    case 2:
        dx = -a4;
        dy = 0;
        one.width = 0;
        two.x = wmap_ui_nav_cvr_frame.x - a4;
        two.width += a4;
        three.x += three.width - a4;
        three.width = a4;
        break;
    case 3:
        dx = -a4;
        dy = -a4;
        one.height = wmap_ui_nav_cvr_frame.height + a4;
        one.y = one.height + three.y - (a4 + wmap_ui_nav_cvr_frame.height);
        two.width = 0;
        three.x += three.width - a4;
        three.width = a4;
        three.height -= a4;
        break;
    case 4:
        dx = 0;
        dy = -a4;
        one.height = wmap_ui_nav_cvr_frame.height + a4;
        one.y = one.height + three.y - (wmap_ui_nav_cvr_frame.height + a4);
        two.width = 0;
        three.width = 0;
        break;
    case 5:
        dx = a4;
        dy = -a4;
        one.y = one.height + three.y - (wmap_ui_nav_cvr_frame.height + a4);
        one.height = wmap_ui_nav_cvr_frame.height + a4;
        two.width = 0;
        three.x += three.width - a4;
        three.width = a4;
        three.height -= a4;
        break;
    case 6:
        dx = a4;
        dy = 0;
        one.width = 0;
        three.width = a4;
        two.width = a4 + wmap_ui_nav_cvr_frame.width;
        break;
    case 7:
        dx = a4;
        dy = a4;
        one.height = a4;
        two.x = wmap_ui_nav_cvr_frame.x - a4;
        three.y = dy + three.y;
        three.width = dy;
        three.height -= dy;
        break;
    }

    tig_window_scroll_rect(wmap_ui_window, &(wmap_info->rect), dx, dy);

    if (wmap_info->refresh_rect != NULL) {
        if (one.width != 0) {
            wmap_info->refresh_rect(&one);
        }

        if (three.width != 0) {
            wmap_info->refresh_rect(&three);
        }

        if (two.width != 0) {
            wmap_info->refresh_rect(&two);
        }
    }
}

// 0x563590
void sub_563590(WmapCoords* coords, bool a2)
{
    WmapInfo* wmap_info;
    int v1;
    int v2;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    v1 = coords->x - wmap_info->field_24;
    if (v1 < 0) {
        v1 = 0;
    } else if (v1 > wmap_info->field_24 + wmap_info->field_2C) {
        v1 = wmap_info->field_2C;
    }

    v2 = coords->y - wmap_info->field_28;
    if (v2 < 0) {
        v2 = 0;
    } else if (v2 > wmap_info->field_24 + wmap_info->field_30) {
        v2 = wmap_info->field_30;
    }

    if (v1 != wmap_info->field_34
        || v2 != wmap_info->field_38) {
        wmap_info->field_34 = v1;
        wmap_info->field_38 = v2;

        if (a2) {
            wmap_info->refresh();
        }
    }
}

// 0x563610
void wmap_ui_handle_scroll()
{
    // NOTE: Very odd initial values.
    int horizontal_direction = -69;
    int vertical_direction = -69;
    bool scroll_horizontally = false;

    if (tig_timer_between(dword_66D9D0, gamelib_ping_time) >= 10) {
        dword_66D9D0 = gamelib_ping_time;

        if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
            if (tig_kb_is_key_pressed(SDL_SCANCODE_LEFT)) {
                scroll_horizontally = true;
                horizontal_direction = 6;
            } else if (tig_kb_is_key_pressed(SDL_SCANCODE_RIGHT)) {
                scroll_horizontally = true;
                horizontal_direction = 2;
            }

            if (tig_kb_is_key_pressed(SDL_SCANCODE_UP)) {
                vertical_direction = 0;
            } else if (tig_kb_is_key_pressed(SDL_SCANCODE_DOWN)) {
                vertical_direction = 4;
            } else {
                if (!scroll_horizontally) {
                    // Neither left/right nor up/down keys are pressed, no need
                    // to continue with scrolling.
                    return;
                }
            }

            switch (horizontal_direction) {
            case 6:
                switch (vertical_direction) {
                case 0:
                    wmap_ui_scroll_with_kb(7);
                    break;
                case 4:
                    wmap_ui_scroll_with_kb(5);
                    break;
                default:
                    wmap_ui_scroll_with_kb(6);
                    break;
                }
                break;
            case 2:
                switch (vertical_direction) {
                case 0:
                    wmap_ui_scroll_with_kb(1);
                    break;
                case 4:
                    wmap_ui_scroll_with_kb(3);
                    break;
                default:
                    wmap_ui_scroll_with_kb(2);
                    break;
                }
                break;
            default:
                switch (vertical_direction) {
                case 0:
                    wmap_ui_scroll_with_kb(0);
                    break;
                case 4:
                    wmap_ui_scroll_with_kb(4);
                    break;
                }
                break;
            }
        }
    }
}

// 0x563750
void wmap_ui_scroll_with_kb(int direction)
{
    wmap_ui_scroll_internal(direction, 4);
}

// 0x563770
void wmap_ui_scroll(int direction)
{
    if (wmap_ui_state != WMAP_UI_STATE_MOVING) {
        wmap_ui_scroll_internal(direction, 8);
    }
}

// 0x563790
void wmap_ui_scroll_internal(int direction, int scale)
{
    WmapInfo* wmap_info;
    int sx;
    int sy;
    int dx;
    int dy;
    int offset_x;
    int offset_y;
    TigRect rect;

    sx = scale * wmap_ui_scroll_speed_x;
    sy = scale * wmap_ui_scroll_speed_y;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    dx = 0;
    dy = 0;

    switch (direction) {
    case 0:
        dy = -sy;
        break;
    case 1:
        dx = sx;
        dy = -sy;
        break;
    case 2:
        dx = sx;
        break;
    case 3:
        dx = sx;
        dy = sy;
        break;
    case 4:
        dy = sy;
        break;
    case 5:
        dx = -sx;
        dy = sy;
        break;
    case 6:
        dx = -sx;
        break;
    case 7:
        dx = -sx;
        dy = -sy;
        break;
    }

    offset_x = wmap_info->field_34;
    offset_y = wmap_info->field_38;

    wmap_info->field_34 += dx;
    wmap_info->field_38 += dy;

    if (wmap_info->field_34 < 0) {
        wmap_info->field_34 = 0;
    } else if (wmap_info->field_34 > wmap_info->field_60) {
        wmap_info->field_34 = wmap_info->field_60;
    }

    if (wmap_info->field_38 < 0) {
        wmap_info->field_38 = 0;
    } else if (wmap_info->field_38 > wmap_info->field_64) {
        wmap_info->field_38 = wmap_info->field_64;
    }

    dx = wmap_info->field_34 - offset_x;
    dy = wmap_info->field_38 - offset_y;
    if (dx == 0 && dy == 0) {
        return;
    }

    if (wmap_info->refresh_rect == NULL) {
        return;
    }

    if (wmap_ui_mode != WMAP_UI_MODE_TOWN) {
        wmap_info->refresh_rect(&(wmap_info->rect));
        return;
    }

    tig_window_scroll_rect(wmap_ui_window,
        &(wmap_info->rect),
        -dx,
        -dy);

    if (dx > 0) {
        rect.x = wmap_ui_nav_cvr_frame.x - dx;
        rect.y = wmap_ui_nav_cvr_frame.y;
        rect.width = wmap_ui_nav_cvr_frame.width + dx;
        rect.height = wmap_ui_nav_cvr_frame.height;
        if (dy > 0) {
            rect.y -= dy;
            rect.height += dy;
        }
        wmap_info->refresh_rect(&rect);

        rect = wmap_info->rect;
        rect.x += rect.width - dx;
        rect.width = dx;
        if (dy > 0) {
            rect.y -= dy;
            rect.height += dy;
        }
        wmap_info->refresh_rect(&rect);
    } else if (dx < 0) {
        rect.x = wmap_ui_nav_cvr_frame.x;
        rect.y = wmap_ui_nav_cvr_frame.y;
        rect.height = wmap_ui_nav_cvr_frame.height;
        rect.width = wmap_ui_nav_cvr_frame.width - dx;
        if (dy > 0) {
            rect.y -= dy;
            rect.height += dy;
        }
        wmap_info->refresh_rect(&rect);

        rect = wmap_info->rect;
        rect.width = -dx;
        if (dy > 0) {
            rect.y -= dy;
            rect.height += dy;
        }
        wmap_info->refresh_rect(&rect);
    }

    if (dy > 0) {
        rect.x = wmap_ui_nav_cvr_frame.x;
        rect.y = wmap_ui_nav_cvr_frame.y - dy;
        rect.width = wmap_ui_nav_cvr_frame.width;
        rect.height = wmap_ui_nav_cvr_frame.height + dy;
        wmap_info->refresh_rect(&rect);

        rect = wmap_info->rect;
        rect.y += rect.height - dy;
        rect.height = dy;
        wmap_info->refresh_rect(&rect);
    } else if (dy < 0) {
        rect = wmap_ui_nav_cvr_frame;
        wmap_info->refresh_rect(&rect);

        rect = wmap_info->rect;
        rect.height = -dy;
        wmap_info->refresh_rect(&rect);
    }
}

// 0x563AC0
void sub_563AC0(int x, int y, WmapCoords* coords)
{
    coords->x = x + wmap_ui_mode_info[wmap_ui_mode].field_34 - wmap_ui_mode_info[wmap_ui_mode].rect.x;
    coords->y = y + wmap_ui_mode_info[wmap_ui_mode].field_38 - wmap_ui_mode_info[wmap_ui_mode].rect.y - wmap_ui_window_frame.y;
}

// 0x563B10
void sub_563B10(int x, int y, WmapCoords* coords)
{
    TigRect rect;
    int64_t width;
    int64_t height;

    rect = wmap_ui_mode_info[wmap_ui_mode].rect;

    if (is_default_wmap()) {
        width = 2000;
        height = 2000;
    } else {
        int64_t limit_x;
        int64_t limit_y;
        location_limits_get(&limit_x, &limit_y);

        width = limit_x / 64;
        height = limit_y / 64;
    }

    coords->x = (int)(((x - rect.x) * width) / rect.width);
    coords->y = (int)(((y - rect.y - wmap_ui_window_frame.y) * height) / rect.height);
}

// 0x563C00
void sub_563C00(int x, int y, WmapCoords* coords)
{
    coords->x = x + wmap_ui_mode_info[wmap_ui_mode].field_34 - wmap_ui_mode_info[wmap_ui_mode].rect.x;
    coords->y = y + wmap_ui_mode_info[wmap_ui_mode].field_38 - wmap_ui_mode_info[wmap_ui_mode].rect.y - wmap_ui_window_frame.y;
}

// 0x563C60
bool sub_563C60(WmapNote* note)
{
    WmapInfo* wmap_info;
    int index;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    if (wmap_info->notes == NULL) {
        return false;
    }

    // Attempt to find and update by id.
    for (index = 0; index < *wmap_info->num_notes; index++) {
        if (wmap_info->notes[index].id == note->id) {
            // Note already exists, copy over everything.
            wmap_info->notes[index] = *note;
            sub_563D50(&(wmap_info->notes[index]));
            return true;
        }
    }

    if (index < 199) {
        // Note does not exists, but there is a room for a new note.
        wmap_info->notes[index] = *note;
        sub_563D50(&(wmap_info->notes[index]));
        note->flags &= ~0x4;
        (*wmap_info->num_notes)++;
        sub_563D80(wmap_info->notes[index].id, wmap_ui_mode);
        return true;
    }

    // Note does not exists and there is no room for a new one.
    return false;
}

// 0x563D50
void sub_563D50(WmapNote* note)
{
    note->field_18.x = note->coords.x;
    note->field_18.y = note->coords.y;
    note->field_18.width = wmap_note_type_icon_max_width + 203;
    note->field_18.height = 50;
}

// 0x563D80
void sub_563D80(int a1, int a2)
{
    (void)a1;
    (void)a2;
}

// 0x563D90
WmapNote* sub_563D90(int id)
{
    int index;

    for (index = 0; index < *wmap_ui_mode_info[wmap_ui_mode].num_notes; index++) {
        if (wmap_ui_mode_info[wmap_ui_mode].notes[index].id == id) {
            return &(wmap_ui_mode_info[wmap_ui_mode].notes[index]);
        }
    }

    return NULL;
}

// 0x563DE0
bool find_note_by_coords(WmapCoords* coords, int* id_ptr)
{
    return find_note_by_coords_mode(coords, id_ptr, wmap_ui_mode);
}

// 0x563E00
bool find_note_by_coords_mode(WmapCoords* coords, int* id_ptr, WmapUiMode mode)
{
    int dx;
    int dy;
    WmapInfo* wmap_info;
    int idx;
    WmapNote* note;

    switch (mode) {
    case WMAP_UI_MODE_WORLD:
    case WMAP_UI_MODE_TOWN:
        dx = wmap_note_type_icon_max_width / 2;
        dy = wmap_note_type_icon_max_height / 2;
        break;
    case WMAP_UI_MODE_CONTINENT:
        dx = 20;
        dy = 20;
        break;
    default:
        if (id_ptr != NULL) {
            *id_ptr = -1;
        }
        return false;
    }

    wmap_info = &(wmap_ui_mode_info[mode]);
    if (wmap_info->notes != NULL) {
        for (idx = *wmap_info->num_notes - 1; idx >= 0; idx--) {
            note = &(wmap_info->notes[idx]);
            if (wmap_note_type_info[note->type].enabled
                && coords->x >= note->coords.x - dx
                && coords->x <= note->coords.x + dx
                && coords->y >= note->coords.y - dy
                && coords->y <= note->coords.y + dy) {
                if (id_ptr != NULL) {
                    *id_ptr = note->id;
                }
                return true;
            }
        }
    }

    if (id_ptr != NULL) {
        *id_ptr = -1;
    }

    return false;
}

// 0x563F00
bool sub_563F00(WmapCoords* coords, int64_t* a2)
{
    int64_t v1;
    int64_t pc_location;

    if (a2 == NULL) {
        return false;
    }

    sub_561800(coords, &v1);
    pc_location = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    if (location_dist(v1, pc_location) < qword_66D850) {
        sub_561490(pc_location, coords);
        *a2 = pc_location;
        return true;
    }

    *a2 = 0;
    return false;
}

// 0x563F90
void sub_563F90(WmapCoords* coords)
{
    stru_66D718.str[0] = '\0';
    stru_66D718.type = dword_66D89C;
    stru_66D718.coords = *coords;
    sub_563D50(&stru_66D718);
    stru_66D718.field_10 = 0;
    sub_563F00(&(stru_66D718.coords), &(stru_66D718.field_10));
    sub_564030(&stru_66D718);
}

// 0x564000
void sub_564000(int a1)
{
    int v1[3];

    v1[0] = WMAP_NOTE_TYPE_NOTE;
    v1[1] = WMAP_NOTE_TYPE_SKULL;
    v1[2] = WMAP_NOTE_TYPE_CHEST;
    dword_66D89C = v1[a1];
}

// 0x564030
void sub_564030(WmapNote* note)
{
    iso_interface_window_set(ROTWIN_TYPE_MAP_NOTE);
    dword_66D9D4 = note;
    wmap_ui_textedit.buffer = note->str;
    textedit_ui_focus(&wmap_ui_textedit);
    wmap_ui_textedit_focused = true;
    intgame_text_edit_refresh(note->str, wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].font);
}

// 0x564070
void sub_564070(bool a1)
{
    dword_66D9D4 = 0;
    stru_66D718.str[0] = '\0';
    stru_66D718.id = -1;
    wmap_ui_textedit_focused = false;
    textedit_ui_unfocus(&wmap_ui_textedit);
    wmap_ui_textedit.buffer = NULL;
    if (a1) {
        intgame_text_edit_refresh(" ", wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].font);
    }
}

// 0x5640C0
void wmap_ui_textedit_on_enter(TextEdit* textedit)
{
    bool should_refresh;

    if (*textedit->buffer != '\0' && sub_5643C0(textedit->buffer)) {
        if (textedit->buffer == stru_66D718.str) {
            dword_66D9D4->flags = 0;
            should_refresh = wmap_note_add(dword_66D9D4);
        } else {
            should_refresh = true;
        }
    } else {
        should_refresh = wmap_note_remove(dword_66D9D4);
    }

    sub_564070(1);

    if (should_refresh) {
        wmap_ui_mode_info[wmap_ui_mode].refresh();
    }
}

// 0x564140
bool wmap_note_add(WmapNote* note)
{
    return wmap_note_add_mode(note, wmap_ui_mode);
}

// 0x564160
bool wmap_note_add_mode(WmapNote* note, WmapUiMode mode)
{
    int note_index;

    if (wmap_ui_mode_info[mode].notes == NULL
        || *wmap_ui_mode_info[mode].num_notes >= 199) {
        return false;
    }

    note_index = *wmap_ui_mode_info[mode].num_notes;

    note->id = wmap_ui_mode_info[mode].field_194++;
    note->flags &= ~0x4;
    wmap_ui_mode_info[mode].notes[note_index] = *note;
    sub_563D50(&(wmap_ui_mode_info[mode].notes[note_index]));
    (*wmap_ui_mode_info[mode].num_notes)++;
    sub_563D80(wmap_ui_mode_info[mode].notes[note_index].id, mode);

    return true;
}

// 0x564210
bool wmap_note_remove(WmapNote* note)
{
    WmapInfo* wmap_info;
    int index;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    if (wmap_info->notes == NULL) {
        return false;
    }

    if (note->id == -1) {
        return false;
    }

    // Find note by id.
    for (index = 0; index < *wmap_info->num_notes; index++) {
        if (wmap_info->notes[index].id == note->id) {
            break;
        }
    }

    if (index >= *wmap_info->num_notes) {
        // Note does not exists.
        return false;
    }

    sub_5642F0(&(wmap_info->notes[index]));
    sub_5642E0(wmap_info->notes[index].id, wmap_ui_mode);

    (*wmap_info->num_notes)--;

    // Move subsequent notes up (one at a time).
    while (index < *wmap_info->num_notes) {
        wmap_info->notes[index] = wmap_info->notes[index + 1];
        index++;
    }

    return true;
}

// 0x5642E0
void sub_5642E0(int id, WmapUiMode mode)
{
    (void)id;
    (void)mode;
}

// 0x5642F0
void sub_5642F0(WmapNote* note)
{
    if ((note->flags & 0x4) != 0) {
        tig_video_buffer_destroy(note->video_buffer);
        note->flags &= ~0x4;
    }
}

// 0x564320
void wmap_ui_textedit_on_change(TextEdit* textedit)
{
    if (*textedit->buffer != '\0') {
        intgame_text_edit_refresh(textedit->buffer, wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].font);
    } else {
        intgame_text_edit_refresh(" ", wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].font);
    }
}

// 0x564360
void sub_564360(int id)
{
    WmapInfo* wmap_info;
    int index;
    int cnt;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);
    cnt = *wmap_info->num_notes;
    if (wmap_info->notes != NULL) {
        for (index = 0; index < cnt; index++) {
            if (wmap_info->notes[index].id == id) {
                break;
            }
        }

        if (index < cnt && (wmap_info->notes[index].flags & 0x2) == 0) {
            sub_564030(&(wmap_info->notes[index]));
        }
    }
}

// 0x5643C0
bool sub_5643C0(const char* str)
{
    while (*str != '\0') {
        if (*str != ' ') {
            return true;
        }
        str++;
    }

    return false;
}

// 0x5643E0
bool sub_5643E0(WmapCoords* coords)
{
    int start = 0;
    WmapRouteType route_type = WMAP_ROUTE_TYPE_WORLD;
    int wp_idx;
    WmapRouteWaypoint* wp;
    int64_t from;
    int64_t to;
    MesFileEntry mes_file_entry;
    WmapPathInfo path_info;
    int steps;
    UiMessage ui_message;

    if (dword_66D880) {
        if (wmap_ui_mode == WMAP_UI_MODE_TOWN) {
            route_type = WMAP_ROUTE_TYPE_TOWN;
        }
    } else {
        if (wmap_ui_mode != WMAP_UI_MODE_TOWN) {
            return false;
        }
        route_type = WMAP_ROUTE_TYPE_TOWN;
    }

    wp_idx = wmap_ui_routes[route_type].length;
    if (wp_idx >= 30) {
        return false;
    }

    wp = &(wmap_ui_routes[route_type].waypoints[wp_idx]);

    if (wmap_ui_mode == WMAP_UI_MODE_TOWN) {
        if (wp_idx == 0) {
            from = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
        } else if (wp_idx < 6) {
            from = wmap_ui_routes[route_type].waypoints[wp_idx - 1].loc;
        } else {
            // "You have reached the maximum allowable number of waypoints and may not add another."
            mes_file_entry.num = 611;
            mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
            return false;
        }

        townmap_coords_to_loc(&wmap_ui_tmi, coords->x, coords->y, &to);

        steps = sub_44EB40(player_get_local_pc_obj(), from, to);
        if (steps == 0) {
            // "Your path is blocked.  Try clicking closer to the previous waypoint."
            mes_file_entry.num = 610;
            mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
            return false;
        }
    } else {
        if (wp_idx == 0) {
            sub_561800(&(wmap_ui_mode_info[wmap_ui_mode].field_3C), &from);
        } else {
            start = wmap_ui_routes[route_type].waypoints[wp_idx - 1].start + wmap_ui_routes[route_type].waypoints[wp_idx - 1].steps;
            from = wmap_ui_routes[route_type].waypoints[wp_idx - 1].loc;
        }

        path_info.from = sector_id_from_loc(from);

        sub_561800(coords, &to);
        path_info.to = sector_id_from_loc(to);
        path_info.max_rotations = 5000 - start;
        path_info.rotations = &(byte_64E828[start]);

        steps = sub_4207D0(&path_info);
        if (steps == 0) {
            // "Your path is blocked.  You must locate either a bridge, pass or means of transporation in order to travel to this destination."
            mes_file_entry.num = 601;
            mes_get_msg(wmap_ui_worldmap_mes_file, &mes_file_entry);
            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);
            return false;
        }
    }

    dword_5C9AD8 = wmap_ui_routes[route_type].length;

    wp->steps = steps;
    wp->start = start;
    wp->field_0 = 0;
    wp->coords = *coords;
    wp->loc = to;
    wp->field_4 = -1;

    sub_5648E0(wmap_ui_routes[route_type].length, wmap_ui_routes[route_type].field_3C4, false);
    wmap_ui_routes[route_type].length++;

    sub_5649C0();

    wmap_ui_mode_info[wmap_ui_mode].refresh();

    return true;
}

// 0x564780
bool sub_564780(WmapCoords* coords, int* idx_ptr)
{
    WmapRouteType route_type;
    int dx;
    int dy;
    int idx;

    route_type = wmap_ui_mode == WMAP_UI_MODE_TOWN
        ? WMAP_ROUTE_TYPE_TOWN
        : WMAP_ROUTE_TYPE_WORLD;
    dx = wmap_ui_note_width / 2 + 5;
    dy = wmap_ui_note_height / 2 + 5;

    for (idx = wmap_ui_routes[route_type].length - 1; idx >= 0; idx--) {
        if (coords->x >= wmap_ui_routes[route_type].waypoints[idx].coords.x - dx
            && coords->x <= wmap_ui_routes[route_type].waypoints[idx].coords.x + dx
            && coords->y >= wmap_ui_routes[route_type].waypoints[idx].coords.y - dy
            && coords->y <= wmap_ui_routes[route_type].waypoints[idx].coords.y + dy) {
            *idx_ptr = idx;
            return true;
        }
    }

    return false;
}

// 0x564830
void sub_564830(int a1, WmapCoords* coords)
{
    (void)a1;
    (void)coords;
}

// 0x564840
void sub_564840(int a1)
{
    WmapRouteType route_type;
    int index;

    route_type = wmap_ui_mode == WMAP_UI_MODE_TOWN
        ? WMAP_ROUTE_TYPE_TOWN
        : WMAP_ROUTE_TYPE_WORLD;
    if (wmap_ui_routes[route_type].length > 0 && a1 < wmap_ui_routes[route_type].length) {
        wmap_ui_routes[route_type].length--;

        for (index = a1; index < wmap_ui_routes[route_type].length; index++) {
            wmap_ui_routes[route_type].waypoints[index] = wmap_ui_routes[route_type].waypoints[index + 1];
        }

        sub_5649C0();
        wmap_ui_mode_info[wmap_ui_mode].refresh();

        if (wmap_ui_routes[route_type].length == 0) {
            dword_65E968 = 0;
        }
    }
}

// 0x5648E0
void sub_5648E0(int a1, int a2, bool a3)
{
    WmapRouteType route_type;

    if (a1 == -1) {
        return;
    }

    route_type = wmap_ui_mode == WMAP_UI_MODE_TOWN
        ? WMAP_ROUTE_TYPE_TOWN
        : WMAP_ROUTE_TYPE_WORLD;
    if (wmap_ui_routes[route_type].waypoints[a1].field_4 == a2) {
        return;
    }

    wmap_ui_routes[route_type].waypoints[a1].field_4 = a2;

    if (a3) {
        sub_5649C0();
        wmap_ui_mode_info[wmap_ui_mode].refresh();
    }
}

// 0x564940
void sub_564940()
{
    if (dword_5C9AD8 != -1 && wmap_ui_mode != WMAP_UI_MODE_TOWN) {
        sub_564970(&(wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[dword_5C9AD8]));
    }
}

// 0x564970
void sub_564970(WmapRouteWaypoint* wp)
{
    int area;

    area = area_get_nearest_known_area(wp->loc, player_get_local_pc_obj(), qword_66D850);
    if (area > 0) {
        wp->loc = area_get_location(area);
        sub_561490(wp->loc, &(wp->coords));
    }
}

// 0x5649C0
void sub_5649C0()
{
}

// 0x5649D0
void sub_5649D0(int a1)
{
    (void)a1;
}

// 0x5649F0
bool sub_5649F0(int64_t loc)
{
    WmapInfo* wmap_info;
    int area;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);
    area = area_get_nearest_area_in_range(loc, true);
    if (area > 0) {
        loc = area_get_location(area);
    }

    if (wmap_ui_teleport(loc)) {
        sub_561490(loc, &(wmap_info->field_3C));
    }

    if (wmap_info->refresh != NULL) {
        wmap_info->refresh();
    }

    ambient_lighting_enable();

    return area > 0;
}

// 0x564A70
void sub_564A70(int64_t pc_obj, int64_t loc)
{
    int area;

    area = area_get_nearest_area_in_range(loc, false);
    if (area > 0) {
        if (!area_is_known(pc_obj, area)) {
            if (area_get_last_known_area(pc_obj) == area) {
                area_reset_last_known_area(pc_obj);
            }
            area_set_known(pc_obj, area);
            sub_562800(area);
        }

        if (area_get_last_known_area(pc_obj) == area) {
            area_reset_last_known_area(pc_obj);
        }
    }
}

// 0x564AF0
void wmap_ui_mark_townmap(int64_t obj)
{
    int64_t pc_loc;
    int pc_townmap;
    int64_t obj_loc;
    int obj_townmap;
    WmapNote note;

    pc_loc = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    pc_townmap = townmap_get(sector_id_from_loc(pc_loc));

    obj_loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    obj_townmap = townmap_get(sector_id_from_loc(obj_loc));

    if (pc_townmap != obj_townmap) {
        return;
    }

    if (pc_townmap == 0) {
        tig_debug_printf("WmapUI: WARNING: Object attempted to mark TownMap when no TownMap is available!\n");
        return;
    }

    if (!townmap_info(pc_townmap, &wmap_ui_tmi)) {
        return;
    }

    townmap_loc_to_coords(&wmap_ui_tmi, obj_loc, &(note.coords.x), &(note.coords.y));

    if (find_note_by_coords_mode(&(note.coords), NULL, WMAP_UI_MODE_TOWN)) {
        return;
    }

    object_examine(obj, OBJ_HANDLE_NULL, note.str);
    sub_563D50(&note);
    note.field_10 = 0;
    note.flags = 0x2;
    note.type = WMAP_NOTE_TYPE_QUEST;

    dword_66D878 = pc_townmap;

    if (!wmap_note_add_mode(&note, 2)) {
        tig_debug_printf("WmapUI: WARNING: Attempt to mark TownMap Failed: Note didn't Add!\n");
    }
}

// 0x564C20
bool wmap_ui_bkg_process_callback(TimeEvent* timeevent)
{
    bool v0;
    bool v1;
    WmapInfo* wmap_info;
    DateTime v9;
    DateTime datetime;
    TimeEvent next_timeevent;
    WmapCoords v8;
    WmapCoords v6;
    int64_t loc;

    (void)timeevent;

    v0 = false;
    v1 = false;
    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    if (wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length > 0) {
        v8 = wmap_info->field_3C;

        v6 = wmap_info->field_3C;
        sub_565170(&v6);
        sub_564E30(&v6, &loc);
        wmap_info->field_3C = v6;

        sub_564A70(player_get_local_pc_obj(), loc);

        if (dword_65E968 >= wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].start + wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].steps) {
            v0 = true;

            sub_564840(0);

            if (wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length > 0) {
                if (!sub_565140()) {
                    while (wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length > 0) {
                        sub_564840(0);
                    }

                    sub_5649F0(loc);
                }
            } else {
                if (sub_5649F0(loc)) {
                    v1 = true;
                }
            }
        }

        sub_563590(&wmap_info->field_3C, false);
        wmap_ui_compass_arrow_frame_set(wmap_ui_compass_arrow_frame_calc(&wmap_info->field_3C, &wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].coords));
        sub_5649C0();
        wmap_ui_mode_info[wmap_ui_mode].refresh();
    }

    sub_564EE0(&v8, &(wmap_info->field_3C), &v9);
    if (!v0) {
        timeevent_inc_datetime(&v9);
    }

    if (v1) {
        wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
        wmap_ui_close();
        return true;
    }

    if (wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length <= 0
        || dword_65E968 >= wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].start + wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].steps) {
        wmap_ui_state_set(WMAP_UI_STATE_NORMAL);
        wmap_ui_close();
        dword_65E968 = 0;
        wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].length = 0;
        return true;
    }

    next_timeevent.type = TIMEEVENT_TYPE_WORLDMAP;
    sub_45A950(&datetime, 62);
    timeevent_add_delay(&next_timeevent, &datetime);
    return true;
}

// 0x564E30
void sub_564E30(WmapCoords* coords, int64_t* loc_ptr)
{
    int x;
    int y;
    int area;

    x = ((dword_66D6F8 - coords->x) << 6) + 32;
    y = (coords->y << 6) + 32;
    *loc_ptr = location_make(x, y);

    area = area_get_nearest_known_area(*loc_ptr, player_get_local_pc_obj(), qword_66D850);
    if (area > 0) {
        if (!area_is_known(player_get_local_pc_obj(), area)) {
            *loc_ptr = area_get_location(area);
            sub_561490(*loc_ptr, coords);
        }
    }
}

// 0x564EE0
int64_t sub_564EE0(WmapCoords* a1, WmapCoords* a2, DateTime* datetime)
{
    int64_t v1;
    int64_t v2;
    int v3;

    sub_561800(a1, &v1);
    sub_561800(a2, &v2);
    v3 = (int)(location_dist(v1, v2) / 64);

    if (datetime != NULL) {
        sub_45A950(datetime, 3600000 * v3);
    }

    return v3;
}

// 0x564F60
void wmap_ui_notify_sector_changed(int64_t pc_obj, int64_t sec)
{
    wmap_ui_townmap = townmap_get(sec);
    if (wmap_ui_townmap != TOWNMAP_NONE) {
        sub_564A70(pc_obj, sector_loc_from_id(sec));
        wmap_ui_town_notes_load();
        ui_set_map_button(UI_PRIMARY_BUTTON_TOWNMAP);
    } else {
        sub_5650C0();
        ui_set_map_button(UI_PRIMARY_BUTTON_WORLDMAP);
    }
    sub_560EF0();
}

// 0x564FD0
void wmap_ui_town_notes_load()
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int index;
    bool success = true;

    if (dword_66D878 != wmap_ui_townmap) {
        sub_5650C0();
    }
    dword_66D878 = wmap_ui_townmap;

    if (dword_66D878 != 0) {
        sprintf(path,
            "%s\\%s.tmn",
            "Save\\Current",
            townmap_name(dword_66D878));

        stream = tig_file_fopen(path, "rb");
        if (stream == NULL) {
            return;
        }

        if (tig_file_fread(&wmap_ui_num_town_notes, sizeof(wmap_ui_num_town_notes), 1, stream) == 1
            && tig_file_fread(&(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_194), sizeof(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_194), 1, stream) == 1) {
            for (index = 0; index < wmap_ui_num_town_notes; index++) {
                if (!wmap_ui_town_note_load(&(wmap_ui_town_notes[index]), stream)) {
                    success = false;
                    break;
                }
            }
        } else {
            success = false;
        }

        tig_file_fclose(stream);

        if (!success) {
            tig_debug_printf("wmap_ui_town_notes_load(): Warning reading data!\n");
        }
    }
}

// 0x5650C0
void sub_5650C0()
{
    if (dword_66D878 != 0) {
        wmap_ui_town_notes_save();
    }
    dword_66D878 = 0;
    wmap_ui_num_town_notes = 0;
}

// 0x5650E0
int wmap_ui_compass_arrow_frame_calc(WmapCoords* a, WmapCoords* b)
{
    if (b->x < a->x) {
        if (b->y < a->y) {
            return 3 * wmap_ui_compass_arrow_num_frames / 4;
        } else {
            return wmap_ui_compass_arrow_num_frames / 2;
        }
    } else {
        if (b->y < a->y) {
            return 0;
        } else {
            return wmap_ui_compass_arrow_num_frames / 4;
        }
    }
}

// 0x565130
void wmap_ui_compass_arrow_frame_set(int frame)
{
    (void)frame;
}

// NOTE: It's just a guess, never used.
//
// 0x5649E0
int wmap_ui_compass_arrow_frame_get()
{
    return 0;
}

// 0x565140
bool sub_565140()
{
    if (!sub_424070(player_get_local_pc_obj(), 3, 0, 1)) {
        return false;
    }

    magictech_demaintain_spells(player_get_local_pc_obj());

    return true;
}

// 0x565170
void sub_565170(WmapCoords* coords)
{
    int64_t loc;
    int64_t adjacent_sec;

    sub_561800(coords, &loc);
    sector_in_dir(sector_id_from_loc(loc), byte_64E828[dword_65E968], &adjacent_sec);

    dword_65E968++;
    if (dword_65E968 < wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].start + wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].steps) {
        sub_561490(sector_loc_from_id(adjacent_sec), coords);
    } else {
        sub_561490(wmap_ui_routes[WMAP_ROUTE_TYPE_WORLD].waypoints[0].loc, coords);
    }
}

// 0x565230
void sub_565230()
{
    TigVideoBufferBlitInfo vb_blit_info;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect tmp_rect;
    TigRect vb_dst_rect;
    WmapNote* note;
    WmapCoords coords;
    int idx;
    int area;

    if (wmap_ui_mode_info[WMAP_UI_MODE_WORLD].str[0] == '\0') {
        return;
    }

    if (!wmTileArtLoad(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].str, &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].video_buffer), &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_2A8))) {
        return;
    }

    wmap_ui_mode_info[WMAP_UI_MODE_WORLD].flags |= 0x01;

    tig_window_fill(wmap_ui_window, &wmap_ui_canvas_frame, tig_color_make(0, 0, 0));

    vb_dst_rect = wmap_ui_mode_info[wmap_ui_mode].rect;

    vb_blit_info.flags = 0;
    vb_blit_info.src_video_buffer = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].video_buffer;
    vb_blit_info.src_rect = &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_2A8);
    vb_blit_info.dst_rect = &vb_dst_rect;

    if (tig_window_vbid_get(wmap_ui_window, &vb_blit_info.dst_video_buffer) != TIG_OK
        || tig_video_buffer_blit(&vb_blit_info) != TIG_OK) {
        tig_debug_printf("WMapUI: Zoomed Blit: ERROR: Blit FAILED!\n");
        return;
    }

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    for (idx = 0; idx < *wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_notes; idx++) {
        note = &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].notes[idx]);
        if (wmap_note_type_info[note->type].enabled) {
            tmp_rect.x = vb_dst_rect.x + note->coords.x;
            tmp_rect.y = vb_dst_rect.y + note->coords.y + wmap_ui_window_frame.y;
            sub_5656B0(tmp_rect.x, tmp_rect.y, &coords);
            tmp_rect.x = vb_dst_rect.x + coords.x;
            tmp_rect.y = vb_dst_rect.y + coords.y;
            tmp_rect.width = 2;
            tmp_rect.height = 2;
            tig_window_fill(wmap_ui_window, &tmp_rect, tig_color_make(255, 0, 0));
        }
    }

    area = area_get_last_known_area(player_get_local_pc_obj());
    if (area != AREA_UNKNOWN) {
        sub_561490(area_get_location(area), &coords);
        tmp_rect.x = vb_dst_rect.x + note->coords.x;
        tmp_rect.y = vb_dst_rect.y + note->coords.y + wmap_ui_window_frame.y;
        sub_5656B0(tmp_rect.x, tmp_rect.y, &coords);
        tmp_rect.x = vb_dst_rect.x + coords.x - 1;
        tmp_rect.y = vb_dst_rect.y + coords.y - 1;
        tmp_rect.width = 5;
        tmp_rect.height = 5;
        tig_window_fill(wmap_ui_window, &tmp_rect, tig_color_make(255, 255, 0));
    }

    tmp_rect.x = vb_dst_rect.x + wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_3C.x;
    tmp_rect.y = vb_dst_rect.y + wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_3C.y + wmap_ui_window_frame.y;
    sub_5656B0(tmp_rect.x, tmp_rect.y, &coords);
    tmp_rect.x = vb_dst_rect.x + coords.x;
    tmp_rect.y = vb_dst_rect.y + coords.y;
    tmp_rect.width = 3;
    tmp_rect.height = 3;
    tig_window_fill(wmap_ui_window, &tmp_rect, tig_color_make(0, 255, 0));

    if (tig_art_interface_id_create(217, 0, 0, 0, &(art_blit_info.art_id)) == TIG_OK
        && tig_art_frame_data(art_blit_info.art_id, &art_frame_data) == TIG_OK) {
        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.y = 382 - wmap_ui_window_frame.y;
        dst_rect.x = 294;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(wmap_ui_window, &art_blit_info);
    }
}

// 0x5656B0
void sub_5656B0(int x, int y, WmapCoords* coords)
{
    TigRect rect;
    int64_t width;
    int64_t height;
    int offset_x;
    int offset_y;

    rect = wmap_ui_mode_info[wmap_ui_mode].rect;

    if (is_default_wmap()) {
        width = 2000;
        height = 2000;
    } else {
        int64_t limit_x;
        int64_t limit_y;
        location_limits_get(&limit_x, &limit_y);

        width = limit_x / 64;
        height = limit_y / 64;
    }

    offset_x = x - rect.x;
    if (offset_x == 0) {
        offset_x = 1;
    }
    coords->x = (int)(offset_x * rect.width / width);

    offset_y = y - wmap_ui_window_frame.y - rect.y;
    if (offset_y == 0) {
        offset_y = 1;
    }
    coords->y = (int)(offset_y * rect.height / height);
}

// 0x5657A0
void wmap_world_refresh_rect(TigRect* rect)
{
    WmapInfo* wmap_info;
    TigRect tmp_rect;
    TigVideoBufferBlitInfo vb_blit_info;
    TigRect vb_src_rect;
    TigRect vb_dst_rect;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int offset_x;
    int offset_y;
    WmapTile* v2;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int x;
    int y;
    int idx;
    WmapNote* note;
    int area;

    wmap_info = &(wmap_ui_mode_info[wmap_ui_mode]);

    if (rect == NULL) {
        rect = &(wmap_info->rect);
    }

    tmp_rect = *rect;

    tig_window_fill(wmap_ui_window, &tmp_rect, tig_color_make(0, 0, 0));

    if (wmap_info->field_68[0] == '\0') {
        return;
    }

    offset_x = wmap_info->field_34;
    offset_y = wmap_info->field_38;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    vb_src_rect.width = 5;
    vb_src_rect.height = 5;

    vb_blit_info.flags = 0;
    vb_blit_info.src_rect = &vb_src_rect;
    vb_blit_info.dst_rect = &vb_dst_rect;

    if (tig_window_vbid_get(wmap_ui_window, &(vb_blit_info.dst_video_buffer)) != TIG_OK) {
        tig_debug_printf("WMapUI: Zoomed Blit: ERROR: Blit FAILED!\n");
        return;
    }

    min_x = offset_x / wmap_info->tile_width;
    if (min_x < 0) {
        min_x = 0;
    }

    min_y = offset_y / wmap_info->tile_height;
    if (min_y < 0) {
        min_y = 0;
    }

    v2 = &(wmap_info->tiles[min_y * wmap_info->num_hor_tiles + min_x]);

    max_x = tmp_rect.width / v2->rect.width + min_x + 1;
    if (max_x > wmap_info->num_hor_tiles) {
        max_x = wmap_info->num_hor_tiles;
    }

    max_y = tmp_rect.height / v2->rect.height + min_y + 2;
    if (max_y > wmap_info->num_vert_tiles) {
        max_y = wmap_info->num_vert_tiles;
    }

    for (y = min_y; y < max_y; y++) {
        for (x = min_x; x < max_x; x++) {
            idx = y * wmap_info->num_hor_tiles + x;
            v2 = &(wmap_info->tiles[idx]);
            vb_src_rect.x = tmp_rect.x + v2->rect.width * x - offset_x;
            vb_src_rect.y = tmp_rect.y + v2->rect.height * y - offset_y;
            vb_src_rect.width = v2->rect.width;
            vb_src_rect.height = v2->rect.height;

            if (tig_rect_intersection(&vb_src_rect, &tmp_rect, &vb_dst_rect) != TIG_OK) {
                break;
            }

            vb_src_rect.x = vb_dst_rect.x - vb_src_rect.x;
            vb_src_rect.y = vb_dst_rect.y - vb_src_rect.y;
            vb_src_rect.width = vb_dst_rect.width;
            vb_src_rect.height = vb_dst_rect.height;

            if (wmTileArtLock(idx)) {
                vb_blit_info.src_video_buffer = v2->video_buffer;
                if (tig_video_buffer_blit(&vb_blit_info) != TIG_OK) {
                    tig_debug_printf("WMapUI: Zoomed Blit: ERROR: Blit FAILED!\n");
                    return;
                }

                wmTileArtUnlock(idx);
            }
        }
    }

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    vb_src_rect.x = 0;
    vb_src_rect.y = 0;

    vb_dst_rect.x = 0;
    vb_dst_rect.y = 0;

    for (idx = 0; idx < *wmap_info->num_notes; idx++) {
        note = &(wmap_info->notes[idx]);
        if (wmap_note_type_info[note->type].enabled
            && sub_565CF0(note)) {
            sub_566D10(note->type, &(note->coords), &tmp_rect, rect, wmap_info);
            sub_565D00(note, &tmp_rect, rect);
        }
    }

    sub_566A80(wmap_info, &tmp_rect, rect);

    area = area_get_last_known_area(player_get_local_pc_obj());
    if (area != AREA_UNKNOWN) {
        WmapCoords coords;

        sub_561490(area_get_location(area), &coords);
        sub_566D10(WMAP_NOTE_TYPE_NEW_LOC, &coords, &tmp_rect, rect, wmap_info);
    }

    offset_x = wmap_info->field_3C.x - wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width / 2 - wmap_info->field_34;
    offset_y = wmap_info->field_3C.y + wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height / -2 - wmap_info->field_38;

    vb_dst_rect.x = offset_x + tmp_rect.x;
    vb_dst_rect.y = offset_y + tmp_rect.y;
    vb_dst_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width;
    vb_dst_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height;

    if (tig_rect_intersection(&vb_dst_rect, rect, &vb_dst_rect) == TIG_OK) {
        dst_rect = vb_dst_rect;
        vb_dst_rect.x -= tmp_rect.x + offset_x;
        vb_dst_rect.y -= tmp_rect.y + offset_y;

        art_blit_info.flags = 0;
        art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].data.art_id;
        art_blit_info.src_rect = &vb_dst_rect;
        art_blit_info.dst_rect = &dst_rect;
        art_blit_info.dst_video_buffer = dword_64E7F4;
        tig_window_blit_art(wmap_ui_window, &art_blit_info);
    }

    if (tig_rect_intersection(&wmap_ui_nav_cvr_frame, rect, &dst_rect) == TIG_OK) {
        src_rect.x = wmap_ui_nav_cvr_frame.x - dst_rect.x;
        src_rect.y = wmap_ui_nav_cvr_frame.y - dst_rect.y;
        src_rect.width = wmap_ui_nav_cvr_width - (wmap_ui_nav_cvr_frame.x - dst_rect.x);
        src_rect.height = wmap_ui_nav_cvr_height - (wmap_ui_nav_cvr_frame.y - dst_rect.y);

        art_blit_info.flags = 0;
        art_blit_info.art_id = wmap_ui_nav_cvr_art_id;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(wmap_ui_window, &art_blit_info);
    }
}

// 0x565CF0
bool sub_565CF0(WmapNote* note)
{
    (void)note;

    return true;
}

// 0x565D00
void sub_565D00(WmapNote* note, TigRect* a2, TigRect* a3)
{
    TigWindowBlitInfo win_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int x;
    int y;
    int dx;
    int dy;

    wmap_note_vbid_lock(note);

    if ((note->flags & 0x4) != 0) {
        x = 0;
        y = 0;
        dx = a2->x + note->coords.x - wmap_ui_mode_info[wmap_ui_mode].field_34;
        dy = a2->y + note->coords.y - wmap_ui_mode_info[wmap_ui_mode].field_38;

        if (wmap_ui_mode == WMAP_UI_MODE_WORLD && note->id <= area_count()) {
            area_get_wm_offset(note->id, &x, &y);
        }

        x += dx - 10;
        y += dy - 10;

        dst_rect.x = x;
        dst_rect.y = y;
        dst_rect.width = note->field_18.width;
        dst_rect.height = note->field_18.height;

        if (tig_rect_intersection(&dst_rect, a3, &dst_rect) == TIG_OK) {
            src_rect.x = dst_rect.x - x;
            src_rect.y = dst_rect.y - y;
            src_rect.width = dst_rect.width;
            src_rect.height = dst_rect.height;

            win_blit_info.type = TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW;
            win_blit_info.vb_blit_flags = 0;
            win_blit_info.src_video_buffer = note->video_buffer;
            win_blit_info.src_rect = &src_rect;
            win_blit_info.dst_window_handle = wmap_ui_window;
            win_blit_info.dst_rect = &dst_rect;
            tig_window_blit(&win_blit_info);
        }
    }
}

// 0x565E40
void wmap_note_vbid_lock(WmapNote* note)
{
    TigVideoBufferCreateInfo vb_create_info;
    TigRect dirty_rect;

    if ((note->flags & 0x4) == 0) {
        vb_create_info.flags = TIG_VIDEO_BUFFER_LOCKED | TIG_VIDEO_BUFFER_VIDEO_MEMORY;
        vb_create_info.width = wmap_note_type_icon_max_width + 203;
        vb_create_info.height = 50;
        vb_create_info.background_color = dword_64E03C;
        vb_create_info.color_key = dword_64E03C;
        if (tig_video_buffer_create(&vb_create_info, &(note->video_buffer)) != TIG_OK) {
            tig_debug_printf("wmap_note_vbid_lock: ERROR: tig_video_buffer_create failed!\n");
            exit(EXIT_SUCCESS); // FIXME: Should be EXIT_FAILURE.
        }

        note->flags |= 0x4;

        sub_565F00(note->video_buffer, &(note->field_18));
        tig_font_push(wmap_note_type_info[note->type].font);
        tig_font_write(note->video_buffer, note->str, &wmap_ui_note_bounds, &dirty_rect);
        tig_font_pop();
    }
}

// 0x565F00
void sub_565F00(TigVideoBuffer* video_buffer, TigRect* rect)
{
    TigVideoBufferBlitInfo vb_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int offset_x;
    int offset_y;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int idx;
    WmapTile* entry;
    int col;
    int row;
    TigRect bounds;

    if (rect == NULL) {
        return;
    }

    bounds.x = rect->x;
    bounds.y = rect->y;
    bounds.width = rect->width;
    bounds.height = rect->height;

    src_rect.width = 5;
    src_rect.height = 5;

    offset_x = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_34;
    offset_y = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].field_38;

    vb_blit_info.flags = TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_CONST;
    vb_blit_info.src_rect = &src_rect;
    vb_blit_info.alpha[0] = 0;
    vb_blit_info.alpha[1] = 0;
    vb_blit_info.alpha[2] = 0;
    vb_blit_info.alpha[3] = 0;
    vb_blit_info.dst_video_buffer = video_buffer;
    vb_blit_info.dst_rect = &dst_rect;

    min_x = offset_x / wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_width;
    if (min_x < 0) {
        min_x = 0;
    }

    min_y = offset_y / wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tile_height;
    if (min_y < 0) {
        min_y = 0;
    }

    idx = min_x + wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles * min_y;
    entry = &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles[idx]);

    max_x = rect->width / entry->rect.width + min_x + 1;
    if (max_x > wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles) {
        max_x = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles;
    }

    max_y = rect->height / entry->rect.height + min_y + 2;
    if (max_y > wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles) {
        max_y = wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_vert_tiles;
    }

    for (row = min_y; row < max_y; row++) {
        for (col = min_x; col < max_x; col++) {
            idx = col + wmap_ui_mode_info[WMAP_UI_MODE_WORLD].num_hor_tiles * row;
            entry = &(wmap_ui_mode_info[WMAP_UI_MODE_WORLD].tiles[idx]);

            src_rect.x = bounds.x + src_rect.width * min_x - offset_x;
            src_rect.y = bounds.y + src_rect.height * min_y - offset_y;

            if (tig_rect_intersection(&src_rect, &bounds, &dst_rect) == TIG_OK) {
                src_rect.x = dst_rect.x - src_rect.x;
                src_rect.y = dst_rect.y - src_rect.y;
                src_rect.width = dst_rect.width;
                src_rect.height = dst_rect.height;

                if (wmTileArtLock(idx)) {
                    dst_rect.x = 0;
                    dst_rect.y = 0;
                    vb_blit_info.src_video_buffer = entry->video_buffer;

                    if (tig_video_buffer_blit(&vb_blit_info) != TIG_OK) {
                        tig_debug_printf("WMapUI: Zoomed Blit: ERROR: Blit FAILED!\n");
                        return;
                    }

                    wmTileArtUnlock(idx);
                }
            }
        }
    }
}

// 0x566130
void wmap_town_refresh_rect(TigRect* rect)
{
    int min_x;
    int min_y;
    int max_y;
    int max_x;
    int row;
    int col;
    int idx;
    TigRect bounds;
    TigRect dirty_rect;
    TigVideoBufferBlitInfo vb_blit_info;
    TigRect vb_src_rect;
    TigRect vb_dst_rect;
    TigArtBlitInfo art_blit_info;
    TigRect art_src_rect;
    TigRect art_dst_rect;
    WmapTile* entry;
    int64_t loc;
    int offset_x;
    int offset_y;
    ObjectList objects;
    ObjectNode* node;
    int note;

    if (rect == NULL) {
        rect = &(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].rect);
    }

    offset_x = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_34;
    offset_y = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_38;
    bounds = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].rect;

    vb_blit_info.flags = 0;
    vb_blit_info.src_rect = &vb_src_rect;
    vb_blit_info.dst_rect = &vb_dst_rect;

    if (tig_window_vbid_get(wmap_ui_window, &(vb_blit_info.dst_video_buffer)) != TIG_OK) {
        tig_debug_printf("WMapUI: wmap_town_refresh_rect(): tig_window_vbid_get() failed!\n");
        return;
    }

    dirty_rect.x = rect->x;
    dirty_rect.y = rect->y + wmap_ui_window_frame.y;
    dirty_rect.width = rect->width;
    dirty_rect.height = rect->height;
    tig_window_invalidate_rect(&dirty_rect);

    dirty_rect.x = rect->x;
    dirty_rect.y = rect->y;
    dirty_rect.width = rect->width;
    dirty_rect.height = rect->height;
    tig_window_fill(wmap_ui_window, &dirty_rect, tig_color_make(0, 0, 0));

    min_x = offset_x / wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tile_width;
    if (min_x < 0) {
        min_x = 0;
    }

    min_y = offset_y / wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tile_height;
    if (min_y < 0) {
        min_y = 0;
    }

    idx = min_x + min_y * wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_hor_tiles;
    entry = &(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tiles[idx]);

    max_x = bounds.width / entry->rect.width + min_x + 2;
    if (max_x > wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_hor_tiles) {
        max_x = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_hor_tiles;
    }

    max_y = bounds.height / entry->rect.height + min_y + 3;
    if (max_y > wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_vert_tiles) {
        max_y = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_vert_tiles;
    }

    for (row = min_y; row < max_y; row++) {
        for (col = min_x; col < max_x; col++) {
            idx = col + row * wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_hor_tiles;
            entry = &(wmap_ui_mode_info[WMAP_UI_MODE_TOWN].tiles[idx]);

            vb_src_rect.width = entry->rect.width;
            vb_src_rect.height = entry->rect.height;
            vb_src_rect.x = bounds.x + col * vb_src_rect.width - offset_x;
            vb_src_rect.y = bounds.y + row * vb_src_rect.height - offset_y;

            if (tig_rect_intersection(&vb_src_rect, &dirty_rect, &vb_dst_rect) == TIG_OK) {
                vb_src_rect.x = vb_dst_rect.x - vb_src_rect.x;
                vb_src_rect.y = vb_dst_rect.y - vb_src_rect.y;
                vb_src_rect.width = vb_dst_rect.width;
                vb_src_rect.height = vb_dst_rect.height;

                if (wmTileArtLock(idx)) {
                    if (townmap_is_waitable(wmap_ui_townmap)) {
                        vb_blit_info.flags = 0;
                        vb_blit_info.src_video_buffer = entry->video_buffer;
                        if (tig_video_buffer_blit(&vb_blit_info) != TIG_OK) {
                            tig_debug_printf("WMapUI: TownMap Blit: ERROR: Blit FAILED!\n");
                            return;
                        }
                    } else if (townmap_tile_blit_info(wmap_ui_townmap, idx, &vb_blit_info)) {
                        vb_blit_info.src_video_buffer = entry->video_buffer;
                        if (tig_video_buffer_blit(&vb_blit_info) != TIG_OK) {
                            tig_debug_printf("WMapUI: TownMap Blit: ERROR: Blit FAILED!\n");
                            return;
                        }
                        wmTileArtUnlock(idx);
                    } else {
                        tig_window_fill(wmap_ui_window, &vb_dst_rect, tig_color_make(0, 0, 0));
                        wmTileArtUnlock(idx);
                    }
                }
            }
        }
    }

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &art_src_rect;
    art_blit_info.dst_rect = &art_dst_rect;

    for (note = 0; note < *wmap_ui_mode_info[WMAP_UI_MODE_TOWN].num_notes; note++) {
        offset_x = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].notes[note].coords.x;
        offset_y = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].notes[note].coords.y;

        offset_x -= wmap_note_type_info[WMAP_NOTE_TYPE_QUEST].width / 2 + wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_34;
        offset_y -= wmap_note_type_info[WMAP_NOTE_TYPE_QUEST].height / 2 + wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_38;

        vb_dst_rect.x = bounds.x + offset_x;
        vb_dst_rect.y = bounds.y + offset_y;
        vb_dst_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_QUEST].width;
        vb_dst_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_QUEST].height;

        if (tig_rect_intersection(&vb_dst_rect, &dirty_rect, &vb_dst_rect) == TIG_OK) {
            art_dst_rect = vb_dst_rect;
            vb_dst_rect.x -= bounds.x + offset_x;
            vb_dst_rect.y -= bounds.y + offset_y;

            art_blit_info.flags = 0;
            art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_QUEST].data.art_id;
            art_blit_info.src_rect = &vb_dst_rect;
            art_blit_info.dst_video_buffer = dword_64E7F4;
            art_blit_info.dst_rect = &art_dst_rect;
            tig_window_blit_art(wmap_ui_window, &art_blit_info);
        }
    }

    sub_566A80(&(wmap_ui_mode_info[WMAP_UI_MODE_TOWN]), &bounds, &dirty_rect);
    offset_x = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_3C.x - wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width / 2 - wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_34;
    offset_y = wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_3C.y - wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height / 2 - wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_38;

    vb_dst_rect.x = bounds.x + offset_x;
    vb_dst_rect.y = bounds.y + offset_y;
    vb_dst_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width;
    vb_dst_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height;

    if (tig_rect_intersection(&vb_dst_rect, &dirty_rect, &vb_dst_rect) == TIG_OK) {
        art_dst_rect = vb_dst_rect;
        vb_dst_rect.x -= bounds.x + offset_x;
        vb_dst_rect.y -= bounds.y + offset_y;

        art_blit_info.flags = 0;
        art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].data.art_id;
        art_blit_info.src_rect = &vb_dst_rect;
        art_blit_info.dst_video_buffer = dword_64E7F4;
        art_blit_info.dst_rect = &art_dst_rect;
        tig_window_blit_art(wmap_ui_window, &art_blit_info);
    }

    object_list_all_followers(player_get_local_pc_obj(), &objects);
    node = objects.head;
    while (node != NULL) {
        loc = obj_field_int64_get(node->obj, OBJ_F_LOCATION);
        townmap_loc_to_coords(&wmap_ui_tmi, loc, &offset_x, &offset_y);
        offset_x -= wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width / 2 + wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_34;
        offset_y -= wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height / 2 + wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_38;

        vb_dst_rect.x = bounds.x + offset_x;
        vb_dst_rect.y = bounds.y + offset_y;
        vb_dst_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width;
        vb_dst_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height;

        if (tig_rect_intersection(&vb_dst_rect, &dirty_rect, &vb_dst_rect) == TIG_OK) {
            art_dst_rect = vb_dst_rect;
            vb_dst_rect.x -= bounds.x + offset_x;
            vb_dst_rect.y -= bounds.y + offset_y;

            art_blit_info.flags = 0;
            art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].data.art_id;
            art_blit_info.src_rect = &vb_dst_rect;
            art_blit_info.dst_video_buffer = dword_64E7F4;
            art_blit_info.dst_rect = &art_dst_rect;
            tig_window_blit_art(wmap_ui_window, &art_blit_info);
        }

        node = node->next;
    }
    object_list_destroy(&objects);

    if (tig_net_is_active()) {
        object_list_party(player_get_local_pc_obj(), &objects);
        node = objects.head;
        while (node != NULL) {
            if (!player_is_local_pc_obj(node->obj)) {
                loc = obj_field_int64_get(node->obj, OBJ_F_LOCATION);
                townmap_loc_to_coords(&wmap_ui_tmi, loc, &offset_x, &offset_y);
                offset_x -= wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width / 2 + wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_34;
                offset_y -= wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height / 2 + wmap_ui_mode_info[WMAP_UI_MODE_TOWN].field_38;

                vb_dst_rect.x = bounds.x + offset_x;
                vb_dst_rect.y = bounds.y + offset_y;
                vb_dst_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].width;
                vb_dst_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].height;

                if (tig_rect_intersection(&vb_dst_rect, &dirty_rect, &vb_dst_rect) == TIG_OK) {
                    art_dst_rect = vb_dst_rect;
                    vb_dst_rect.x -= bounds.x + offset_x;
                    vb_dst_rect.y -= bounds.y + offset_y;

                    art_blit_info.flags = 0;
                    art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_CROSS].data.art_id;
                    art_blit_info.src_rect = &vb_dst_rect;
                    art_blit_info.dst_video_buffer = dword_64E7F4;
                    art_blit_info.dst_rect = &art_dst_rect;
                    tig_window_blit_art(wmap_ui_window, &art_blit_info);
                }
            }
            node = node->next;
        }
        object_list_destroy(&objects);
    }

    if (tig_rect_intersection(&wmap_ui_nav_cvr_frame, &dirty_rect, &art_dst_rect) == TIG_OK) {
        art_src_rect.x = art_dst_rect.x - wmap_ui_nav_cvr_frame.x;
        art_src_rect.y = art_dst_rect.y - wmap_ui_nav_cvr_frame.y;
        art_src_rect.width = art_dst_rect.width;
        art_src_rect.height = art_dst_rect.height;

        art_blit_info.flags = 0;
        art_blit_info.art_id = wmap_ui_nav_cvr_art_id;
        art_blit_info.src_rect = &art_src_rect;
        art_blit_info.dst_rect = &art_dst_rect;
        tig_window_blit_art(wmap_ui_window, &art_blit_info);
    }
}

// 0x566A80
void sub_566A80(WmapInfo* a1, TigRect* a2, TigRect* a3)
{
    WmapRouteType route_type;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigLine line;
    TigDrawLineModeInfo line_mode_info;
    TigDrawLineStyleInfo line_style_info;
    int index;
    int x1;
    int y1;
    int x2;
    int y2;

    route_type = wmap_ui_mode == WMAP_UI_MODE_TOWN
        ? WMAP_ROUTE_TYPE_TOWN
        : WMAP_ROUTE_TYPE_WORLD;
    if (wmap_ui_routes[route_type].length <= 0) {
        return;
    }

    line_mode_info.mode = TIG_DRAW_LINE_MODE_NORMAL;
    line_mode_info.thickness = 1;
    tig_draw_set_line_mode(&line_mode_info);

    line_style_info.style = TIG_LINE_STYLE_DASHED;
    tig_draw_set_line_style(&line_style_info);

    x1 = a2->x + a1->field_3C.x - a1->field_34;
    y1 = a2->y + a1->field_3C.y - a1->field_38;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].width;
    src_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].height;

    art_blit_info.flags = 0;
    art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_NOTE].data.art_id;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_video_buffer = dword_64E7F4;

    for (index = 0; index < wmap_ui_routes[route_type].length; index++) {
        x2 = a2->x + wmap_ui_routes[route_type].waypoints[index].coords.x - a1->field_34;
        y2 = a2->y + wmap_ui_routes[route_type].waypoints[index].coords.y - a1->field_38;

        line.x1 = x1;
        line.y1 = y1;
        line.x2 = x2;
        line.y2 = y2;

        if (tig_line_intersection(a3, &line) == TIG_OK) {
            tig_window_line(wmap_ui_window,
                &line,
                index == dword_5C9AD8 ? dword_64E034 : dword_65E974);
        }

        src_rect.x = x2 - wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].width / 2;
        src_rect.y = y2 - wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].height / 2;
        src_rect.width = wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].width;
        src_rect.height = wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].height;
        art_blit_info.art_id = wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].data.art_id;

        if (tig_rect_intersection(&src_rect, a3, &src_rect) == TIG_OK) {
            dst_rect = src_rect;

            src_rect.x -= x2 - wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].width / 2;
            src_rect.y -= y2 - wmap_note_type_info[WMAP_NOTE_TYPE_WAYPOINT].height / 2;

            art_blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(wmap_ui_window, &art_blit_info);
        }

        x1 = x2;
        y1 = y2;
    }
}

// 0x566CC0
void wmap_ui_get_current_location(int64_t* loc_ptr)
{
    if (wmap_ui_created && wmap_ui_state == WMAP_UI_STATE_MOVING) {
        sub_561800(&wmap_ui_mode_info[wmap_ui_mode].field_3C, loc_ptr);
    } else {
        *loc_ptr = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    }
}

// 0x566D10
void sub_566D10(int type, WmapCoords* coords, TigRect* a3, TigRect* a4, WmapInfo* wmap_info)
{
    int dx;
    int dy;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;

    dx = coords->x - wmap_note_type_info[type].width / 2 - wmap_info->field_34;
    dy = coords->y - wmap_note_type_info[type].height / 2 - wmap_info->field_38;

    src_rect.x = a3->x + dx;
    src_rect.y = a3->y + dy;
    src_rect.width = wmap_note_type_info[type].width;
    src_rect.height = wmap_note_type_info[type].height;
    if (tig_rect_intersection(&src_rect, a4, &src_rect) == TIG_OK) {
        dst_rect = src_rect;

        src_rect.x -= a3->x + dx;
        src_rect.y -= a3->y + dy;

        art_blit_info.flags = 0;
        art_blit_info.art_id = wmap_note_type_info[type].data.art_id;
        art_blit_info.dst_video_buffer = dword_64E7F4;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(wmap_ui_window, &art_blit_info);
    }
}
