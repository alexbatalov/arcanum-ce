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
    int stolen_gold_cap;
    bool logos;
    bool intro;
    bool disable_combat_music;
    bool disable_spell_level_requirement;
    bool disable_xp_per_hit;
    bool full_xp_per_kill;
    bool hide_zero_damage_to_creatures;
    bool hide_zero_damage_to_objects;
    bool no_character_points_at_start;
    bool start_stats_at_one;
    bool magical_aptitude_bonus;
    bool technological_aptitude_bonus;
    bool starting_fate_point;
    bool disable_natural_recovery;
    bool poison_immunity;
} HighResConfig;

void highres_config_load(void);
const HighResConfig* highres_config_get(void);

#endif /* GAME_HIGHRES_CONFIG_H_ */
