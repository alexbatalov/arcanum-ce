# Arcanum Restored CE Phase 2 Config Foundation Result

Date: 2026-07-02

Branch: `phase2-config-foundation`

Worktree:

```text
D:\projects\Arcanum CE Restored - phase2 config
```

Base:

```text
origin/main @ 070e4fc1
```

## Scope

This lane implements the first Phase 2 config-foundation slice.

The goal is to make `arcanum-ce.cfg` a sectioned, self-documenting runtime
configuration surface while preserving the existing highres config behavior and
default single-player behavior.

Kept out of scope:

- QoL UI work
- new player-visible toggles
- CE overlay mounting
- DAT writer work
- module packaging
- multiplayer transport
- save-format changes

## Files Changed

Source files:

- `CMakeLists.txt`
- `src/game/ce_config.c`
- `src/game/ce_config.h`
- `src/game/settings.c`
- `src/game/settings.h`
- `src/main.c`

Report file:

- `docs/reports/arcanum-restored-ce-phase2-config-foundation-result.md`

No original Arcanum game assets, DAT contents, ART files, saves, screenshots,
runtime folders, or binary payloads were added to the repository.

## Changes Made

- Extended the existing `Settings` registry with optional section names.
- Added section-aware register/get/set helpers while keeping the existing flat
  settings API intact.
- Added optional default writeback for config surfaces that want missing
  registered defaults written to disk.
- Fixed `settings_init` so `flags` is initialized for non-global `Settings`
  instances.
- Fixed trailing whitespace trimming in `settings_trim`.
- Preserved empty values such as `Module=` and `Profile=` when loading settings.
- Added `ce_config_ensure_defaults()` for `arcanum-ce.cfg`.
- Added sectioned default groups:
  - `[system]`
  - `[display]`
  - `[qol]`
- Copied legacy flat `arcanum-ce.cfg` values into missing sectioned values.
- Copied legacy `HighRes/config.ini` values into missing sectioned values.
- Kept legacy `HighRes/config.ini` read-only.
- Called CE config default writeback after `tig_init`, before game/UI startup.

The existing `highres_config` parser remains the behavior source for this slice.
Section headers are compatible with that parser because it ignores non-key lines
and continues reading the same key names.

## Behavior Notes

Default single-player behavior is unchanged:

- All QoL defaults are off or neutral.
- Existing `HighRes/config.ini` values still load before `arcanum-ce.cfg`.
- Existing `arcanum-ce.cfg` values still override legacy config values.
- The new writeback runs only after TIG initializes successfully.

When missing defaults are added, `arcanum-ce.cfg` may be rewritten by the
settings writer. Unknown loaded keys are preserved, but comments are not
preserved by this first slice.

## Commands Run

```text
git diff --check
rg -n -i "<restricted-public-terms>" CMakeLists.txt src/game/settings.c src/game/settings.h src/game/ce_config.c src/game/ce_config.h src/main.c
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --preset windows-x64
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
out\build\windows-x64\Debug\arcanum-ce-module-check.exe docs
out\build\windows-x64\Debug\arcanum-ce.exe -scan --help
out\build\windows-x64\Debug\arcanum-ce.exe -scan docs
out\build\windows-x64\Debug\arcanum-ce.exe -nosound -window
```

The runtime smoke used:

```text
out\tmp\phase2-config-smoke
```

The smoke folder hardlinked only local stock `tig.dat` so TIG could initialize.
No original game assets were committed.

## Check Results

Diff hygiene:

```text
git diff --check
Result: pass
```

Restricted public-term scan:

```text
Result: pass
```

Configure:

```text
cmake --preset windows-x64
Result: pass
```

Build:

```text
cmake --build --preset windows-x64-debug
Result: pass
```

The build produced:

- `out\build\windows-x64\Debug\arcanum-ce.exe`
- `out\build\windows-x64\Debug\arcanum-ce-module-check.exe`

Existing warnings from untouched source files were still present:

```text
first_party\tig\src\database.c(941,43): warning C4267
first_party\tig\src\database.c(954,20): warning C4267
src\game\item.c(2344,9): warning C4101
src\game\multiplayer.c(1102,13): warning C4101
src\game\name.c(1426,1): warning C4716
src\game\path.c(1012,1): warning C4716
src\game\terrain.c(602,1): warning C4716
src\ui\dialog_ui.c(362,9): warning C4101
```

Module check on docs:

```text
Files scanned: 18
Issues: 0
Result: pass
```

Engine scan help:

```text
arcanum-ce.exe -scan --help
Result: pass
```

Engine scan on docs:

```text
Files scanned: 18
Issues: 0
Result: pass
```

Default config generation smoke:

```text
Path: out\tmp\phase2-config-smoke
Result: pass

Generated arcanum-ce.cfg sections:
- [qol]
- [display]
- [system]
```

Legacy import smoke:

```text
Legacy source: out\tmp\phase2-config-smoke\HighRes\config.ini
Legacy values: Windowed=1, UseAnyEquipment=1, Module=Arcanum
Legacy file unchanged: true
Sectioned values written to arcanum-ce.cfg: true
Result: pass
```

The throwaway engine smoke exited with code `1` after config generation because
the smoke folder intentionally did not contain full game data:

```text
gamelib_init(): init function 1 (Description) failed
```

This is expected for the minimal no-assets smoke.

Unavailable local checks:

```text
clang-format: not available on PATH
markdownlint-cli2: not available
markdownlint: not available
```

## Known Limits

- This slice does not add a config UI.
- This slice does not add new QoL behavior.
- This slice does not remove legacy flat keys from existing configs.
- This slice does not preserve comments when `arcanum-ce.cfg` is rewritten to
  add missing defaults.
- This slice does not add overlay mounting or DAT packaging.

## Next Recommended Slice

After review and merge, continue Phase 2 with config reporting and validation:

- have the module checker report known CE config keys and unknown config keys
- document the generated `arcanum-ce.cfg` sections
- keep player-visible QoL work deferred until Phase 5
