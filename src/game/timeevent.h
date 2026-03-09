#ifndef ARCANUM_GAME_TIMEEVENT_H_
#define ARCANUM_GAME_TIMEEVENT_H_

#include "game/context.h"
#include "game/location.h"
#include "game/obj.h"

#define TIME_STR_LENGTH 20

typedef enum TimeType {
    TIME_TYPE_REAL_TIME,
    TIME_TYPE_GAME_TIME,
    TIME_TYPE_ANIMATIONS,
    TIME_TYPE_COUNT,
} TimeType;

typedef enum TimeEventType {
    TIMEEVENT_TYPE_DEBUG,
    TIMEEVENT_TYPE_ANIM,
    TIMEEVENT_TYPE_BKG_ANIM,
    TIMEEVENT_TYPE_FIDGET_ANIM,
    TIMEEVENT_TYPE_SCRIPT,
    TIMEEVENT_TYPE_MAGICTECH,
    TIMEEVENT_TYPE_POISON,
    TIMEEVENT_TYPE_RESTING,
    TIMEEVENT_TYPE_FATIGUE,
    TIMEEVENT_TYPE_AGING,
    TIMEEVENT_TYPE_AI,
    TIMEEVENT_TYPE_COMBAT,
    TIMEEVENT_TYPE_TB_COMBAT,
    TIMEEVENT_TYPE_AMBIENT_LIGHTING,
    TIMEEVENT_TYPE_WORLDMAP,
    TIMEEVENT_TYPE_SLEEPING,
    TIMEEVENT_TYPE_CLOCK,
    TIMEEVENT_TYPE_NPC_WAIT_HERE,
    TIMEEVENT_TYPE_MAINMENU,
    TIMEEVENT_TYPE_LIGHT,
    TIMEEVENT_TYPE_MULTIPLAYER,
    TIMEEVENT_TYPE_LOCK,
    TIMEEVENT_TYPE_NPC_RESPAWN,
    TIMEEVENT_TYPE_RECHARGE_MAGIC_ITEM,
    TIMEEVENT_TYPE_DECAY_DEAD_BODIE,
    TIMEEVENT_TYPE_ITEM_DECAY,
    TIMEEVENT_TYPE_COMBAT_FOCUS_WIPE,
    TIMEEVENT_TYPE_NEWSPAPERS,
    TIMEEVENT_TYPE_TRAPS,
    TIMEEVENT_TYPE_FADE,
    TIMEEVENT_TYPE_MP_CTRL_UI,
    TIMEEVENT_TYPE_UI,
    TIMEEVENT_TYPE_TELEPORTED,
    TIMEEVENT_TYPE_SCENERY_RESPAWN,
    TIMEEVENT_TYPE_RANDOM_ENCOUNTER,
    TIMEEVENT_TYPE_COUNT,
} TimeEventType;

typedef enum TimeEventParamType {
    TIMEEVENT_PARAM_TYPE_INTEGER,
    TIMEEVENT_PARAM_TYPE_OBJECT,
    TIMEEVENT_PARAM_TYPE_LOCATION,
    TIMEEVENT_PARAM_TYPE_FLOAT,
    TIMEEVENT_PARAM_TYPE_COUNT,
} TimeEventParamType;

// NOTE: The only way to ensure proper alignment in some structs is to denote
// `DateTime` as 64-bit int. Otherwise things like `QuestInfo` and `RumorInfo`
// would have different (smaller) size. However it does not explain why some
// `DateTime` values are passed by reference, while others are passed by value.
// The storage of TS values in object system is `uint64_t`. There is also
// some code that relies on `DateTime` to be struct and I don't want to break or
// refactor it. Because of these uncertanities this type is designated as a
// union.
typedef union DateTime {
    struct {
        unsigned int days;
        unsigned int milliseconds;
    };
    uint64_t value;
} DateTime;

typedef union TimeEventParam {
    int integer_value;
    int64_t object_value;
    int64_t location_value;
    float float_value;
    void* pointer_value;
} TimeEventParam;

// Max number of params timeevent params.
#define TIMEEVENT_PARAM_COUNT 4

typedef struct TimeEvent {
    DateTime datetime;
    int type;
    TimeEventParam params[TIMEEVENT_PARAM_COUNT];
} TimeEvent;

typedef bool(TimeEventProcessFunc)(TimeEvent* timeevent);
typedef bool(TimeEventEnumerateFunc)(TimeEvent* timeevent);

typedef struct TimeEventFuncs {
    TimeEventProcessFunc* bkg_anim_func;
    TimeEventProcessFunc* worldmap_func;
    TimeEventProcessFunc* ambient_lighting_func;
    TimeEventProcessFunc* sleeping_func;
    TimeEventProcessFunc* clock_func;
    TimeEventProcessFunc* mainmenu_func;
    TimeEventProcessFunc* mp_ctrl_ui_func;
} TimeEventFuncs;

DateTime sub_45A7C0(void);
DateTime sub_45A7D0(DateTime* other);
int sub_45A7F0(void);
int sub_45A820(unsigned int milliseconds);
int datetime_seconds_since_reference_date(DateTime* datetime);
int datetime_get_hour(DateTime* datetime);
int datetime_get_hour_since_reference_date(DateTime* datetime);
int datetime_get_minute(DateTime* datetime);
int datetime_get_day_since_reference_date(void);
int datetime_get_day(DateTime* datetime);
int datetime_get_month(DateTime* datetime);
int datetime_get_year(DateTime* datetime);
void sub_45A950(DateTime* datetime, unsigned int milliseconds);
int datetime_compare(const DateTime* datetime1, const DateTime* datetime2);
bool sub_45A9B0(DateTime* datetime, unsigned int milliseconds);
void datetime_sub_milliseconds(DateTime* datetime, unsigned int milliseconds);
void datetime_add_milliseconds(DateTime* datetime, unsigned int milliseconds);
void datetime_add_datetime(DateTime* datetime, DateTime* other);
int datetime_current_hour(void);
int datetime_current_minute(void);
int datetime_current_second(void);
int datetime_current_day(void);
int datetime_current_month(void);
int datetime_current_year(void);
void datetime_format_datetime(DateTime* datetime, char* dest);
void datetime_format_time(DateTime* time, char* dest);
void datetime_format_date(DateTime* time, char* dest);
bool game_time_is_day(void);
void datetime_set_start_year(int year);
void datetime_set_start_hour(int ratio);
int sub_45AD70(void);
bool timeevent_init(GameInitInfo* init_info);
bool timeevent_set_funcs(TimeEventFuncs* funcs);
void timeevent_reset(void);
void timeevent_exit(void);
bool timeevent_save(TigFile* stream);
bool timeevent_load(GameLoadInfo* load_info);
bool sub_45B300(void);
void sub_45B320(void);
void sub_45B340(void);
void sub_45B360(void);
void timeevent_ping(tig_timestamp_t timestamp);
bool timeevent_add_delay(TimeEvent* timeevent, DateTime* delay);
bool timeevent_add_immediate(TimeEvent* timeevent);
bool timeevent_add_base(TimeEvent* timeevent, DateTime* base);
bool timeevent_add_delay_at(TimeEvent* timeevent, DateTime* delay, DateTime* at);
bool timeevent_add_delay_base_at(TimeEvent* timeevent, DateTime* delay, DateTime* base, DateTime* at);
void timeevent_clear(void);
void timeevent_clear_for_map_close(void);
bool timeevent_clear_all_typed(int list);
bool timeevent_clear_one_typed(int list);
bool timeevent_clear_all_ex(int list, TimeEventEnumerateFunc* callback);
bool timeevent_clear_one_ex(int list, TimeEventEnumerateFunc* callback);
bool sub_45C0E0(int list);
bool timeevent_any(int list, TimeEventEnumerateFunc* callback);
bool timeevent_inc_milliseconds(unsigned int milliseconds);
bool timeevent_inc_datetime(DateTime* datetime);
void timeevent_sync(DateTime* game_time, DateTime* anim_time);
void sub_45C580(void);
void timeevent_notify_pc_teleported(int map);
void timeevent_debug_lists(void);

#endif /* ARCANUM_GAME_TIMEEVENT_H_ */
