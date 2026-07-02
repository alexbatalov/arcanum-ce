# Fallout 2 CE to Arcanum CE Systems Audit

Date: 2026-07-02

## Repositories

| Repo | URL | Branch | Commit |
| --- | --- | --- | --- |
| Fallout 2 CE reference | https://github.com/fallout2-ce/fallout2-ce.git | main | f3caac0bda058a777afa3ebcd9d39d420c48265a |
| Arcanum CE Restored | https://github.com/harleyfolgado/Arcanum-ce-Restored.git | main | 1f939c5894b8a4b1926a285e20f9457114d753ee |

Reference checkout used: `D:\projects\fallout2-ce-reference`.

## Summary

Fallout 2 CE has several mature, modder-facing systems that are worth learning from, but most are not safe to port directly into Arcanum CE because the games use different data layouts, script ecosystems, module assumptions, and UI stacks.

The safest immediate transfer is better static compatibility reporting around loose files and DAT contents. This gives Arcanum modders a concrete view into override behavior without changing runtime asset resolution, config semantics, save formats, or gameplay.

## Feature Comparison

| Area | Fallout 2 CE evidence | Arcanum CE equivalent | Classification | Notes |
| --- | --- | --- | --- | --- |
| Config/settings loading | Central `fallout2.cfg`, typed settings registry, defaults written back by `settingsWriteToConfig`. | Command-line flags plus `HighRes/config.ini` import in `src/game/highres_config.c`; simpler `Settings` helper for game settings. | SAFE TO IMPLEMENT NOW | Implemented optional `arcanum-ce.cfg` read support as a thin layer over existing startup settings. Writeback and full registry remain deferred. |
| Config migration | `game_config_migration.cc` migrates `f2_res.ini` and some Sfall `ddraw.ini` settings into CE config files. | `HighRes/config.ini` is read at startup but not rewritten into a new CE-owned config. | ARCANUM-SPECIFIC WORK NEEDED | Safe next step is a written config design and one-way import plan, not an ad hoc new file. |
| Mod/profile loading | Fallout supports popular total conversion expectations and content config overrides. | Arcanum loads stock modules and has `-mod:` selection. | RISKY / DEFER | Module/profile loading changes are engine-wide and could break stock Arcanum behavior. |
| DAT/ZIP/loose-file override behavior | Fallout `xfile.cc` supports directories, DATs, ZIP diagnostics, and explicit archive error logs. | TIG file repositories search DATs/directories; Arcanum supports DAT archives and loose files but had limited static visibility. | SAFE TO IMPLEMENT NOW | Implemented read-only DAT index and loose override reporting in `arcanum-ce-module-check`; validated against a real CE runtime after fixing DAT1 footer handling. |
| High-res and UI scaling options | `fallout2.cfg` exposes resolution, windowed mode, scale, UI options, and f2_res migration. | `-geometry`, `-window`, and UAP HighRes keys for width/height/window/FPS/scroll. | DESIGN TRANSFER ONLY | UI scaling is not a small patch; Arcanum UI needs dedicated layout work. |
| Debug/logging/crash diagnostics | Fallout buffers early debug messages, supports debug modes, and logs config migration. | TIG logs through `tig_debug_printf`; startup failures are still mostly generic. | ARCANUM-SPECIFIC WORK NEEDED | Good target, but should be scoped around concrete startup failure cases. |
| Missing asset reporting | Fallout has more focused archive/open diagnostics; Sfall and content systems expose mod-facing gaps. | Runtime logs many missing art/file failures; module checker reports static script/dialog/message issues. | SAFE TO IMPLEMENT NOW | Override reporting was the safest first slice; deeper missing art/script reference resolution remains future scanner work. |
| Music/audio/image format support | README documents `.ogg/.wav` map music and `.png` static assets; code includes decoders. | Arcanum README expects MP3 music from installed assets and SDL3_mixer is configured with mp3/wave. | DESIGN TRANSFER ONLY | Format expansion should be user-facing and tested with Arcanum asset routes before implementation. |
| Save/load QoL | Fallout has modernized QoL around platform save handling. | Arcanum has existing save/load and F7/F8 behavior. | RISKY / DEFER | Save format and UX changes are high-risk without manual game QA. |
| Compatibility docs | Fallout has `SFALL_COMPATIBILITY.md` and README modder notes. | Arcanum has `docs/ARCANUM_CE_MODULE_CHECK.md`. | SAFE TO IMPLEMENT NOW | Updated module-check docs; broader compatibility matrix should follow after more scanner coverage. |
| Build/release workflow | Fallout builds tools such as `ce-dat-tool`, packages `ce.dat`, and has richer platform workflows. | Arcanum has CMake presets, CI, and `arcanum-ce-module-check`. | DESIGN TRANSFER ONLY | No release workflow change in this slice. |
| Script/opcode extension patterns | Fallout has extensive Sfall opcode/hook code and compatibility tracking. | Arcanum scripts are different; current checker knows Arcanum script condition/action IDs. | NOT RELEVANT | Do not port Fallout-specific scripting or gameplay extension systems directly. |

## Safe Candidates Chosen

- Add read-only DAT index reporting to `arcanum-ce-module-check`.
- Report loose files whose normalized paths match entries inside scanned DAT archives.
- Keep override matches informational, not errors.
- Update companion-tool documentation.
- Add optional `arcanum-ce.cfg` startup settings read support without removing `HighRes/config.ini`.
- Accept lower snake_case config keys while preserving UAP-style key names.

## Deferred

- Config writeback/default generation: needs a design document covering precedence, platform paths, and migration from UAP `HighRes/config.ini`.
- Total-conversion/module profiles: engine-wide and risky.
- ZIP archive support in Arcanum TIG: loader-level behavior change, not safe in this pass.
- UI scale and player-facing option panels: requires layout/runtime QA.
- Sfall/opcode systems: Fallout-specific and not relevant to Arcanum gameplay.
- Save/load QoL: high risk without manual stock-game regression testing.

## Publication

The safe implementation slice was committed and pushed to `publish/main`.
