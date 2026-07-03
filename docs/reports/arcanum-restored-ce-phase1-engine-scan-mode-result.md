# Arcanum Restored CE Phase 1 Engine Scan Mode Result

Date: 2026-07-02

Branch: `phase1-engine-scan-mode`

Worktree:

```text
D:\projects\Arcanum CE Restored - phase1 scan mode
```

Base:

```text
origin/main @ a5e606f5
```

## Scope

This lane implements the first Phase 1 engine `-scan` slice only.

The existing read-only `arcanum-ce-module-check` scanner is now callable from
the engine binary:

```text
arcanum-ce -scan [--verbose] [--profile <cfg>] <module-or-data-path>
```

When `-scan` is present, the process forwards the remaining arguments to the
module checker and exits before high-resolution config loading, TIG
initialization, window creation, module loading, or gameplay startup.

Kept out of scope:

- new scanner coverage
- config systems
- multiplayer transport
- CE overlay mounting
- DAT writer work
- QoL features
- CI preview builds
- gameplay behavior changes

## Files Changed

Source and build files:

- `CMakeLists.txt`
- `src/main.c`
- `src/tools/module_check.c`
- `src/tools/module_check.h`

Docs:

- `docs/ARCANUM_CE_MODULE_CHECK.md`
- `docs/reports/arcanum-restored-ce-phase1-engine-scan-mode-result.md`

No original Arcanum game assets, DAT contents, saves, screenshots, or private
runtime files were added.

## Changes Made

- Added `src/tools/module_check.h` with a public
  `arcanum_ce_module_check_main` entry point.
- Refactored `src/tools/module_check.c` so the existing scanner implementation
  can be called by both:
  - standalone `arcanum-ce-module-check`
  - engine `arcanum-ce -scan`
- Kept the standalone module-check executable by compiling a tiny `main`
  wrapper only when `ARCANUM_CE_MODULE_CHECK_STANDALONE` is defined.
- Linked `src/tools/module_check.c` into `arcanum-ce`.
- Added `handle_scan_mode` in `src/main.c`.
- Documented the engine `-scan` entry point in
  `docs/ARCANUM_CE_MODULE_CHECK.md`.

## Behavior Notes

Normal startup remains unchanged when `-scan` is absent.

The `-scan` branch runs before:

- `highres_config_load`
- `tig_init`
- window/video initialization
- game library initialization
- module loading
- main menu or gameplay entry

The scan path is read-only and reuses the existing module checker behavior. It
does not extract archives, write inputs, run scripts, or prove runtime behavior.

`arcanum-ce -scan` requires the same explicit path as
`arcanum-ce-module-check`. If no path is supplied, it prints usage and exits
with failure.

## Commands Run

```text
git worktree add -b phase1-engine-scan-mode "D:\projects\Arcanum CE Restored - phase1 scan mode" origin/main
rg -n "engine `-scan`|-scan mode|scan mode|module-check|arcanum-ce-module-check|Phase 1" docs src first_party -g "*.md" -g "*.c" -g "*.h"
rg -n "add_executable|module_check|arcanum-ce|src/tools|main.c|target_sources|target_link" CMakeLists.txt src first_party -g "CMakeLists.txt" -g "*.cmake"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --preset windows-x64
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock\data"
out\build\windows-x64\Debug\arcanum-ce.exe -scan "D:\Arcanum\dont touch\Arcanum-gogstock\data"
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock"
out\build\windows-x64\Debug\arcanum-ce.exe -scan "D:\Arcanum\dont touch\Arcanum-gogstock"
out\build\windows-x64\Debug\arcanum-ce.exe -scan
out\build\windows-x64\Debug\arcanum-ce.exe -scan --help
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

The first full worktree build surfaced existing warnings in untouched files:

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

These warnings are from untouched files and were not fixed in this lane.

Standalone module-check, stock loose data:

```text
Path: D:\Arcanum\dont touch\Arcanum-gogstock\data
Directories scanned: 3
Files scanned: 1424
Issues: 0
```

Engine `-scan`, stock loose data:

```text
Path: D:\Arcanum\dont touch\Arcanum-gogstock\data
Directories scanned: 3
Files scanned: 1424
Issues: 0
```

Standalone module-check, full stock root:

```text
Path: D:\Arcanum\dont touch\Arcanum-gogstock
Directories scanned: 15
Files scanned: 1584
DAT archives indexed: 7
DAT entries: 107883
Issues: 0
```

Engine `-scan`, full stock root:

```text
Path: D:\Arcanum\dont touch\Arcanum-gogstock
Directories scanned: 15
Files scanned: 1584
DAT archives indexed: 7
DAT entries: 107883
Issues: 0
```

Engine `-scan --help`:

```text
Result: usage printed, exit code 0
```

Engine `-scan` without a path:

```text
Result: usage printed, exit code 1
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

- This lane adds an engine entry point to the existing scanner; it does not add
  new scanner coverage.
- The scanner remains static/read-only evidence. It does not initialize the
  game, run scripts, or prove runtime behavior.
- This lane is based on `origin/main`; it does not include the separate
  `phase1-diagnostic-format-strings` or `phase1-early-log-buffer` commits
  unless those lanes are later merged into main.

Rollback is direct: revert the source/build/doc edits listed above. No runtime
data, save data, config files, or asset files are affected.

## Next Recommended Slice

After the Phase 1 format-string, early-log, and engine `-scan` lanes are
reviewed and merged, open the CI preview-builds branch as the remaining Phase 1
safety-tools slice.
