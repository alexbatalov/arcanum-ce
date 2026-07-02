#ifndef GAME_HIGHRES_CONFIG_H_
#define GAME_HIGHRES_CONFIG_H_

#include <stdbool.h>

typedef struct HighResConfig {
    bool loaded;
    int width;
    int height;
    bool windowed;
    bool show_fps;
    int scroll_fps;
    int scroll_dist;
    int identification_cost;
    int level_cap;
    int stats_cap;
    int speed_cap;
    int reaction_modifier_cap;
    int carry_weight_cap;
    bool logos;
    bool intro;
    bool disable_combat_music;
    bool disable_spell_level_requirement;
    bool disable_xp_per_hit;
    bool full_xp_per_kill;
    bool hide_zero_damage_to_creatures;
    bool hide_zero_damage_to_objects;
} HighResConfig;

void highres_config_load(void);
const HighResConfig* highres_config_get(void);

#endif /* GAME_HIGHRES_CONFIG_H_ */
