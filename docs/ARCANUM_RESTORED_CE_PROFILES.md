# Arcanum Restored CE Profiles

Restored CE profiles are a lightweight diagnostic layer over the existing Arcanum module loader.

They do not change repository order, DAT extraction, save formats, or multiplayer behavior. The goal is to make a runtime self-describing before deeper module, character, and multiplayer work depends on it.

## Config Selection

`arcanum-ce.cfg` can select a profile and default module:

```ini
profile=restored-default
module=Arcanum
startup_report=1
startup_report_path=restored-ce-startup-report.txt
```

If `profile` has no path separator and does not end in `.cfg`, the engine reads it from `profiles/<profile>.cfg`.

The existing `-mod:` command-line switch still wins for that launch.

## Profile File

Example `profiles/restored-default.cfg`:

```ini
module=Arcanum
expected_dat=arcanum1.dat
expected_dat=arcanum2.dat
expected_dat=arcanum3.dat
expected_dat=arcanum4.dat
expected_dat=tig.dat
expected_root=data
expected_module_root=Modules\Arcanum
expected_loose_root=Modules\Arcanum\maps
```

Supported expectation keys are:

- `expected_path`
- `expect_path`
- `expected_dat`
- `expect_dat`
- `expected_root`
- `expect_root`
- `expected_module_root`
- `expected_loose_root`

Keys are case-insensitive. Dashes and underscores are ignored.

## Startup Report

When `startup_report=1`, the engine writes a plain text report with:

- Config source status for `HighRes/config.ini`, `arcanum-ce.cfg`, and the selected profile.
- Selected module and configured module.
- Window and scroll settings after config and command-line processing.
- Core data path presence checks.
- Existing module DAT, patch, and loose directory path checks.
- Profile expected path checks.

The report is informational. Missing expected paths do not stop the game by themselves.

## Module Checker

`arcanum-ce-module-check` can validate profile expectations without launching the game:

```console
arcanum-ce-module-check --profile profiles/restored-default.cfg .
```

Profile-check warnings contribute to the tool's non-zero exit status.

## Boundaries

This first slice is intentionally conservative:

- It does not add a new module manager UI.
- It does not change file repository precedence.
- It does not generate or rewrite profile files.
- It does not claim multiplayer or campaign-co-op correctness.

Those features should build on profile reports and module-check evidence once the baseline runtime is repeatable.
