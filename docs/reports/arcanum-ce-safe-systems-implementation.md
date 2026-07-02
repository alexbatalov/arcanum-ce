# Arcanum CE Safe Systems Implementation

Date: 2026-07-02

## Repositories

| Repo | URL | Branch | Commit |
| --- | --- | --- | --- |
| Fallout 2 CE reference | https://github.com/fallout2-ce/fallout2-ce.git | main | f3caac0bda058a777afa3ebcd9d39d420c48265a |
| Arcanum CE Restored | https://github.com/harleyfolgado/Arcanum-ce-Restored.git | main | 1f939c5894b8a4b1926a285e20f9457114d753ee |

## Implemented

Implemented two safe transfers from Fallout 2 CE's modder-facing systems:

- A read-only DAT/loose override report in `arcanum-ce-module-check`.
- Optional `arcanum-ce.cfg` startup settings read support layered over the existing `HighRes/config.ini` import.

The tool now:

- Tracks loose file paths relative to the scanned root.
- Reads Arcanum-format DAT archive indexes without extracting archive contents.
- Leaves non-archive `.dat` payloads as inventory only instead of counting them as failed archives.
- Normalizes path separators and case for static comparison.
- Prints DAT archive counts, indexed entry counts, and loose-file override matches.
- Treats override matches as informational, not structural scanner issues.

The config loader now:

- Starts from existing defaults.
- Reads `HighRes/config.ini` first when present.
- Reads `arcanum-ce.cfg` second when present, allowing CE-native values to override the legacy import.
- Accepts both UAP-style keys such as `ScrollFPS` and lower snake_case keys such as `scroll_fps`.
- Leaves command-line launch overrides in `src/main.c` unchanged.

## Files Changed

- `src/tools/module_check.c`
- `src/game/highres_config.c`
- `README.md`
- `docs/ARCANUM_CE_CONFIG.md`
- `docs/ARCANUM_CE_MODULE_CHECK.md`
- `docs/reports/fallout2-ce-to-arcanum-ce-systems-audit.md`
- `docs/reports/arcanum-ce-safe-systems-implementation.md`

## Build and Check Results

Commands run:

```console
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --preset windows-x64
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug --target arcanum-ce-module-check
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
git diff --check
out\build\windows-x64\Debug\arcanum-ce-module-check.exe --help
out\build\windows-x64\Debug\arcanum-ce-module-check.exe docs
out\build\windows-x64\Debug\arcanum-ce-module-check.exe out\tmp\module-check-dat-fixture
out\build\windows-x64\Debug\arcanum-ce-module-check.exe "D:\Arcanum\ThrowawayBuilds\Arcanum CE Config Runtime QA 20260702-160533"
```

Results:

- Configure succeeded with Visual Studio 2022 BuildTools CMake.
- `arcanum-ce` and `arcanum-ce-module-check` built successfully in the Windows x64 Debug preset.
- `git diff --check` passed.
- `--help` returned usage text successfully.
- `docs` scan returned zero issues.
- Generated Arcanum-format DAT fixture scan returned zero issues and reported one loose override:
  - DAT archives indexed: 1
  - DAT entries: 1
  - loose files overriding DAT entries: 1
- Runtime config QA used a disposable copy of the 2026-07-02 CE milestone at `D:\Arcanum\ThrowawayBuilds\Arcanum CE Config Runtime QA 20260702-160533`.
- Runtime launch with `arcanum-ce.cfg` set to `width=1280`, `height=720`, and `windowed=1` produced a 1280x720 client window.
- Runtime launch with the same config plus `-geometry=1024x768` produced a 1024x768 client window, confirming command-line geometry still overrides file settings.
- Real-runtime module scan indexed 10 valid DAT archives, 108,663 DAT entries, and 102 loose override matches with zero DAT archive failures.
- The real-runtime module scan still returned non-zero because of existing strict dialog/static warnings outside this DAT-indexing slice.

No original Arcanum game assets were committed. Runtime QA used a disposable local copy outside the repo, and the DAT fixture was generated under `out\tmp\module-check-dat-fixture`.

## Manual Test Steps

1. Build `arcanum-ce` and `arcanum-ce-module-check`.
2. Run it against a loose module or game data folder:

```console
arcanum-ce-module-check <module-or-data-path>
```

3. Review the `DAT archives` and `DAT/loose override report` sections.
4. Confirm any `override:` lines are expected mod overrides.
5. Treat `Issues:` as structural failures; override lines alone are informational.
6. Create `arcanum-ce.cfg` beside `arcanum-ce.exe` with values such as `width=1280`, `height=720`, and `windowed=1`.
7. Launch the game and confirm existing command-line overrides still work for the same launch.

## Rollback Notes

To roll back this implementation, revert:

- `src/tools/module_check.c`
- `src/game/highres_config.c`
- `README.md`
- `docs/ARCANUM_CE_CONFIG.md`
- `docs/ARCANUM_CE_MODULE_CHECK.md`
- `docs/reports/fallout2-ce-to-arcanum-ce-systems-audit.md`
- `docs/reports/arcanum-ce-safe-systems-implementation.md`

No runtime asset-loader behavior, save format, or config writeback behavior was changed.

## Publication

The implementation was committed and pushed to `publish/main`.
