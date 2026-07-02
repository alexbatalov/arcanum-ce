# Arcanum CE Config

`arcanum-ce.cfg` is an optional player-facing config file in the game directory.

It is a small compatibility-safe layer over the existing `HighRes/config.ini` import. The engine still starts with built-in defaults, then reads `HighRes/config.ini` if present, then reads `arcanum-ce.cfg` if present. Command-line switches handled by `src/main.c` still override matching file settings for the current launch.

## Example

```ini
profile=restored-default
module=Arcanum
width=1280
height=720
windowed=1
show_fps=0
scroll_fps=35
scroll_dist=10
startup_report=1
logos=1
intro=1
```

## Supported Keys

Key names are case-insensitive. Dashes and underscores are ignored, so `ScrollFPS`, `scroll_fps`, and `scroll-fps` are equivalent.

The current parser supports the existing UAP-compatible keys:

- `width`
- `height`
- `windowed`
- `module`
- `profile`
- `startup_report`
- `startup_report_path`
- `show_fps`
- `scroll_fps`
- `scroll_dist`
- `identification_cost`
- `level_cap`
- `stats_cap`
- `speed_cap`
- `reaction_modifier_cap`
- `carry_weight_cap`
- `stolen_gold_cap`
- `name_length`
- `logos`
- `intro`
- `disable_combat_music`
- `disable_spell_level_requirement`
- `disable_xp_per_hit`
- `full_xp_per_kill`
- `disable_sprite_mirroring`
- `hide_zero_damage_to_creatures`
- `hide_zero_damage_to_objects`
- `no_character_points_at_start`
- `start_stats_at_one`
- `magical_aptitude_bonus`
- `technological_aptitude_bonus`
- `starting_fate_point`
- `disable_natural_recovery`
- `poison_immunity`
- `disable_prowling_to_hit_bonus`
- `disable_aptitude_to_hit_penalty`
- `technological_cursor`
- `disable_global_lighting_change`
- `free_buying`
- `free_selling`
- `endless_consumables`
- `share_current_skill_training`
- `reduced_int_requirements_for_schemes`
- `inventory_active_for_all_followers`
- `use_any_equipment`
- `expected_path`
- `expected_dat`
- `expected_root`
- `expected_module_root`
- `expected_loose_root`

When `profile` is set, Restored CE also reads `profiles/<profile>.cfg` unless the value already looks like a path or `.cfg` file.

Profile files use the same key format and are intended for module/default selection plus expectation checks. See [`ARCANUM_RESTORED_CE_PROFILES.md`](ARCANUM_RESTORED_CE_PROFILES.md).

## Boundaries

This file is not written by the engine yet. It does not replace module data, DAT loading, saves, or command-line switches. The profile layer is diagnostic and selection-oriented; it does not change the existing loader repository order.
