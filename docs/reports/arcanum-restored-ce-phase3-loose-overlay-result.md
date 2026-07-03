# Phase 3 Loose CE Overlay Result

Branch: `phase3-ce-loose-overlay`

Commit hash before work: `09305dc9e41aef2406268c13fb220746bdc66868`

Status: implementation validated locally; nothing pushed.

## Scope

This lane implements the Phase 3 lane 1 loose CE overlay only.

It does not add DAT writer work, does not create `arcanum-ce.dat`, does not
commit original game assets, does not enable quality-of-life features, and does
not touch multiplayer.

## Files Changed

- `.gitignore`
- `CMakeLists.txt`
- `docs/ARCANUM_CE_OVERLAY.md`
- `docs/reports/arcanum-restored-ce-phase3-loose-overlay-result.md`
- `src/game/ce_overlay.c`
- `src/game/ce_overlay.h`
- `src/game/gamelib.c`
- `src/tools/module_check.c`

## Mount Order

Before this lane:

1. `tig.dat`
2. `.`
3. `arcanum*.dat`
4. `data`
5. selected module DAT, patch DATs, and loose module directory when a module is
   loaded

TIG repository adds are inserted at the head of the search list, so newer mounts
have higher lookup priority.

After this lane:

1. `tig.dat`
2. `.`
3. `arcanum*.dat`
4. `data`
5. optional `ce`
6. selected module DAT, patch DATs, and loose module directory when a module is
   loaded

In lookup priority terms, `ce/` sits above stock core data when present. Later
module mounts still sit above the CE overlay.

## Detection

`ce/` is detected with native filesystem directory checks against the process
working directory. This avoids accidentally resolving `ce` through TIG's virtual
repository search.

If `ce/` is missing, startup logs:

```text
Arcanum CE: ce/ overlay not found, continuing normally.
Arcanum CE: ce/ overlay root path: ce
```

Missing `ce/` is not an error.

If `ce/` is present, startup mounts it with `tig_file_repository_add("ce")` and
logs:

```text
Arcanum CE: ce/ overlay mounted.
Arcanum CE: ce/ overlay root path: ce
```

## Override Reporting

Override reporting was added to the read-only module scanner. The scanner now
prints a CE overlay report showing:

- whether `ce/` was present in the scanned root
- how many overlay files were tracked
- how many overlay files shadow loose files
- how many overlay files shadow DAT entries

Individual shadow paths are listed only in `--verbose` scanner output.

## Asset Boundary

The runtime `ce/` folder is ignored by Git through `.gitignore` so local overlay
payloads and original game assets are not accidentally committed.

## Git Status

At handoff, the branch has only this lane's local changes:

```text
## phase3-ce-loose-overlay...origin/main
 M .gitignore
 M CMakeLists.txt
 M src/game/gamelib.c
 M src/tools/module_check.c
?? docs/ARCANUM_CE_OVERLAY.md
?? docs/reports/arcanum-restored-ce-phase3-loose-overlay-result.md
?? src/game/ce_overlay.c
?? src/game/ce_overlay.h
```

## Validation Results

- `git diff --check`: passed.
- `cmake --preset windows-x64`: passed with the Visual Studio bundled CMake.
- `cmake --build --preset windows-x64-debug`: passed.
- `arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock\data"`:
  passed with `Issues: 0`.
- `arcanum-ce-module-check.exe "D:\Arcanum\dont touch\Arcanum-gogstock"`:
  passed with `Issues: 0`.
- `arcanum-ce.exe -scan docs`: passed with `Issues: 0`.
- Runtime smoke without `ce/`: reached `gamelib_init`, logged that `ce/` was
  not found, and continued to the expected missing-data failure in the temp
  root.
- Runtime smoke with empty `ce/`: reached `gamelib_init`, mounted `ce/`, and
  continued to the expected missing-data failure in the temp root.
- Runtime smoke with `ce/mes/overlay-smoke.txt`: reached `gamelib_init`,
  mounted `ce/`, and continued to the expected missing-data failure in the temp
  root.
- Scanner smoke with harmless text files under both `ce/mes/` and `mes/`:
  passed with `Issues: 0` and reported one CE overlay loose-file shadow.

The Windows build still reports pre-existing warnings outside this lane,
including return-value warnings in `name.c`, `path.c`, and `terrain.c`. No new
warning was reported for `ce_overlay.c` or the scanner changes.

## Manual Test Steps

Temporary smoke roots were created under:

```text
out/tmp/phase3-overlay-validation/
```

They contain only local test files, a hardlink to the stock local `tig.dat`, and
small validation config files. They are under the ignored `out/` tree and are
not part of the Git diff.

Runtime commands used:

```text
arcanum-ce.exe -nosound -window -geometry 800x600
```

The controlled temp roots intentionally omit stock game data beyond `tig.dat`.
The expected result is an early startup failure after the overlay mount point,
not a playable session.

## Risks

- `ce/` changes effective file lookup priority only when present.
- Current overlay support is loose-folder only; packed `arcanum-ce.dat` remains a
  later Phase 3 lane.

## Next Recommended Phase 3 Lane

After review and merge, add a small read-only overlay fixture/scanner validation
lane or begin packed DAT planning without implementing the writer yet.

## Push Status

Nothing was pushed.
