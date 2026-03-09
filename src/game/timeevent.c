#include "game/timeevent.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/gfade.h"
#include "game/item.h"
#include "game/light.h"
#include "game/magictech.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/newspaper.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/stat.h"
#include "game/teleport.h"
#include "game/trap.h"
#include "game/ui.h"

typedef unsigned int TimeEventParamTypeFlags;

#define P0_INT 0x0001u
#define P0_OBJ 0x0002u
#define P0_LOC 0x0004u
#define P1_INT 0x0008u
#define P1_OBJ 0x0010u
#define P1_LOC 0x0020u
#define P2_INT 0x0040u
#define P2_OBJ 0x0080u
#define P2_LOC 0x0100u
#define P3_INT 0x0200u
#define P3_OBJ 0x0400u
#define P3_LOC 0x0800u
#define P0_FLOAT 0x1000u
#define P1_FLOAT 0x2000u
#define P2_FLOAT 0x4000u
#define P3_FLOAT 0x8000u

typedef struct TimeEventNode {
    TimeEvent te;
    Ryan field_30[TIMEEVENT_PARAM_TYPE_COUNT];
    struct TimeEventNode* next;
    int field_D4;
} TimeEventNode;

typedef void(TimeEventExitFunc)(TimeEvent* timeevent);
typedef bool(TimeEventShouldSaveFunc)(TimeEvent* timeevent);

typedef struct TimeEventTypeInfo {
    char name[20];
    bool saveable;
    TimeEventParamTypeFlags flags;
    int time_type;
    TimeEventProcessFunc* process_func;
    TimeEventExitFunc* exit_func;
    TimeEventShouldSaveFunc* should_save_func;
} TimeEventTypeInfo;

static bool timeevent_do_nothing(TimeEvent* timeevent);
static bool timeevent_save_node(TimeEventTypeInfo* timeevent_type_info, TimeEventNode* timeevent, TigFile* stream);
static bool timeevent_load_node(TimeEvent* timeevent, TigFile* stream);
static bool timeevent_add_delay_base(TimeEvent* timeevent, DateTime* delay, DateTime* base);
static bool timeevent_add_base_at_func(TimeEvent* timeevent, DateTime* base, DateTime* at);
static TimeEventNode* timeevent_node_create(void);
static void timeevent_node_destroy(TimeEventNode* node);
static bool timeevent_recover_handles(TimeEventNode* timeevent);
static bool timeevent_recover_handles_internal(TimeEventNode* node, bool force);
static void sub_45B750(void);
static bool sub_45B7A0(TimeEventNode* node);
static bool sub_45BAF0(TimeEvent* timeevent);
static bool sub_45BB40(TimeEventNode* node);
static void timeevent_save_nodes_to_map(const char* name);
static int sub_45C500(TimeEventNode* node);
static void timeevent_load_nodes_from_map(const char* name);
static void timeevent_break_nodes_to_map(const char* name);
static bool debug_timeevent_process(TimeEvent* timeevent);
static void timeevent_debug_node(TimeEventNode* timeevent, int node);

// 0x5B2178
static const char* off_5B2178[TIME_TYPE_COUNT] = {
    /*  TIME_TYPE_REAL_TIME */ "Real-Time",
    /*  TIME_TYPE_GAME_TIME */ "Game-Time",
    /* TIME_TYPE_ANIMATIONS */ "Game-Time2 (Animations)",
};

// 0x5B2188
static TimeEventTypeInfo stru_5B2188[TIMEEVENT_TYPE_COUNT] = {
    /*               TIMEEVENT_TYPE_DEBUG */ { "Debug", false, P1_INT | P0_INT, TIME_TYPE_REAL_TIME, debug_timeevent_process, NULL, NULL },
    /*                TIMEEVENT_TYPE_ANIM */ { "Anim", true, P0_INT, TIME_TYPE_ANIMATIONS, anim_timeevent_process, NULL, NULL },
    /*            TIMEEVENT_TYPE_BKG_ANIM */ { "Bkg Anim", false, P2_INT | P1_INT | P0_INT, TIME_TYPE_REAL_TIME, timeevent_do_nothing, NULL, NULL },
    /*         TIMEEVENT_TYPE_FIDGET_ANIM */ { "Fidget Anim", false, 0, TIME_TYPE_REAL_TIME, anim_fidget_timeevent_process, NULL, NULL },
    /*              TIMEEVENT_TYPE_SCRIPT */ { "Script", true, P3_OBJ | P2_OBJ | P1_INT | P0_INT, TIME_TYPE_GAME_TIME, script_timeevent_process, NULL, NULL },
    /*           TIMEEVENT_TYPE_MAGICTECH */ { "MagicTech", true, P2_INT | P0_INT, TIME_TYPE_GAME_TIME, magictech_timeevent_process, NULL, NULL },
    /*              TIMEEVENT_TYPE_POISON */ { "Poison", true, P2_INT | P1_OBJ | P0_INT, TIME_TYPE_GAME_TIME, stat_poison_timeevent_process, NULL, NULL },
    /*             TIMEEVENT_TYPE_RESTING */ { "Resting", true, 10, TIME_TYPE_GAME_TIME, critter_resting_timeevent_process, NULL, NULL },
    /*             TIMEEVENT_TYPE_FATIGUE */ { "Fatigue", true, P2_INT | P1_OBJ | P0_INT, TIME_TYPE_GAME_TIME, critter_fatigue_timeevent_process, NULL, NULL },
    /*               TIMEEVENT_TYPE_AGING */ { "Aging", true, 0, TIME_TYPE_GAME_TIME, timeevent_do_nothing, NULL, NULL },
    /*                  TIMEEVENT_TYPE_AI */ { "AI", false, P1_INT | P0_OBJ, TIME_TYPE_GAME_TIME, ai_timeevent_process, NULL, NULL },
    /*              TIMEEVENT_TYPE_COMBAT */ { "Combat", true, 0, TIME_TYPE_GAME_TIME, timeevent_do_nothing, NULL, NULL },
    /*           TIMEEVENT_TYPE_TB_COMBAT */ { "TB Combat", true, 0, TIME_TYPE_REAL_TIME, combat_tb_timeevent_process, NULL, NULL },
    /*    TIMEEVENT_TYPE_AMBIENT_LIGHTING */ { "Ambient Lighting", true, 0, TIME_TYPE_GAME_TIME, timeevent_do_nothing, NULL, NULL },
    /*            TIMEEVENT_TYPE_WORLDMAP */ { "WorldMap", true, 0, TIME_TYPE_REAL_TIME, timeevent_do_nothing, NULL, NULL },
    /*            TIMEEVENT_TYPE_SLEEPING */ { "Sleeping", false, P0_INT, TIME_TYPE_REAL_TIME, timeevent_do_nothing, NULL, NULL },
    /*               TIMEEVENT_TYPE_CLOCK */ { "Clock", true, 0, TIME_TYPE_GAME_TIME, timeevent_do_nothing, NULL, NULL },
    /*       TIMEEVENT_TYPE_NPC_WAIT_HERE */ { "NPC Wait Here", true, P0_OBJ, TIME_TYPE_GAME_TIME, ai_npc_wait_here_timeevent_process, NULL, NULL },
    /*            TIMEEVENT_TYPE_MAINMENU */ { "MainMenu", false, P0_INT, TIME_TYPE_REAL_TIME, timeevent_do_nothing, NULL, NULL },
    /*               TIMEEVENT_TYPE_LIGHT */ { "Light", false, P1_INT | P0_INT, TIME_TYPE_ANIMATIONS, light_timeevent_process, NULL, NULL },
    /*         TIMEEVENT_TYPE_MULTIPLAYER */ { "Multiplayer", false, P0_INT, TIME_TYPE_REAL_TIME, multiplayer_timeevent_process, NULL, NULL },
    /*                TIMEEVENT_TYPE_LOCK */ { "Lock", true, P0_OBJ, TIME_TYPE_GAME_TIME, object_lock_timeevent_process, NULL, NULL },
    /*         TIMEEVENT_TYPE_NPC_RESPAWN */ { "NPC Respawn", true, P0_OBJ, TIME_TYPE_GAME_TIME, npc_respawn_timevent_process, NULL, NULL },
    /* TIMEEVENT_TYPE_RECHARGE_MAGIC_ITEM */ { "Recharge Magic-Item", true, P2_INT | P1_INT | P0_OBJ, TIME_TYPE_GAME_TIME, magictech_recharge_timeevent_process, NULL, NULL },
    /*    TIMEEVENT_TYPE_DECAY_DEAD_BODIE */ { "Decay Dead Bodie", true, P1_INT | P0_OBJ, TIME_TYPE_GAME_TIME, critter_decay_timeevent_process, NULL, NULL },
    /*          TIMEEVENT_TYPE_ITEM_DECAY */ { "Item Decay", true, P1_INT | P0_OBJ, TIME_TYPE_GAME_TIME, item_decay_timeevent_process, NULL, NULL },
    /*   TIMEEVENT_TYPE_COMBAT_FOCUS_WIPE */ { "Combat-Focus Wipe", true, P1_INT | P0_OBJ, TIME_TYPE_GAME_TIME, critter_npc_combat_focus_wipe_timeevent_process, NULL, NULL },
    /*          TIMEEVENT_TYPE_NEWSPAPERS */ { "Newspapers", true, 0, TIME_TYPE_GAME_TIME, newspaper_timeevent_process, NULL, NULL },
    /*               TIMEEVENT_TYPE_TRAPS */ { "Traps", true, P2_OBJ | P1_LOC | P0_INT, TIME_TYPE_GAME_TIME, trap_timeevent_process, NULL, NULL },
    /*                TIMEEVENT_TYPE_FADE */ { "Fade", true, P2_FLOAT | P3_INT | P1_INT | P0_INT, TIME_TYPE_GAME_TIME, gfade_timeevent_process, NULL, NULL },
    /*          TIMEEVENT_TYPE_MP_CTRL_UI */ { "MP Ctrl UI", false, 0, TIME_TYPE_REAL_TIME, NULL, NULL, NULL },
    /*                  TIMEEVENT_TYPE_UI */ { "UI", false, P1_INT | P0_INT, TIME_TYPE_REAL_TIME, ui_timeevent_process, NULL, NULL },
    /*          TIMEEVENT_TYPE_TELEPORTED */ { "Teleported", false, P0_OBJ, TIME_TYPE_GAME_TIME, object_teleported_timeevent_process, NULL, NULL },
    /*     TIMEEVENT_TYPE_SCENERY_RESPAWN */ { "Scenery Respawn", true, P0_OBJ, TIME_TYPE_GAME_TIME, object_scenery_respawn_timeevent_process, NULL, NULL },
    /*    TIMEEVENT_TYPE_RANDOM_ENCOUNTER */ { "Random Encounter", true, 0, TIME_TYPE_GAME_TIME, ui_wmap_rnd_timeevent_process, NULL, NULL },
};

// 0x5B278C
static int datetime_start_year = 1885;

// 0x5B2790
static int datetime_start_time_in_milliseconds = 36000000;

// 0x5B2794
static TimeEventParamTypeFlags dword_5B2794[TIMEEVENT_PARAM_COUNT][TIMEEVENT_PARAM_TYPE_COUNT] = {
    {
        P0_INT,
        P0_OBJ,
        P0_LOC,
        P0_FLOAT,
    },
    {
        P1_INT,
        P1_OBJ,
        P1_LOC,
        P1_FLOAT,
    },
    {
        P2_INT,
        P2_OBJ,
        P2_LOC,
        P2_FLOAT,
    },
    {
        P3_INT,
        P3_OBJ,
        P3_LOC,
        P3_FLOAT,
    },
};

// 0x5E7638
static TimeEventNode* timeevent_lists[TIME_TYPE_COUNT];

// 0x5E7E14
static TimeEventNode* timeevent_new_lists[TIME_TYPE_COUNT];

// 0x5E85F0
static bool timeevent_editor;

// 0x5E85F8
static DateTime timeevent_time[TIME_TYPE_COUNT];

#define timeevent_real_time (timeevent_time[TIME_TYPE_REAL_TIME])
#define timeevent_game_time (timeevent_time[TIME_TYPE_GAME_TIME])
#define timeevent_anim_time (timeevent_time[TIME_TYPE_ANIMATIONS])

// 0x5E8610
static tig_timestamp_t dword_5E8610;

// 0x5E8614
static bool timeevent_initialized;

// 0x5E8618
static int dword_5E8618;

// 0x5E861C
static bool timeevent_in_ping;

// 0x5E8620
static bool dword_5E8620;

// 0x5E8624
static tig_timestamp_t dword_5E8624;

// 0x5E8628
static int dword_5E8628;

// 0x5DE6E0
static bool dword_5DE6E0;

// 0x45A7C0
DateTime sub_45A7C0(void)
{
    return timeevent_game_time;
}

// TODO: Check generated assembly (return value is eax:edx without first param?)
// 0x45A7D0
DateTime sub_45A7D0(DateTime* other)
{
    DateTime new_time;

    new_time.days = timeevent_game_time.days - other->days;
    new_time.milliseconds = timeevent_game_time.milliseconds - other->milliseconds;
    return new_time;
}

// 0x45A7F0
int sub_45A7F0(void)
{
    // NOTE: Uninline.
    return datetime_seconds_since_reference_date(&timeevent_game_time);
}

// 0x45A820
int sub_45A820(unsigned int milliseconds)
{
    return datetime_seconds_since_reference_date(&timeevent_game_time) - milliseconds;
}

// 0x45A840
int datetime_seconds_since_reference_date(DateTime* datetime)
{
    return datetime->days * 86400 + datetime->milliseconds / 1000;
}

// 0x45A870
int datetime_get_hour(DateTime* datetime)
{
    return datetime->milliseconds / 3600000 % 24;
}

// 0x45A890
int datetime_get_hour_since_reference_date(DateTime* datetime)
{
    return datetime->milliseconds / 3600000 + 24 * datetime->days;
}

// 0x45A8B0
int datetime_get_minute(DateTime* datetime)
{
    return datetime->milliseconds / 60000 % 60;
}

// 0x45A8D0
int datetime_get_day_since_reference_date(void)
{
    return timeevent_game_time.days + 1;
}

// 0x45A8E0
int datetime_get_day(DateTime* datetime)
{
    return datetime->days % 30 + 1;
}

// 0x45A900
int datetime_get_month(DateTime* datetime)
{
    return datetime->days / 30 % 12 + 1;
}

// 0x45A920
int datetime_get_year(DateTime* datetime)
{
    return datetime_start_year + datetime->days / 360;
}

// 0x45A950
void sub_45A950(DateTime* datetime, unsigned int milliseconds)
{
    datetime->days = 0;
    if (milliseconds != 0) {
        datetime->milliseconds = milliseconds;
    } else {
        datetime->milliseconds = 1;
    }
}

// 0x45A970
int datetime_compare(const DateTime* datetime1, const DateTime* datetime2)
{
    if (datetime2->days > datetime1->days) return -1;
    if (datetime2->days < datetime1->days) return 1;
    if (datetime2->milliseconds > datetime1->milliseconds) return -1;
    if (datetime2->milliseconds < datetime1->milliseconds) return 1;
    return 0;
}

// 0x45A9B0
bool sub_45A9B0(DateTime* datetime, unsigned int milliseconds)
{
    DateTime other;

    sub_45A950(&other, milliseconds);
    return datetime_compare(datetime, &other) >= 0;
}

// 0x45A9E0
void datetime_sub_milliseconds(DateTime* datetime, unsigned int milliseconds)
{
    if (datetime->milliseconds < milliseconds) {
        datetime->milliseconds = milliseconds - datetime->milliseconds;
        // TODO: Looks wrong, check.
        datetime->days += datetime->milliseconds - milliseconds;
    } else {
        datetime->milliseconds -= milliseconds;
    }
}

// 0x45AA10
void datetime_add_milliseconds(DateTime* datetime, unsigned int milliseconds)
{
    datetime->milliseconds += milliseconds;
    if (datetime->milliseconds > 86400000) {
        datetime->days += datetime->milliseconds / 86400000;
        datetime->milliseconds %= 86400000;
    }
}

// 0x45AA50
void datetime_add_datetime(DateTime* datetime, DateTime* other)
{
    datetime->days += other->days;
    // NOTE: Uninline.
    datetime_add_milliseconds(datetime, other->milliseconds);
}

// 0x45AAA0
int datetime_current_hour(void)
{
    return timeevent_game_time.milliseconds / 3600000 % 24;
}

// 0x45AAC0
int datetime_current_minute(void)
{
    return timeevent_game_time.milliseconds / 60000 % 60;
}

// 0x45AAE0
int datetime_current_second(void)
{
    return timeevent_game_time.milliseconds / 1000 % 60;
}

// 0x45AB00
int datetime_current_day(void)
{
    return timeevent_game_time.days / 30 % 12 + 1;
}

// 0x45AB20
int datetime_current_month(void)
{
    return timeevent_game_time.days % 30 + 1;
}

// 0x45AB40
int datetime_current_year(void)
{
    return datetime_start_year + timeevent_game_time.days / 360;
}

// 0x45AB70
void datetime_format_datetime(DateTime* datetime, char* dest)
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    SDL_DateFormat date_format;

    if (dest != NULL) {
        year = datetime_get_year(datetime);
        month = datetime_get_month(datetime);
        day = datetime_get_day(datetime);
        hour = datetime_get_hour(datetime);
        minute = datetime_get_minute(datetime);
        second = datetime_seconds_since_reference_date(datetime) % 60;

        if (!SDL_GetDateTimeLocalePreferences(&date_format, NULL)) {
            date_format = SDL_DATE_FORMAT_MMDDYYYY;
        }

        switch (date_format) {
        case SDL_DATE_FORMAT_MMDDYYYY:
            // Month/Day/Year (default)
            sprintf(dest, "%02d:%02d:%02d %d/%d/%d", hour, minute, second, month, day, year);
            break;
        case SDL_DATE_FORMAT_DDMMYYYY:
            // Day/Month/Year
            sprintf(dest, "%02d:%02d:%02d %d/%d/%d", hour, minute, second, day, month, year);
            break;
        case SDL_DATE_FORMAT_YYYYMMDD:
            // Year/Month/Day
            sprintf(dest, "%02d:%02d:%02d %d/%d/%d", hour, minute, second, year, month, day);
            break;
        }
    }
}

// 0x45AC20
void datetime_format_time(DateTime* time, char* dest)
{
    int hour;
    int minute;
    int second;

    if (dest != NULL) {
        hour = datetime_get_hour(time);
        minute = datetime_get_minute(time);
        second = datetime_seconds_since_reference_date(time) % 60;
        sprintf(dest, "%02d:%02d:%02d", hour, minute, second);
    }
}

// 0x45AC70
void datetime_format_date(DateTime* time, char* dest)
{
    int year;
    int month;
    int day;
    SDL_DateFormat date_format;

    if (dest != NULL) {
        year = datetime_get_year(time);
        month = datetime_get_month(time);
        day = datetime_get_day(time);

        if (!SDL_GetDateTimeLocalePreferences(&date_format, NULL)) {
            date_format = SDL_DATE_FORMAT_MMDDYYYY;
        }

        switch (date_format) {
        case SDL_DATE_FORMAT_MMDDYYYY:
            // Month/Day/Year (default)
            sprintf(dest, "%d/%d/%d", month, day, year);
            break;
        case SDL_DATE_FORMAT_DDMMYYYY:
            // Day/Month/Year
            sprintf(dest, "%d/%d/%d", day, month, year);
            break;
        case SDL_DATE_FORMAT_YYYYMMDD:
            // Year/Month/Day
            sprintf(dest, "%d/%d/%d", year, month, day);
            break;
        }
    }
}

// 0x45ACF0
bool game_time_is_day(void)
{
    int hour = datetime_current_hour();
    return hour >= 6 && hour < 18;
}

// 0x45AD10
void datetime_set_start_year(int year)
{
    if (year >= 0) {
        datetime_start_year = year;
    }
}

// 0x45AD20
void datetime_set_start_hour(int hour)
{
    if (hour >= 0) {
        datetime_start_time_in_milliseconds = 3600000 * hour;
        sub_45A950(&timeevent_game_time, datetime_start_time_in_milliseconds);
        sub_45A950(&timeevent_anim_time, datetime_start_time_in_milliseconds);
    }
}

// 0x5B2790
int sub_45AD70(void)
{
    return datetime_start_time_in_milliseconds;
}

// 0x45AD80
bool timeevent_do_nothing(TimeEvent* timeevent)
{
    (void)timeevent;

    return true;
}

// 0x45AD90
bool timeevent_init(GameInitInfo* init_info)
{
    timeevent_editor = init_info->editor;

    if (!timeevent_initialized) {
        timeevent_lists[TIME_TYPE_REAL_TIME] = NULL;
        timeevent_lists[TIME_TYPE_GAME_TIME] = NULL;
        timeevent_lists[TIME_TYPE_ANIMATIONS] = NULL;

        timeevent_new_lists[TIME_TYPE_REAL_TIME] = NULL;
        timeevent_new_lists[TIME_TYPE_GAME_TIME] = NULL;
        timeevent_new_lists[TIME_TYPE_ANIMATIONS] = NULL;

        sub_45A950(&timeevent_real_time, 0);
        sub_45A950(&timeevent_game_time, datetime_start_time_in_milliseconds);
        sub_45A950(&timeevent_anim_time, datetime_start_time_in_milliseconds);

        tig_timer_now(&dword_5E8610);

        sub_45B340();

        timeevent_initialized = true;
    }

    return true;
}

// 0x45AE20
bool timeevent_set_funcs(TimeEventFuncs* funcs)
{
    if (timeevent_initialized) {
        if (funcs != NULL) {
            stru_5B2188[TIMEEVENT_TYPE_BKG_ANIM].process_func = funcs->bkg_anim_func;
            stru_5B2188[TIMEEVENT_TYPE_WORLDMAP].process_func = funcs->worldmap_func;
            stru_5B2188[TIMEEVENT_TYPE_AMBIENT_LIGHTING].process_func = funcs->ambient_lighting_func;
            stru_5B2188[TIMEEVENT_TYPE_SLEEPING].process_func = funcs->sleeping_func;
            stru_5B2188[TIMEEVENT_TYPE_CLOCK].process_func = funcs->clock_func;
            stru_5B2188[TIMEEVENT_TYPE_MAINMENU].process_func = funcs->mainmenu_func;
            stru_5B2188[TIMEEVENT_TYPE_MP_CTRL_UI].process_func = funcs->mp_ctrl_ui_func;
        }
    }

    return true;
}

// 0x45AE80
void timeevent_reset(void)
{
    timeevent_clear();
    sub_45A950(&timeevent_real_time, 0);
    sub_45A950(&timeevent_game_time, datetime_start_time_in_milliseconds);
    sub_45A950(&timeevent_anim_time, datetime_start_time_in_milliseconds);
    tig_timer_now(&dword_5E8610);
}

// 0x45AEC0
void timeevent_exit(void)
{
    timeevent_initialized = false;
    timeevent_clear();
}

// 0x45AED0
bool timeevent_save(TigFile* stream)
{
    int index;
    int count_pos;
    int count;
    TimeEventNode* timeevent;
    int pos;
    TimeEventTypeInfo* info;

    if (tig_file_fwrite(&timeevent_real_time, sizeof(timeevent_real_time), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&timeevent_game_time, sizeof(timeevent_game_time), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&timeevent_anim_time, sizeof(timeevent_anim_time), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < TIME_TYPE_COUNT; index++) {
        count = 0;
        if (tig_file_fgetpos(stream, &count_pos) != 0) {
            return false;
        }

        if (tig_file_fwrite(&count, sizeof(count), 1, stream) != 1) {
            return false;
        }

        timeevent = timeevent_lists[index];
        while (timeevent != NULL) {
            info = &(stru_5B2188[timeevent->te.type]);
            // NOTE: Original code is slightly different. It uses bitwise AND
            // with 0x1 implying `saveable` is a bitfield.
            if (info->saveable) {
                if (info->should_save_func == NULL || info->should_save_func(&(timeevent->te))) {
                    if (!timeevent_save_node(info, timeevent, stream)) {
                        return false;
                    }

                    count++;
                }
            }
            timeevent = timeevent->next;
        }

        if (tig_file_fgetpos(stream, &pos) != 0) {
            return false;
        }

        if (tig_file_fsetpos(stream, &count_pos) != 0) {
            return false;
        }

        if (tig_file_fwrite(&count, sizeof(count), 1, stream) != 1) {
            return false;
        }

        if (tig_file_fsetpos(stream, &pos) != 0) {
            return false;
        }
    }

    return true;
}

// 0x45B050
bool timeevent_save_node(TimeEventTypeInfo* timeevent_type_info, TimeEventNode* node, TigFile* stream)
{
    int index;

    if (stream == NULL) {
        return false;
    }

    if (tig_file_fwrite(&(node->te.datetime), sizeof(node->te.datetime), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(node->te.type), sizeof(node->te.type), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_INTEGER] & timeevent_type_info->flags) != 0) {
            if (tig_file_fwrite(&(node->te.params[index].integer_value), sizeof(node->te.params[index].integer_value), 1, stream) != 1) {
                return false;
            }
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & timeevent_type_info->flags) != 0) {
            if (!object_save_obj_handle_safe(&(node->te.params[index].object_value), &(node->field_30[index]), stream)) {
                return false;
            }
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_LOCATION] & timeevent_type_info->flags) != 0) {
            if (tig_file_fwrite(&(node->te.params[index].location_value), sizeof(node->te.params[index].location_value), 1, stream) != 1) {
                return false;
            }
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_FLOAT] & timeevent_type_info->flags) != 0) {
            if (tig_file_fwrite(&(node->te.params[index].float_value), sizeof(node->te.params[index].float_value), 1, stream) != 1) {
                return false;
            }
        }
    }

    return true;
}

// 0x45B120
bool timeevent_load(GameLoadInfo* load_info)
{
    int time_type;
    int count;
    int index;
    TimeEvent timeevent;

    if (load_info->stream == NULL) {
        return false;
    }

    if (tig_file_fread(&timeevent_real_time, sizeof(timeevent_real_time), 1, load_info->stream) != 1) {
        return false;
    }

    if (tig_file_fread(&timeevent_game_time, sizeof(timeevent_game_time), 1, load_info->stream) != 1) {
        return false;
    }

    if (tig_file_fread(&timeevent_anim_time, sizeof(timeevent_anim_time), 1, load_info->stream) != 1) {
        return false;
    }

    for (time_type = 0; time_type < TIME_TYPE_COUNT; time_type++) {
        if (tig_file_fread(&count, sizeof(count), 1, load_info->stream) != 1) {
            return false;
        }

        for (index = 0; index < count; index++) {
            if (!timeevent_load_node(&timeevent, load_info->stream)) {
                return false;
            }

            if (!sub_45BAF0(&timeevent)) {
                return false;
            }
        }
    }

    return true;
}

// 0x45B200
bool timeevent_load_node(TimeEvent* timeevent, TigFile* stream)
{
    int index;
    int64_t obj;

    if (stream == NULL) {
        return false;
    }

    if (tig_file_fread(&(timeevent->datetime), sizeof(timeevent->datetime), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fread(&(timeevent->type), sizeof(timeevent->type), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_INTEGER] & stru_5B2188[timeevent->type].flags) != 0) {
            if (tig_file_fread(&(timeevent->params[index].integer_value), sizeof(timeevent->params[index].integer_value), 1, stream) != 1) {
                return false;
            }
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & stru_5B2188[timeevent->type].flags) != 0) {
            if (!object_load_obj_handle_safe(&obj, 0, stream)) {
                return false;
            }
            timeevent->params[index].object_value = obj;
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_LOCATION] & stru_5B2188[timeevent->type].flags) != 0) {
            if (tig_file_fread(&(timeevent->params[index].location_value), sizeof(timeevent->params[index].location_value), 1, stream) != 1) {
                return false;
            }
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_FLOAT] & stru_5B2188[timeevent->type].flags) != 0) {
            if (tig_file_fread(&(timeevent->params[index].float_value), sizeof(timeevent->params[index].float_value), 1, stream) != 1) {
                return false;
            }
        }
    }

    return true;
}

// 0x45B300
bool sub_45B300(void)
{
    if (dword_5E8618 == 0) {
        return sub_52A900();
    } else {
        return true;
    }
}

// 0x45B320
void sub_45B320(void)
{
    if (dword_5E8618 < 30) {
        dword_5E8618++;
        dword_5DE6E0 = true;
    }
}

// 0x45B340
void sub_45B340(void)
{
    if (dword_5E8618 > 0) {
        dword_5E8618--;
        if (dword_5E8618 == 0) {
            dword_5DE6E0 = false;
        }
    }
}

// 0x45B360
void sub_45B360(void)
{
    dword_5E8618 = 0;
    dword_5DE6E0 = false;
}

// 0x45B370
void timeevent_ping(tig_timestamp_t timestamp)
{
    tig_duration_t delta;
    int time_type;
    DateTime* datetime;
    TimeEventNode* node;
    TimeEventTypeInfo* info;
    int iter = 0;
    TimeEventNode offending_node_candidate;

    delta = tig_timer_between(dword_5E8610, timestamp);
    if (delta < 5) {
        return;
    }

    dword_5E8610 = timestamp;

    if (delta > 250) {
        delta = 250;
    }

    timeevent_in_ping = true;
    datetime_add_milliseconds(&timeevent_real_time, delta);

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        if (!sub_45B300()) {
            if (dword_5E8628 < 1000 * (sub_4A38A0() + 16)) {
                datetime_add_milliseconds(&timeevent_game_time, 8 * delta);
                datetime_add_milliseconds(&timeevent_anim_time, 8 * delta);
                dword_5E8628 += 8 * delta;
            }
        }
    } else {
        if (!sub_45B300()) {
            if (!combat_turn_based_is_active()) {
                datetime_add_milliseconds(&timeevent_game_time, 8 * delta);
            }
            datetime_add_milliseconds(&timeevent_anim_time, 8 * delta);
        }
    }

    for (time_type = 0; time_type < TIME_TYPE_COUNT; time_type++) {
        switch (time_type) {
        case TIME_TYPE_REAL_TIME:
            datetime = &timeevent_real_time;
            break;
        case TIME_TYPE_GAME_TIME:
            datetime = &timeevent_game_time;
            break;
        case TIME_TYPE_ANIMATIONS:
            datetime = &timeevent_anim_time;
            break;
        default:
            // Should be unreachable.
            assert(0);
        }

        // TimeEventNode objects are sorted by their datetime, so we are only
        // interested in head node.
        while ((node = timeevent_lists[time_type]) != NULL
            && datetime_compare(datetime, &(node->te.datetime)) >= 0) {
            timeevent_lists[time_type] = node->next;

            info = &(stru_5B2188[node->te.type]);

            if (timeevent_recover_handles(node)) {
                // Save current node for debug purposes. In case infinite loop
                // is detected (see below), this is the offending node which
                // either creates too many timeevents or spams one timeevent for
                // immediate processing.
                offending_node_candidate = *node;

                info->process_func(&(node->te));
            }

            // Give user code a chance for cleanup.
            //
            // NOTE: This is useless, there are no cleanup functions.
            if (info->exit_func != NULL) {
                info->exit_func(&(node->te));
            }

            timeevent_node_destroy(node);

            // Attempt to detect infinite loop using simple counter. Original
            // devs assumed 500 events is just too much for one tick to be true.
            // This might happen if process function enqueue new timeevents for
            // immediate processing.
            if (++iter > 500) {
                tig_debug_printf("TimeEvent: timeevent_ping: ERROR: Suspected Infinite Loop Caught: Last Type: %s!\n", info->name);
                timeevent_debug_node(&offending_node_candidate, -1);
                iter = 0;
                break;
            }
        }
    }

    timeevent_in_ping = false;

    sub_45B750();

    if (tig_net_is_active()
        && tig_net_is_host()
        && tig_timer_between(dword_5E8624, timestamp) > 950) {
        PacketGameTime pkt;

        dword_5E8624 = timestamp;

        pkt.type = 3;
        pkt.game_time = timeevent_game_time.value;
        pkt.anim_time = timeevent_anim_time.value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x45B600
void timeevent_node_destroy(TimeEventNode* node)
{
    FREE(node);
}

// 0x45B610
bool timeevent_recover_handles(TimeEventNode* timeevent)
{
    return timeevent_recover_handles_internal(timeevent, false);
}

// 0x45B620
bool timeevent_recover_handles_internal(TimeEventNode* node, bool force)
{
    int index;
    TimeEventTypeInfo* info;
    int64_t obj;

    info = &(stru_5B2188[node->te.type]);

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if (node->field_30[index].objid.type != OID_TYPE_NULL) {
            obj = node->te.params[index].object_value;
            if (obj != OBJ_HANDLE_NULL || force) {
                if (map_is_clearing_objects()) {
                    return false;
                }

                if (obj == OBJ_HANDLE_NULL
                    || !obj_handle_is_valid(obj)) {
                    if (!sub_443F80(&obj, &(node->field_30[index]))) {
                        node->te.params[index].object_value = OBJ_HANDLE_NULL;
                        tig_debug_printf("TimeEvent: ERROR: Object validate recovery FAILED: TE-Type: %s!\n", info->name);
                        return false;
                    }
                    node->te.params[index].object_value = obj;
                }

                if (obj == OBJ_HANDLE_NULL
                    || (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
                    return false;
                }
            }
        } else {
            if ((info->flags & dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT]) != 0) {
                node->te.params[index].object_value = OBJ_HANDLE_NULL;
            }
        }
    }

    return true;
}

// 0x45B750
void sub_45B750(void)
{
    int index;
    TimeEventNode** node_ptr;
    TimeEventNode* node;

    for (index = 0; index < TIME_TYPE_COUNT; index++) {
        node_ptr = &(timeevent_new_lists[index]);
        while (*node_ptr != NULL) {
            node = *node_ptr;
            *node_ptr = node->next;
            if (sub_45B7A0(node)) {
                sub_45BB40(node);
            } else {
                timeevent_node_destroy(node);
            }
        }
    }
}

// 0x45B7A0
bool sub_45B7A0(TimeEventNode* node)
{
    TimeEventTypeInfo* timeevent_type_info;
    int index;

    timeevent_type_info = &(stru_5B2188[node->te.type]);
    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & timeevent_type_info->flags) != 0) {
            if (!obj_handle_is_valid(node->te.params[index].object_value)) {
                return false;
            }
        }
    }

    return true;
}

// 0x45B800
bool timeevent_add_delay(TimeEvent* timeevent, DateTime* delay)
{
    return timeevent_add_delay_base(timeevent, delay, NULL);
}

// 0x45B820
bool timeevent_add_immediate(TimeEvent* timeevent)
{
    DateTime datetime;
    bool rc;

    dword_5E8620 = true;

    sub_45A950(&datetime, 0);
    rc = timeevent_add_delay(timeevent, &datetime);

    dword_5E8620 = false;

    return rc;
}

// 0x45B860
bool timeevent_add_base(TimeEvent* timeevent, DateTime* base)
{
    return timeevent_add_base_at_func(timeevent, base, NULL);
}

// 0x45B880
bool timeevent_add_delay_at(TimeEvent* timeevent, DateTime* delay, DateTime* at)
{
    return timeevent_add_delay_base_at(timeevent, delay, NULL, at);
}

// 0x45B8A0
bool timeevent_add_delay_base(TimeEvent* timeevent, DateTime* delay, DateTime* base)
{
    return timeevent_add_delay_base_at(timeevent, delay, base, NULL);
}

// 0x45B8C0
bool timeevent_add_base_at_func(TimeEvent* timeevent, DateTime* base, DateTime* at)
{
    TimeEventNode** node_ptr;
    TimeEventNode* node;
    int time_type;
    int index;

    if (timeevent == NULL) {
        return false;
    }

    if (timeevent->type >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    if (timeevent_editor) {
        return false;
    }

    node = timeevent_node_create();
    if (node == NULL) {
        tig_debug_printf("timeevent_add_base_offset_at_func: Error: failed to malloc node!\n");
        exit(EXIT_FAILURE);
    }

    timeevent->datetime = *base;

    time_type = stru_5B2188[timeevent->type].time_type;
    if (!timeevent_in_ping || dword_5E8620) {
        node_ptr = &(timeevent_lists[time_type]);
    } else {
        node_ptr = &(timeevent_new_lists[time_type]);
    }

    while (*node_ptr != NULL) {
        if (datetime_compare(&(timeevent->datetime), &((*node_ptr)->te.datetime)) <= 0) {
            break;
        }

        node_ptr = &((*node_ptr)->next);
    }

    node->next = *node_ptr;
    node->te = *timeevent;

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & stru_5B2188[timeevent->type].flags) != 0) {
            sub_443EB0(timeevent->params[index].object_value, &(node->field_30[index]));
        } else {
            node->field_30[index].objid.type = OID_TYPE_NULL;
        }
    }

    *node_ptr = node;

    if (at != NULL) {
        *at = timeevent->datetime;
    }

    return true;
}

// 0x45BA20
TimeEventNode* timeevent_node_create(void)
{
    return (TimeEventNode*)MALLOC(sizeof(TimeEventNode));
}

// 0x45BA30
bool timeevent_add_delay_base_at(TimeEvent* timeevent, DateTime* delay, DateTime* base, DateTime* at)
{
    DateTime time;

    if (timeevent == NULL) {
        return false;
    }

    if (timeevent->type >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    if (base != NULL && (base->days != 0 || base->milliseconds != 0)) {
        time = *base;
    } else {
        switch (stru_5B2188[timeevent->type].time_type) {
        case TIME_TYPE_REAL_TIME:
            time = timeevent_real_time;
            break;
        case TIME_TYPE_GAME_TIME:
            time = timeevent_game_time;
            break;
        default:
            // FIXME: Use `TIME_TYPE_ANIMATIONS` explicitly.
            time = timeevent_anim_time;
            break;
        }
    }

    datetime_add_datetime(&time, delay);
    return timeevent_add_base_at_func(timeevent, &time, at);
}

// 0x45BAF0
bool sub_45BAF0(TimeEvent* timeevent)
{
    TimeEventNode* node;

    if (timeevent == NULL) {
        return false;
    }

    if (timeevent->type >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    if (timeevent_editor) {
        return false;
    }

    node = timeevent_node_create();
    node->te = *timeevent;
    node->next = NULL;
    // NOTE: Unsafe, leaking memory if underlying function fails.
    return sub_45BB40(node);
}

// 0x45BB40
bool sub_45BB40(TimeEventNode* node)
{
    TimeEventNode** node_ptr;
    int time_type;
    int index;

    if (node == NULL) {
        return false;
    }

    time_type = stru_5B2188[node->te.type].time_type;
    if (timeevent_in_ping) {
        node_ptr = &(timeevent_new_lists[time_type]);
    } else {
        node_ptr = &(timeevent_lists[time_type]);
    }

    while (*node_ptr != NULL) {
        if (datetime_compare(&(node->te.datetime), &((*node_ptr)->te.datetime)) <= 0) {
            break;
        }
        node_ptr = &((*node_ptr)->next);
    }

    node->next = *node_ptr;

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & stru_5B2188[node->te.type].flags) != 0) {
            sub_443EB0(node->te.params[index].object_value, &(node->field_30[index]));
        } else {
            node->field_30[index].objid.type = OID_TYPE_NULL;
        }
    }

    *node_ptr = node;

    return true;
}

// 0x45BC20
void timeevent_clear(void)
{
    int index;
    TimeEventNode* node;

    for (index = 0; index < TIME_TYPE_COUNT; index++) {
        while (timeevent_lists[index] != NULL) {
            node = timeevent_lists[index];
            timeevent_lists[index] = node->next;

            if (stru_5B2188[node->te.type].exit_func != NULL) {
                stru_5B2188[node->te.type].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);
        }

        while (timeevent_new_lists[index] != NULL) {
            node = timeevent_new_lists[index];
            timeevent_new_lists[index] = node->next;

            if (stru_5B2188[node->te.type].exit_func != NULL) {
                stru_5B2188[node->te.type].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);
        }
    }
}

// 0x45BCE0
void timeevent_clear_for_map_close(void)
{
    char* name;

    anim_goal_interrupt_all_goals();

    timeevent_clear_all_typed(TIMEEVENT_TYPE_ANIM);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_MAGICTECH);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_AI);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_COMBAT);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_TB_COMBAT);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_WORLDMAP);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_TELEPORTED);

    if (teleport_is_teleporting()) {
        if (map_get_name(map_current_map(), &name)) {
            timeevent_save_nodes_to_map(name);
            anim_save_nodes_to_map(name);
            magictech_save_nodes_to_map(name);
        } else {
            tig_debug_printf("TimeEvent: timeevent_clear_for_map_close: ERROR: Failed to find map name!\n");
        }
    }
}

// 0x45BD70
bool timeevent_clear_all_typed(int list)
{
    TimeEventNode** node_ptr;
    TimeEventNode* node;

    if (list >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    node_ptr = &(timeevent_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);
        } else {
            node_ptr = &(node->next);
        }
    }

    node_ptr = &(timeevent_new_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);
        } else {
            node_ptr = &(node->next);
        }
    }

    return true;
}

// 0x45BE40
bool timeevent_clear_one_typed(int list)
{
    TimeEventNode** node_ptr;
    TimeEventNode* node;

    if (list >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    node_ptr = &(timeevent_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);

            break;
        }

        node_ptr = &(node->next);
    }

    node_ptr = &(timeevent_new_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);

            break;
        }

        node_ptr = &(node->next);
    }

    return true;
}

// 0x45BF10
bool timeevent_clear_all_ex(int list, TimeEventEnumerateFunc* callback)
{
    TimeEventNode** node_ptr;
    TimeEventNode* node;

    if (list >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    node_ptr = &(timeevent_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list && callback(&(node->te))) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);
        } else {
            node_ptr = &(node->next);
        }
    }

    node_ptr = &(timeevent_new_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list && callback(&(node->te))) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);
        } else {
            node_ptr = &(node->next);
        }
    }

    return true;
}

// 0x45BFF0
bool timeevent_clear_one_ex(int list, TimeEventEnumerateFunc* callback)
{
    TimeEventNode** node_ptr;
    TimeEventNode* node;

    if (list >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    node_ptr = &(timeevent_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list && callback(&(node->te))) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);

            break;
        }

        node_ptr = &(node->next);
    }

    node_ptr = &(timeevent_new_lists[stru_5B2188[list].time_type]);
    while (*node_ptr != NULL) {
        node = *node_ptr;
        if (node->te.type == list && callback(&(node->te))) {
            *node_ptr = node->next;

            if (stru_5B2188[list].exit_func != NULL) {
                stru_5B2188[list].exit_func(&(node->te));
            }

            timeevent_node_destroy(node);

            break;
        }

        node_ptr = &(node->next);
    }

    return true;
}

// 0x45C0E0
bool sub_45C0E0(int list)
{
    TimeEventNode* node;

    if (list >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    node = timeevent_new_lists[stru_5B2188[list].time_type];
    while (node != NULL) {
        if (node->te.type == list) {
            return true;
        }

        node = node->next;
    }

    node = timeevent_lists[stru_5B2188[list].time_type];
    while (node != NULL) {
        if (node->te.type == list) {
            return true;
        }

        node = node->next;
    }

    return false;
}

// 0x45C140
bool timeevent_any(int list, TimeEventEnumerateFunc* callback)
{
    TimeEventNode* node;

    if (list >= TIMEEVENT_TYPE_COUNT) {
        return false;
    }

    node = timeevent_new_lists[stru_5B2188[list].time_type];
    while (node != NULL) {
        if (node->te.type == list && callback(&(node->te))) {
            return true;
        }

        node = node->next;
    }

    node = timeevent_lists[stru_5B2188[list].time_type];
    while (node != NULL) {
        if (node->te.type == list && callback(&(node->te))) {
            return true;
        }

        node = node->next;
    }

    return false;
}

// 0x45C1C0
bool timeevent_inc_milliseconds(unsigned int milliseconds)
{
    DateTime datetime;

    sub_45A950(&datetime, milliseconds);
    // NOTE: Uninline.
    return timeevent_inc_datetime(&datetime);
}

// 0x45C200
bool timeevent_inc_datetime(DateTime* datetime)
{
    datetime_add_datetime(&timeevent_game_time, datetime);
    datetime_add_datetime(&timeevent_anim_time, datetime);

    return true;
}

// 0x45C230
void timeevent_sync(DateTime* game_time, DateTime* anim_time)
{
    dword_5E8628 = 0;

    if (datetime_compare(game_time, &timeevent_game_time) > 0) {
        timeevent_game_time = *game_time;
    }

    if (datetime_compare(anim_time, &timeevent_anim_time) > 0) {
        timeevent_anim_time = *anim_time;
    }
}

// 0x45C2F0
void timeevent_save_nodes_to_map(const char* name)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    bool exists = false;
    int count;
    int time_type;
    TimeEventNode* node;
    TimeEventNode** node_ptr;

    sprintf(path, "Save\\Current\\maps\\%s\\TimeEvent.dat", name);

    if (tig_file_exists(path, NULL)) {
        exists = true;
        stream = tig_file_fopen(path, "r+b");
    } else {
        stream = tig_file_fopen(path, "wb");
    }

    if (stream == NULL) {
        tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Couldn't create TimeEvent data file for map!\n");
        return;
    }

    count = 0;
    if (exists) {
        if (tig_file_fseek(stream, 0, SEEK_SET) != 0) {
            tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Seeking to start of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fread(&count, sizeof(count), 1, stream) != 1) {
            tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Reading Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fseek(stream, 0, SEEK_END) != 0) {
            tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Seeking to end of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    } else {
        if (tig_file_fwrite(&count, sizeof(count), 1, stream) != 1) {
            tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    }

    for (time_type = 0; time_type < TIME_TYPE_COUNT; time_type++) {
        node_ptr = &(timeevent_lists[time_type]);
        while (*node_ptr != NULL) {
            node = *node_ptr;
            if (sub_45C500(node) < 0) {
                *node_ptr = node->next;

                if (!timeevent_save_node(&(stru_5B2188[node->te.type]), node, stream)) {
                    tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Failed to save out nodes!\n");
                    tig_file_fclose(stream);

                    // FIXME: Other error-handling code does not remove this
                    // file.
                    tig_file_remove(path);

                    return;
                }

                count++;

                if (stru_5B2188[node->te.type].exit_func != NULL) {
                    stru_5B2188[node->te.type].exit_func(&(node->te));
                }

                timeevent_node_destroy(node);
            } else {
                node_ptr = &(node->next);
            }
        }
    }

    if (tig_file_fseek(stream, 0, SEEK_SET) != 0) {
        tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    if (tig_file_fwrite(&count, sizeof(count), 1, stream) != 1) {
        tig_debug_printf("TimeEvent: timeevent_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    tig_file_fclose(stream);
}

// 0x45C500
int sub_45C500(TimeEventNode* node)
{
    int count1 = 0;
    int count2 = 0;
    int index;

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & stru_5B2188[node->te.type].flags) != 0) {
            if (teleport_is_teleporting_obj(node->te.params[index].object_value)) {
                count1++;
            } else {
                count2++;
            }
        }
    }

    if (count2 > 0) {
        return -1;
    } else if (count1 > 0) {
        return 1;
    } else {
        return 0;
    }
}

// 0x45C580
void sub_45C580(void)
{
    int time_type;
    TimeEventNode* node;
    char* name;

    for (time_type = 0; time_type < TIME_TYPE_COUNT; time_type++) {
        node = timeevent_lists[time_type];
        while (node != NULL) {
            timeevent_recover_handles_internal(node, true);
            node = node->next;
        }

        node = timeevent_new_lists[time_type];
        while (node != NULL) {
            timeevent_recover_handles_internal(node, true);
            node = node->next;
        }
    }

    if (map_get_name(map_current_map(), &name)) {
        timeevent_load_nodes_from_map(name);
        anim_load_nodes_from_map(name);
        magictech_load_nodes_from_map(name);
    }
}

// 0x45C610
void timeevent_load_nodes_from_map(const char* name)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int count;
    int index;
    TimeEvent timeevent;

    sprintf(path, "Save\\Current\\maps\\%s\\TimeEvent.dat", name);

    if (!tig_file_exists(path, NULL)) {
        return;
    }

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        tig_debug_printf("TimeEvent: timeevent_load_nodes_from_map: ERROR: Couldn't open TimeEvent data file for map!\n");
        return;
    }

    count = 0;
    if (tig_file_fread(&count, sizeof(count), 1, stream) != 1) {
        // FIX: Typo.
        tig_debug_printf("TimeEvent: timeevent_load_nodes_from_map: ERROR: Reading Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    for (index = 0; index < count; index++) {
        if (!timeevent_load_node(&timeevent, stream)) {
            break;
        }

        if (!sub_45BAF0(&timeevent)) {
            break;
        }
    }

    tig_file_fclose(stream);

    if (index < count) {
        tig_debug_printf("TimeEvent: timeevent_load_nodes_from_map: ERROR: Failed to load all nodes!\n");
    }

    tig_file_remove(path);
}

// 0x45C720
void timeevent_notify_pc_teleported(int map)
{
    char* name;
    char path[TIG_MAX_PATH];

    timeevent_clear_all_typed(TIMEEVENT_TYPE_ANIM);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_MAGICTECH);

    if (map_get_name(map, &name)) {
        sprintf(path, "Save\\Current\\maps\\%s", name);

        if (!tig_file_is_directory(path)) {
            tig_file_mkdir(path);
        }

        magictech_break_nodes_to_map(name);
        anim_break_nodes_to_map(name);
        timeevent_break_nodes_to_map(name);
    }
}

// 0x45C7B0
void timeevent_break_nodes_to_map(const char* name)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    bool exists = false;
    int count;
    int time_type;
    TimeEventNode* node;
    TimeEventNode** node_ptr;

    sprintf(path, "Save\\Current\\maps\\%s\\TimeEvent.dat", name);

    if (tig_file_exists(path, NULL)) {
        exists = true;
        stream = tig_file_fopen(path, "r+b");
    } else {
        stream = tig_file_fopen(path, "wb");
    }

    if (stream == NULL) {
        tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Couldn't create TimeEvent data file for map!\n");
        return;
    }

    count = 0;
    if (exists) {
        if (tig_file_fseek(stream, 0, SEEK_SET) != 0) {
            tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Seeking to start of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fread(&count, sizeof(count), 1, stream) != 1) {
            tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Reading Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fseek(stream, 0, SEEK_END) != 0) {
            tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Seeking to end of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    } else {
        if (tig_file_fwrite(&count, sizeof(count), 1, stream) != 1) {
            tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Writing Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    }

    for (time_type = 0; time_type < TIME_TYPE_COUNT; time_type++) {
        node_ptr = &(timeevent_lists[time_type]);
        while (*node_ptr != NULL) {
            node = *node_ptr;
            if (sub_45C500(node) > 0) {
                *node_ptr = node->next;

                if (!timeevent_save_node(&(stru_5B2188[node->te.type]), node, stream)) {
                    tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Failed to save out nodes!\n");
                    tig_file_fclose(stream);

                    // FIXME: Other error-handling code does not remove this
                    // file.
                    tig_file_remove(path);

                    return;
                }

                count++;

                if (stru_5B2188[node->te.type].exit_func != NULL) {
                    stru_5B2188[node->te.type].exit_func(&(node->te));
                }

                timeevent_node_destroy(node);
            } else {
                node_ptr = &(node->next);
            }
        }
    }

    if (tig_file_fseek(stream, 0, SEEK_SET) != 0) {
        tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Writing Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    if (tig_file_fwrite(&count, sizeof(count), 1, stream) != 1) {
        tig_debug_printf("TimeEvent: timeevent_break_nodes_to_map: ERROR: Writing Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    tig_file_fclose(stream);
}

// 0x45CA00
bool debug_timeevent_process(TimeEvent* timeevent)
{
    tig_timestamp_t now;
    tig_duration_t delta;

    tig_timer_now(&now);

    delta = tig_timer_between(timeevent->params[0].integer_value, now);
    tig_debug_printf("TimeEvent: t:%d(%d), deviation: %3.2f%%\n",
        delta,
        timeevent->params[1].integer_value,
        ((double)delta / (double)timeevent->params[1].integer_value - 1.0) * 100.0);

    return true;
}

// 0x45CA60
void timeevent_debug_lists(void)
{
    TimeEventNode* node;
    int time_type_counts[TIME_TYPE_COUNT];
    int timeevent_type_counts[TIMEEVENT_TYPE_COUNT];
    char time_str[TIME_STR_LENGTH];
    int index;
    DateTime time;

    tig_debug_printf("\n\nTimeEvent DEBUG Lists:\n");
    tig_debug_printf("----------------------\n\n");

    memset(time_type_counts, 0, sizeof(time_type_counts));
    memset(timeevent_type_counts, 0, sizeof(timeevent_type_counts));

    for (index = 0; index < TIME_TYPE_COUNT; index++) {
        switch (index) {
        case TIME_TYPE_REAL_TIME:
            time = timeevent_real_time;
            break;
        case TIME_TYPE_GAME_TIME:
            time = timeevent_game_time;
            break;
        case TIME_TYPE_ANIMATIONS:
            time = timeevent_anim_time;
            break;
        default:
            // NOTE: Unreachable, switch is exhaustive.
            tig_debug_printf("TimeEvent: timeevent_debug_lists: ERROR: Out-of-Bounds Time Type!\n");
            time = timeevent_game_time;
            break;
        }

        datetime_format_datetime(&time, time_str);
        tig_debug_printf("\t[%s] Game Time: [%s]\n", off_5B2178[index], time_str);

        node = timeevent_new_lists[index];
        while (node != NULL) {
            time_type_counts[index]++;
            timeevent_type_counts[node->te.type]++;
            timeevent_debug_node(node, time_type_counts[index]);
            node = node->next;
        }

        node = timeevent_lists[index];
        while (node != NULL) {
            time_type_counts[index]++;
            timeevent_type_counts[node->te.type]++;
            timeevent_debug_node(node, time_type_counts[index]);
            node = node->next;
        }
    }

    tig_debug_printf("\n\nTimeEvent List Counts:\n");

    for (index = 0; index < TIME_TYPE_COUNT; index++) {
        tig_debug_printf("   List [%s]: %d\n",
            off_5B2178[index],
            time_type_counts[index]);
    }

    tig_debug_printf("\nTimeEvent Type Counts:\n");

    for (index = 0; index < TIMEEVENT_TYPE_COUNT; index++) {
        tig_debug_printf("   Type [%s]: %d\n",
            stru_5B2188[index].name,
            timeevent_type_counts[index]);
    }
}

// 0x45CC10
void timeevent_debug_node(TimeEventNode* node, int node_index)
{
    // 0x5E7E20
    static char object_name[MAX_STRING];

    char time_str[TIME_STR_LENGTH];
    int index;

    datetime_format_datetime(&(node->te.datetime), time_str);
    tig_debug_printf("TimeEvent: DBG: Node(%d): Type: [%s], Time: [%s]", node_index, stru_5B2188[node->te.type].name, time_str);

    for (index = 0; index < TIMEEVENT_PARAM_COUNT; index++) {
        if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_INTEGER] & stru_5B2188[node->te.type].flags) != 0) {
            tig_debug_printf(", D%d[%d]val",
                index,
                node->te.params[index].integer_value);
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_OBJECT] & stru_5B2188[node->te.type].flags) != 0) {
            if (obj_handle_is_valid(node->te.params[index].object_value)) {
                if (node->te.params[index].object_value != OBJ_HANDLE_NULL) {
                    object_examine(node->te.params[index].object_value,
                        node->te.params[index].object_value,
                        object_name);
                } else {
                    strcpy(object_name, "<OBJ_HANDLE_NULL>");
                }
            } else {
                strcpy(object_name, "<INVALIDATED OBJ>");
            }
            tig_debug_printf(", D%d[%s(%I64d)]obj",
                index,
                object_name,
                node->te.params[index].object_value);
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_LOCATION] & stru_5B2188[node->te.type].flags) != 0) {
            tig_debug_printf(", D%d[%d x %d]loc",
                index,
                LOCATION_GET_X(node->te.params[index].location_value),
                LOCATION_GET_Y(node->te.params[index].location_value));
        } else if ((dword_5B2794[index][TIMEEVENT_PARAM_TYPE_FLOAT] & stru_5B2188[node->te.type].flags) != 0) {
            tig_debug_printf(", D%d[%f]loc",
                index,
                node->te.params[index].float_value);
        }
    }

    tig_debug_printf("\n");
}
