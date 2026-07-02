# Arcanum Restored CE Profile Diagnostics Implementation

Date: 2026-07-02

## Summary

Implemented the first safe profile and startup-diagnostics slice for Arcanum Restored CE.

This change adds:

- `arcanum-ce.cfg` string keys for `module`, `profile`, and `startup_report_path`.
- Optional profile loading from `profiles/<profile>.cfg` or an explicit `.cfg` path.
- Startup report generation to `restored-ce-startup-report.txt` by default.
- Profile expected-path declarations for DATs, module roots, and loose override roots.
- `arcanum-ce-module-check --profile <cfg> <runtime-root>` static validation.

## Boundaries

This is not a new module loader and does not change repository precedence.

The existing loader still controls:

- Root `arcanum*.dat` loading.
- Root `data` loading.
- Module `.dat`, `.patch0` through `.patch9`, and module directory loading.
- Command-line `-mod:` override behavior.

## Runtime Behavior

The startup report records:

- Config source status.
- Selected profile and profile path.
- Selected module.
- Window and scroll settings after command-line overrides.
- Core data path presence.
- Existing module DAT, patch, and directory path presence.
- Profile expected path presence.

Startup report failures are non-fatal. Startup initialization failures now return `EXIT_FAILURE` for the covered startup path instead of reporting success.

## Validation

Checks run:

```console
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
git diff --check
out\build\windows-x64\Debug\arcanum-ce-module-check.exe docs
out\build\windows-x64\Debug\arcanum-ce-module-check.exe out\tmp\module-check-dat-fixture
out\build\windows-x64\Debug\arcanum-ce-module-check.exe --profile out\tmp\profile-check-fixture\profile.cfg out\tmp\profile-check-fixture
out\build\windows-x64\Debug\arcanum-ce-module-check.exe --profile "D:\Arcanum\ThrowawayBuilds\Arcanum CE Config Runtime QA 20260702-160533\profiles\restored-default.cfg" "D:\Arcanum\ThrowawayBuilds\Arcanum CE Config Runtime QA 20260702-160533"
```

Results:

- Full Windows x64 Debug build passed.
- `git diff --check` passed.
- `docs` scan passed with zero issues.
- DAT fixture scan passed with one DAT/loose override.
- Profile fixture scan passed with zero issues and four expected paths.
- Real disposable runtime scan loaded the profile, indexed 10 DAT archives, found 108,663 DAT entries, and found 102 loose override matches with zero profile-specific missing-path warnings.
- Real disposable runtime scan still returned non-zero because of existing strict dialog/static warnings outside this profile slice.

Runtime QA launched a disposable game-root copy with:

```ini
profile=restored-default
module=Arcanum
startup_report=1
windowed=1
width=1280
height=720
intro=0
logos=0
```

and confirmed `restored-ce-startup-report.txt` was created with `status=ok`, `profile_config_loaded=1`, `selected_module=Arcanum`, and all declared expected paths present.

A second launch with `-mod:Vormantown` confirmed command-line module override still wins: the report showed `configured_module=Arcanum` and `selected_module=vormantown`.

## Rollback

Revert:

- `src/game/highres_config.h`
- `src/game/highres_config.c`
- `src/main.c`
- `src/tools/module_check.c`
- `README.md`
- `docs/ARCANUM_CE_CONFIG.md`
- `docs/ARCANUM_CE_MODULE_CHECK.md`
- `docs/ARCANUM_RESTORED_CE_PROFILES.md`

No original game assets are committed by this change.
