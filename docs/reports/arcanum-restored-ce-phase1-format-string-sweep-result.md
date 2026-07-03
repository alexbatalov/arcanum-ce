# Arcanum Restored CE Phase 1 Format-String Sweep Result

Date: 2026-07-02

Branch: `phase1-diagnostic-format-strings`

## Scope

This lane implements Phase 1 lane 1 only: format-string diagnostics hardening
for `tig_debug_printf`, `tig_debug_println`, and printf-style fatal/error call
sites.

Kept out of scope:

- early-log buffering
- engine `-scan` mode
- config systems
- multiplayer transport
- CE overlay mounting
- DAT writer work
- QoL features
- drive-by cleanup

## Files Changed

Source files:

- `first_party/tig/src/sound.c`
- `src/game/anim_private.c`
- `src/game/map.c`
- `src/game/party.c`
- `src/game/terrain.c`

Report file:

- `docs/reports/arcanum-restored-ce-phase1-format-string-sweep-result.md`

No original Arcanum game assets, DAT contents, saves, screenshots, or private
runtime files were added.

## Changes Made

- Added the missing `type` argument to a `tig_debug_printf` sound-handle
  exhaustion diagnostic.
- Added the missing animation ID string argument to
  `anim_goal_restart` diagnostics.
- Repaired a mobile object save diagnostic so the existing path argument is
  consumed by a literal `%s` format.
- Repaired four map gender-check diagnostics that had two location conversion
  specifiers but only one packed location argument. These now log X and Y
  components explicitly.
- Repaired a party save diagnostic so the existing count argument is consumed
  by a literal `%d` format.
- Repaired a terrain diagnostic typo from `$d` to `%d` and supplied the missing
  `base` argument.

## Audit Notes

An inline parser audited printf-style calls under `src` and `first_party` for:

- `tig_debug_printf`
- `tig_memory_fatal_error`
- `SDL_SetError`

Parser result:

```text
CALLS 972
DYNAMIC_FORMAT_ARGS 11
PROBLEMS 0
```

The dynamic-format findings were prototypes/function definitions plus
compile-time `PRI*` format macro concatenations. No generated runtime string
was left as a `tig_debug_printf` format argument.

`tig_debug_println` was reviewed through `first_party/tig/src/debug.c`; it
routes its input through `tig_debug_printf("%s\n", string)`, so generated
strings passed to `tig_debug_println` are already protected by a literal
format.

Message-box error calls were reviewed separately. `SDL_ShowSimpleMessageBox`
uses plain string parameters, not printf-style formatting.

## Commands Run

```text
git switch -c phase1-diagnostic-format-strings
rg -n "tig_debug_printf|tig_debug_println|fatal|Fatal|SDL_SetError|SDL_ShowSimpleMessageBox" src first_party -g "*.c" -g "*.h"
<inline parser for tig_debug_printf, tig_memory_fatal_error, SDL_SetError>
rg -n "\$[diuoxXfFeEgGaAcCsSp]|%[diuoxXfFeEgGaAcCsSp][A-Za-z0-9_]|%$" src first_party -g "*.c" -g "*.h"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock\data"
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock"
git diff --check
```

## Check Results

Build:

```text
cmake --build --preset windows-x64-debug
Result: pass
```

The build completed and produced:

- `out\build\windows-x64\Debug\arcanum-ce.exe`
- `out\build\windows-x64\Debug\arcanum-ce-module-check.exe`

One existing compiler warning remains visible:

```text
src\game\terrain.c(602,1): warning C4716: 'sub_4E8D60': must return a value
```

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

- This lane does not add early-log buffering.
- This lane does not add engine `-scan`.
- This lane does not add CI preview builds.
- This lane does not change multiplayer behavior.
- This lane does not change default single-player gameplay behavior.

Rollback is direct: revert the five source-file edits and this report. No
runtime data, save data, config files, or asset files are affected.

## Next Recommended Slice

After this lane is reviewed and merged, finish the remaining Phase 1 lanes as
separate branches. Keep early-log buffering, engine `-scan` mode, and CI preview
builds independent.
