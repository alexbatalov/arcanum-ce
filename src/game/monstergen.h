#ifndef ARCANUM_GAME_MONSTERGEN_H_
#define ARCANUM_GAME_MONSTERGEN_H_

#include "game/context.h"
#include "game/timeevent.h"

typedef enum GeneratorRate {
    GENERATOR_RATE_SECOND,
    GENERATOR_RATE_HALF_MINUTE,
    GENERATOR_RATE_MINUTE,
    GENERATOR_RATE_HOUR,
    GENERATOR_RATE_DAY,
    GENERATOR_RATE_WEEK,
    GENERATOR_RATE_MONTH,
    GENERATOR_RATE_YEAR,
    GENERATOR_RATE_COUNT,
} GeneratorRate;

typedef unsigned int GeneratorFlags;

#define GENERATOR_DAY 0x01u
#define GENERATOR_NIGHT 0x02u
#define GENERATOR_INACTIVE_ON_SCREEN 0x04u
#define GENERATOR_SPAWN_ALL 0x08u
#define GENERATOR_IGNORE_TOTAL 0x10u

typedef struct GeneratorInfo {
    /* 0000 */ GeneratorFlags flags;
    /* 0008 */ int64_t obj;
    /* 0010 */ int id;
    /* 0014 */ int max_concurrent;
    /* 0018 */ int max_total;
    /* 001C */ GeneratorRate rate;
    /* 0020 */ bool enabled;
    /* 0024 */ int cur_concurrent;
    /* 0028 */ int cur_total;
} GeneratorInfo;

bool monstergen_init(GameInitInfo* init_info);
void monstergen_reset(void);
void monstergen_exit(void);
void monstergen_resize(GameResizeInfo* resize_info);
bool monstergen_load(GameLoadInfo* load_info);
bool monstergen_save(TigFile* stream);
void monstergen_get(int64_t obj, GeneratorInfo* info);
bool monstergen_set(GeneratorInfo* info);
bool monstergen_remove(int64_t obj);
bool monstergen_process(int64_t obj, DateTime* datetime);
void monstergen_notify_killed(int64_t obj);
bool monstergen_is_disabled(int id);
void monstergen_enable(int id);
void monstergen_disable(int id);

#endif /* ARCANUM_GAME_MONSTERGEN_H_ */
