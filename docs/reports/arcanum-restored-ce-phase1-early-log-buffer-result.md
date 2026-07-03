# Arcanum Restored CE Phase 1 Early-Log Buffer Result

Date: 2026-07-02

Branch: `phase1-early-log-buffer`

Worktree:

```text
D:\projects\Arcanum CE Restored - phase1 early log
```

Base:

```text
origin/main @ a5e606f5
```

## Scope

This lane implements the Phase 1 B4 early-log buffering slice only.

It adds a static pre-init debug buffer to TIG logging so `tig_debug_printf`
messages emitted before `tig_debug_init` are preserved and flushed once debug
logging initializes. TIG init failure paths explicitly flush the buffer before
returning.

Kept out of scope:

- debug setting modes
- config systems
- engine `-scan` mode
- CI preview builds
- multiplayer transport
- CE overlay mounting
- DAT writer work
- QoL features
- gameplay logic changes

## Files Changed

Source files:

- `first_party/tig/include/tig/debug.h`
- `first_party/tig/src/core.c`
- `first_party/tig/src/debug.c`

Report file:

- `docs/reports/arcanum-restored-ce-phase1-early-log-buffer-result.md`

No original Arcanum game assets, DAT contents, saves, screenshots, or private
runtime files were added.

## Changes Made

- Added `tig_debug_flush`.
- Moved the existing partial-line debug buffer from a function-local static into
  debug module state.
- Added a fixed-size early debug buffer for complete log output emitted before
  `tig_debug_init`.
- Flushed early output during `tig_debug_init` before printing the normal debug
  timestamp header.
- Flushed any partial debug line during `tig_debug_exit`.
- Flushed early debug output before returning from `tig_init` SDL initialization
  failure.
- Flushed early debug output before returning from `tig_init` TIG module
  initialization failure.

## Behavior Notes

This is diagnostics-only behavior. It does not alter game state, startup
decisions, module loading decisions, save data, rendering, audio, input,
networking, or configuration.

Default single-player gameplay remains unchanged.

The early buffer uses static storage only and does not allocate memory. This
keeps it usable before the TIG memory output hook is installed.

If the early buffer fills, excess bytes are counted and a truncation diagnostic
is emitted on flush.

## Commands Run

```text
git worktree add -b phase1-early-log-buffer "D:\projects\Arcanum CE Restored - phase1 early log" origin/main
rg -n "early-log|early log|Phase 1|Next Recommended|log buffering|buffer" docs src first_party -g "*.md" -g "*.c" -g "*.h"
rg -n "tig_debug_init|tig_debug_exit|tig_debug_printf|tig_debug_println|tig_memory_set_output_func|SDL_Log|SDL_LogSet|SDL_LogOutput" src first_party -g "*.c" -g "*.h"
rg -n "SDL_Log|SDL_SetLog|SDL_LogSet|log file|arcanum.*log|debug.*log|OutputDebug|stderr|stdout|console" . -g "*.c" -g "*.h" -g "*.md" -g "CMakeLists.txt"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --preset windows-x64
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock\data"
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock"
git diff --check
```

## Check Results

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

The build completed and produced:

- `out\build\windows-x64\Debug\arcanum-ce.exe`
- `out\build\windows-x64\Debug\arcanum-ce-module-check.exe`

Existing warnings still visible in the isolated first build:

```text
first_party\tig\src\database.c(941,43): warning C4267
first_party\tig\src\database.c(954,20): warning C4267
src\game\item.c(2344,9): warning C4101
src\game\multiplayer.c(1102,13): warning C4101
src\game\path.c(1012,1): warning C4716
src\game\name.c(1426,1): warning C4716
src\game\terrain.c(602,1): warning C4716
src\ui\dialog_ui.c(362,9): warning C4101
```

These warnings are from untouched files and are not caused by the early-log
buffer change.

Stock loose data scan:

```text
Path: D:\Arcanum\dont touch\Arcanum-gogstock\data
Directories scanned: 3
Files scanned: 1424
Issues: 0
```

Full stock root scan:

```text
Path: D:\Arcanum\dont touch\Arcanum-gogstock
Directories scanned: 15
Files scanned: 1584
DAT archives indexed: 7
DAT entries: 107883
Issues: 0
```

Diff hygiene:

```text
git diff --check
Result: pass
```

Markdown lint:

```text
Local markdownlint-cli2: not available
Local markdownlint: not available
package.json: not present
Result: not run
```

## Known Limits

- This lane does not add debug mode selection.
- This lane does not add engine `-scan`.
- This lane does not add CI preview builds.
- This lane does not change multiplayer behavior.
- This lane does not change default single-player gameplay behavior.
- This lane is based on `origin/main`; it does not include the separate
  `phase1-diagnostic-format-strings` commit unless that lane is later merged
  into main.

Rollback is direct: revert the three source-file edits and this report. No
runtime data, save data, config files, or asset files are affected.

## Next Recommended Slice

After the format-string and early-log lanes are reviewed and merged, open the
Phase 1 engine `-scan` branch as the next safety-tools slice. Keep CI preview
builds as a separate follow-up branch.
