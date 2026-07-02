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
    bool disable_sprite_mirroring;
    bool hide_zero_damage_to_creatures;
    bool hide_zero_damage_to_objects;
    bool no_character_points_at_start;
    bool start_stats_at_one;
    bool magical_aptitude_bonus;
    bool technological_aptitude_bonus;
    bool starting_fate_point;
    bool disable_natural_recovery;
    bool poison_immunity;
    bool disable_prowling_to_hit_bonus;
    bool disable_aptitude_to_hit_penalty;
    bool technological_cursor;
    bool disable_global_lighting_change;
    bool free_buying;
    bool free_selling;
    bool endless_consumables;
    bool share_current_skill_training;
    bool reduced_int_requirements_for_schemes;
    bool inventory_active_for_all_followers;
} HighResConfig;

void highres_config_load(void);
const HighResConfig* highres_config_get(void);

#endif /* GAME_HIGHRES_CONFIG_H_ */
