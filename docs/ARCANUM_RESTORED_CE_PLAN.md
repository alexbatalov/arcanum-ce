# Arcanum Restored CE Plan

Date: 2026-07-02

Research-only pass. Nothing in this document has been implemented; it is a roadmap of systems worth transferring from our Fallout 2 CE fork into Arcanum Restored CE.

## Repositories Compared

| Repo | Path | Branch | Commit |
| --- | --- | --- | --- |
| Arcanum Restored CE | `D:\projects\Arcanum CE Restored` | main | e95cd0ab7ad76fddb81ab8b1d35dff9dec543b86 |
| Fallout 2 CE fork | `D:\projects\Fallout2 CE Fork` | codex/private-issue-fix-queue-20260630 | 8b06e135d3ea36b6f2245c7f35f4a26776ee21a4 |

This builds on the earlier upstream-only comparison in
`docs/reports/fallout2-ce-to-arcanum-ce-systems-audit.md` (which compared against
`fallout2-ce/fallout2-ce` at `f3caac0`). What is new in this pass:

- The comparison target is **our fork**, which carries the private issue-fix queue
  (`GITHUB_ISSUES_FIX_PLAN.md`, `PRIVATE_CE_BUILD_WORKLOG.md`, `scan_unimplemented` extensions,
  sfall test scripts, format-string and animation-lifetime hardening) plus feature branches
  (`ammo-counter-20260701`, `codex/mainmenu-overlay-review-20260702`, `mainmenu-fade`,
  `codex/hero-appearance-selector-upstream`).
- Multiplayer is examined explicitly, because Arcanum — unlike Fallout 2 — shipped with
  multiplayer and the CE codebase still contains most of that game-layer code.
- The safe slice from the earlier audit (DAT/loose override reporting in
  `arcanum-ce-module-check`, `arcanum-ce.cfg` read support) is already merged and is treated as
  the baseline here, not re-proposed.

## Key Architectural Findings

**Arcanum CE multiplayer is stubbed, not absent.** `src/game/multiplayer.c` (~3,000 lines) and
`src/game/mp_utils.c` are real game-layer multiplayer logic — message handlers, message
validation callbacks, server options (player killing, friendly fire, auto-equip, key sharing),
max-player handling. They compile against `src/net_compat.h`, a no-op macro layer that replaced
the original TIG network module (`first_party/tig/src` has no `net.c`). Restoring multiplayer is
therefore a **transport-layer reimplementation** under a mostly intact game layer, not a
from-scratch feature. Fallout 2 CE has nothing to port here, but its hardening patterns
(validation before use, guarded handles, diagnostics) apply directly to the sync layer.

**The script ecosystems are incompatible.** Fallout 2 runs a bytecode interpreter with sfall
opcode extensions; Arcanum scripts are condition/action ID records. Nothing in
`sfall_opcodes.cc` / `sfall_arrays.cc` / `sfall_metarules.cc` ports directly. What ports is the
**pattern**: engine-owned hook points, user global scripts loaded from a folder, and an
engine-side scanner that validates script content against what the engine actually implements.

**Fallout 2 CE ships its own asset overlay (`ce.dat`).** `files/ce.dat/*` is packaged at build
time by `ce-dat-tool` (see `CMakeLists.txt` ~line 588) and mounted so the engine can ship new UI
art, configs, and text without touching game data. Arcanum CE has no equivalent; TIG's file
repository already supports DAT + loose-file priority, so the mount mechanism exists.

## Candidate Systems

Classification: **P0** = high value, low-to-moderate risk, do first · **P1** = high value,
needs design · **P2** = valuable, large or risky · **NO-PORT** = explicitly not transferring.

### A. Modding

| # | System | Fallout 2 CE evidence | Arcanum today | Priority |
| --- | --- | --- | --- | --- |
| A1 | Engine-owned overlay, **loose folder first** (`ce/` mounted at top priority, mount logged), `arcanum-ce.dat` packing later | `files/ce.dat/` packaged by `ce-dat-tool`, mounted above game data | TIG repositories support DAT + loose priority; no CE-owned overlay | **P0** (loose mount) |
| A2 | DAT pack/unpack tool | `tools/dat_tool.cc`, `tools/dat_archive.cc` | `arcanum-ce-module-check` already parses Arcanum DAT1 indexes read-only | **P1** (after A1 loose mount is proven; the writer is a standalone tool with no engine risk, but it has no consumer until the mount works) |
| A3 | ART ↔ PNG converter | `tools/ce_frm2png.cc` (both directions, indexed PNG) | No art tooling; modders use third-party ArtView-era tools | **P1** |
| A4 | Typed settings registry with sections and writeback | `settings.h` structs (`SystemSettings`, `QolSettings`, `MapperSettings`…), `settingsInit`/write-back; `game_config_migration.cc` one-way legacy import | Original `settings.c` linked-list registry (per-key change callbacks, already has save); ~20 CE toggles added ad hoc; `arcanum-ce.cfg` is read-only | **P1** |
| A5 | Global script slots + engine hook points (pattern only) | `sfall_global_scripts.*`, `sfall_script_hooks.*`, recent `HOOK_MOVECOST`/`HOOK_ITEMDAMAGE` | Arcanum script system has no user-global scripts and no engine hook registry | **P2** |
| A6 | Total-conversion/content profiles | `content_config.cc` | `-mod:` module selection only | **P2** (was RISKY/DEFER; keep deferred until A1/A4 land) |
| A7 | Integrated map editor | `src/mapper/` — full mapper in the engine, `MapperSettings`, `dev_path` save redirection, EDG editing, TMP-file fixes | No editor; WorldEd is external and Win9x-era | **P2** (long-term flagship for "module expansion support") |

Notes:

- **A1 is the highest-leverage modding item, and it starts loose.** The Module Expansion import
  work (`docs/reports/arcanum-module-expansion-ce-import-result.md`, 8,133 loose files) shows
  exactly the pain: mods land as loose files with no manifest, no versioning, and overwrite
  risk. Step 1 is a `ce/` loose overlay folder (`ce/mes/`, `ce/art/`, `ce/rules/`, `ce/ui/`)
  mounted at top priority with the mount and every override logged — a small, observable engine
  change that proves the mechanism. Only after that is proven does A2 turn the folder into a
  packed `arcanum-ce.dat` (distribution polish) and, later, a documented mod-package format.
  The A2 writer extends the existing module-check DAT reader, keeping the read-only scanner
  untouched.
- **A4 should reuse Arcanum's own `settings_register` machinery** rather than porting the C++
  struct approach: register CE keys into the existing registry, add sectioned
  `arcanum-ce.cfg` writeback with defaults (like `settingsWriteToConfig`), and fold the ~20
  existing CE toggles under a `[qol]`-style grouping. One-way migration from UAP
  `HighRes/config.ini` follows the `game_config_migration.cc` shape: import once, mark migrated,
  never write back to the legacy file.
- **A5 concrete Arcanum form:** a `scripts/global/` folder scanned at module load; each global
  script gets heartbeat/start/map-enter invocation slots, mirroring
  `sfall_gl_scr_exec_start_proc` / `process_main` / `exec_map_update_scripts`. Hook points come
  later and only where Arcanum has natural seams (damage calc, movement cost, barter price).

### B. Reliability / Diagnostics

| # | System | Fallout 2 CE evidence | Arcanum today | Priority |
| --- | --- | --- | --- | --- |
| B1 | Unimplemented-feature scanner | `scan_unimplemented.*` + fork extensions (`scan_unimplemented_opcodes.h`, `_configs.h`, `_sfall.h`); command-line activated, walks all scripts/configs and reports what the engine doesn't implement | `arcanum-ce-module-check` validates script condition/action IDs statically, outside the engine | **P0** |
| B2 | Format-string and diagnostics hardening | Fork's #512-subset: all generated strings passed through literal formats to `debugPrint`/fatal paths; fixed missing-arg crashes (upstream #511) | `tig_debug_printf` call sites unaudited for the same class of bug | **P0** |
| B3 | Object/handle lifetime guards | Fork's #513-subset (`animationKindIsCallback`, null-after-unlock, destroyed-owner tracking); upstream #493 `whoHitMe` corruption guard | Arcanum `obj`/`anim` handle validation exists but save-load corruption guards are thinner; `anim.c` is flagged in README as least-understood module | **P1** |
| B4 | Early-log buffering + debug modes | `debug.cc` buffers pre-init messages; `DebugSettings` selects output mode | `tig_debug_init` starts logging only after TIG init; startup failures before that are silent | **P1** |
| B5 | Structured QA workflow | `GITHUB_ISSUES_FIX_PLAN.md` fix-lane triage + `PRIVATE_CE_BUILD_WORKLOG.md` per-fix evidence, throwaway-build validation, SHA-recorded binaries | Already partially adopted (ThrowawayBuilds/MilestoneBuilds, dated reports) | **P0** (formalize as a doc, near-zero code risk) |
| B6 | CI preview/release builds | Fork's `ci-build.yml` / `release.yml` changes, upstream preview-build workflows | CMake presets + basic CI | **P1** |

Notes:

- **B1 is the single best reliability transfer.** Concrete Arcanum form: an optional engine mode
  (`-scan` flag, mirroring `scanUnimplementdParseCommandLineArguments`) that loads every module
  script/dialog/mes through the real engine parsers and reports unknown condition/action IDs,
  missing art/sound references, and unknown config keys — complementing the external static
  scanner with the engine's own truth. This directly serves the README's "compatibility scans"
  goal and pays off again for every restoration module.
- B2 is mechanical and low risk: same grep-and-fix sweep the fork ran, applied to
  `tig_debug_printf` / fatal-error call sites.

### C. Multiplayer / Co-op

Fallout 2 CE offers no networking to port; this lane is Arcanum-native work informed by the
fork's hardening discipline.

| # | Step | Detail | Priority |
| --- | --- | --- | --- |
| C1 | Multiplayer surface inventory | Map every `net_compat.h` macro to its call sites; document what each stubbed `tig_net_*` function must actually do; catalog `multiplayer.c` message types and the existing validation callback contract | **P0** (research/doc only) |
| C2 | TIG net transport layer | Reintroduce `tig/net.c` behind a compile-time flag using SDL3-friendly transport (SDL_net or a small enet-style UDP layer); implement the `net_compat.h` contract so `multiplayer.c` compiles against the real thing with zero game-layer edits at first | **P2** |
| C3 | LAN co-op milestone | Host/join on LAN, 2 players, stock module, with the server options already modeled in `multiplayer.c` (friendly fire, player killing, key sharing, auto-equip) | **P2** |
| C4 | Sync auditing | Deterministic checks (object counts, critter HP checksums) exchanged via the existing message-validation hook; desync logging through the B4 debug channel | **P2** |

**Hard gate: this lane is documentation-only (C1) until Phases 0–5 are delivered.** No
transport code, no `net_compat.h` changes, no `tig/net.c`, no matter how tempting the stub
inventory makes it look. C2–C4 are a monster-sized effort and only start once single-player CE
is cleaner and the reliability/config/overlay foundations exist.

Guardrail from the README: single-player compatibility stays protected — `tig_net_is_active()`
must keep compiling to constant `0` in default builds so the optimizer strips all
multiplayer branches exactly as today.

### D. Engine / UI / QoL

| # | System | Fallout 2 CE evidence | Arcanum fit | Priority |
| --- | --- | --- | --- | --- |
| D1 | In-game help/hotkey reference | Upstream #498 in-game HELP menu (`game.cc`) | Arcanum has rich hotkeys (`hotkey_ui.c`) and no in-game reference; content ships via the A1 overlay | **P1** |
| D2 | Main-menu overlay/fade polish | Fork branches `codex/mainmenu-overlay-review-20260702`, `mainmenu-fade` (menu kept behind subscreens, fallback backdrop, fade) | `mainmenu_ui.c` has the same subscreen structure; port the *behavior*, not the code | **P1** |
| D3 | HUD status readouts | Fork branch `ammo-counter-20260701` (optional ammo counter, config-gated) | Arcanum analogue: ammo/charges/fatigue readouts in `intgame.c`, gated by an A4 config key | **P1** |
| D4 | Hero appearance presentation | `hero_appearance.cc` + fork's hero-appearance-selector branch | README's character-expansion goal (portraits, art routes) — same pattern of appearance variants resolved at art-lookup time | **P2** |
| D5 | FPS limiter / frame pacing | `fps_limiter.cc` | Arcanum has `ScrollFPS` UAP keys; a uniform frame limiter is small and self-contained | **P1** |
| D6 | Audio format expansion (ogg/flac music) | Upstream ogg/wav map-music support | SDL3_mixer is already the backend; enabling ogg is mostly build config + path resolution fallbacks (`gsound.c`) | **P1** |
| D7 | Movie scaling / windowed polish | Upstream #478 movie scaling under HRP | Arcanum `gmovie.c` + `hrp.c`; needs QA against Bink compat layer | **P2** |
| D8 | Extra save slots / save QoL | `QolSettings::extra_save_slots`, platform save-dir fixes | RISKY/DEFER stands from prior audit until a save-format regression suite exists | **P2** |
| D9 | Touch/gamepad input | `touch.cc`, dinput layers | Only relevant if the android/ios targets become active goals | **P2** |

### NO-PORT (unchanged from prior audit, reconfirmed against the fork)

- sfall opcode/array/metarule/global-var implementations — Fallout bytecode-specific.
- Fallout proto/perk/trait data systems (`perks.cfg` in the fork's ce.dat) — no Arcanum analogue.
- `interpreter.cc` and everything downstream of it.
- Fallout HRP tile stencil (`tile_hires_stencil.cc`) — Arcanum's iso renderer is unrelated;
  Arcanum already fixed its own roof/wall cases (`roof.c`).

## Phased Roadmap

Each phase is a separate, independently shippable lane. Do not start a phase until the previous
one is merged and QA'd; do not mix lanes in one branch.

**Phase 0 — Docs/process only (zero source changes)**
- B5: adopt the fix-plan/worklog structure as `docs/ARCANUM_CE_FIX_PLAN.md` +
  `docs/ARCANUM_CE_BUILD_WORKLOG.md` conventions.
- C1: multiplayer surface inventory document (map `net_compat.h` macros to call sites; catalog
  `multiplayer.c` message types and the validation-callback contract).

**Phase 1 — Safety tools (boring on purpose; builds trust)**
- B2 format-string sweep over `tig_debug_printf` / fatal-error call sites.
- B4 early-log buffering.
- B1 engine `-scan` mode, first read-only slice (shares parsers, changes no gameplay paths).
- B6 CI preview builds so every later phase produces installable artifacts.

**Phase 2 — Config system (later phases need config gates)**
- A4 settings registry consolidation, sectioned `arcanum-ce.cfg` writeback with defaults,
  one-way UAP `HighRes/config.ini` migration, existing ~20 CE toggles grouped under `[qol]`.

**Phase 3 — Overlay, loose first**
- A1 loose `ce/` overlay folder (`mes/`, `art/`, `rules/`, `ui/`) mounted at top priority.
- Mount and every override logged; override report surfaced through `arcanum-ce-module-check`.
- No DAT writer in this phase — the loose mount is the proof.

**Phase 4 — Module/mod packaging (only after the overlay is proven)**
- A2 DAT writer, `arcanum-ce.dat` build-time packing of the `ce/` folder.
- Mod-package manifest format + override report integration.
- A3 ART↔PNG tool.

**Phase 5 — Player-visible QoL** (each item individually config-gated per Phase 2)
- D1 help screen, D2 main-menu polish, D3 HUD readouts, D5 FPS limiter, D6 ogg music.

**Phase 6 — Big rocks (order by appetite; each needs its own design doc first)**
- C2→C4 multiplayer transport and LAN co-op.
- A5 global scripts/hooks; A7 integrated editor; D4 hero appearance; A6 content profiles.

## Execution Gates

Rules for whoever (or whatever agent) implements this plan:

1. **One lane per branch.** A slice touches one phase's files only. If a change wants to touch
   DAT tools *and* settings *and* UI, it is mis-scoped — split it.
2. **Doc-only phases produce zero source diffs.** Phase 0 output is markdown files, nothing else.
3. **Multiplayer is frozen at C1 (docs) until Phase 6.** No `net_compat.h` edits, no transport
   code, regardless of how complete the stub inventory looks.
4. **A5 (global scripts/hooks) and A7 (integrated editor) are design-doc-first, Phase 6 only.**
   Hook systems can silently break save/runtime behavior; the editor is months of work and stays
   a future flagship, not part of this push.
5. **Every engine-touching slice keeps default single-player behavior bit-identical** unless the
   slice is explicitly a default change, and passes the validation rules below before merge.

## Validation Rules (carried over from the F2 fork workflow)

- Every slice gets a throwaway-build runtime QA pass against real GOG data
  (`D:\Arcanum\ThrowawayBuilds\...`), with commands and results recorded in a dated report under
  `docs/reports/`.
- `arcanum-ce-module-check` must stay green on stock data before and after each slice.
- No original game assets committed; fixtures generated under `out/tmp/`.
- Default-configuration behavior must be bit-identical for single-player unless the slice is
  explicitly a default change (matches the config-toggle convention already used on main).
