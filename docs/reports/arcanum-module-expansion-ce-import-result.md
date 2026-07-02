# Arcanum Module Expansion CE Import Result

Date: 2026-07-02

## Summary

Imported Harley's stock/GOG Arcanum Module Expansion loose-file mod into the local Arcanum CE test build.

No push, PR, or commit was performed. No CE engine source files were changed.

## Paths

- Source stock/GOG Module Expansion runtime: `D:\Arcanum\ThrowawayBuilds\Arcanum Module Expansion V1 Public Beta Nora QA 20260625 A`
- Target Arcanum CE test build: `D:\Arcanum\MilestoneBuilds\Arcanum CE Restored 19 Slot Dark Elf Milestone - PASSED 20260702`
- Repo: `D:\projects\Arcanum CE Restored`
- Backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056`
- Backup manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056\backup-manifest.json`
- Import list: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056\import-list.txt`
- Rollback script: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056\rollback-arcanum-module-expansion-ce-import.ps1`

## What Was Imported

Loose-file import into the CE test build:

- `data\dlg`
- `data\scr`
- `data\mes`
- `modules\Arcanum.PATCH7`

Import file count: 8,133

- 8,132 files copied directly.
- 1 file merged: `data\mes\gamedesc.mes`.
- 8,132 files were newly created in the target.
- 1 existing target file was overwritten after backup via a merge.

The complete exact file list is recorded in `import-list.txt` and every copied/merged file is recorded with source path, target path, hashes, sizes, and action in `backup-manifest.json`.

## Files Backed Up

Backup file count: 1

- Original target file: `D:\Arcanum\MilestoneBuilds\Arcanum CE Restored 19 Slot Dark Elf Milestone - PASSED 20260702\data\mes\gamedesc.mes`
- Backup copy: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056\overwritten\data\mes\gamedesc.mes`
- Original target SHA256: `D216625164A7BBAAE4999B12174ED194ADA39987AF9523D61976AC74863D16E6`
- Original target size: 47 bytes

## MES Merge

Merged `data\mes\gamedesc.mes` instead of replacing it.

- Preserved existing CE target rows:
  - Line 1: `{16104}{Vaelira Night}`
  - Line 2: `{16105}{Kaelor Night}`
- Added source Module Expansion lines: 1,524
- Added unique source row IDs: 1,508
- Added line range in merged target: lines 3-1526
- First added source rows:
  - Line 3: `{30000}{King}`
  - Line 4: `{30001}{King Praetor}`

No other existing target `.mes` files needed merging.

## Scripts And Dialogs

Primary travel chain now present in the CE target:

- `data\dlg\30720CaptainTravelMenu.dlg`
  - Target hash matches source: `061BF35748F2B1A35694FEFE636B8EA7D3A4863543C41E4DA03F2CD6BFEFE50E`
- `data\scr\30720CaptainTravelMenu.scr`
  - Target hash matches source: `5EBFAF23A50D9700C79A9DA10285F8BD038DBF76F1F3770C4166BA6F9269D279`
- `data\dlg\30732CaptainsMateReturnDialog.dlg`
  - Target hash matches source: `3E420EFFD72FF77DF71107BC0BF2C30B060882FFFECE8E56635B8EC55BA34803`
- `data\scr\30732CaptainsMateReturnDialog.scr`
  - Target hash matches source: `45F7245A49D425ED969BAA373351ED0C0692CFC7063AE88BD878CA92F93F17EB`

Captain menu destinations verified in target dialog:

- Vormantown
- Dungeon Crawl 2
- Buried Secrets
- Dusty Dunes
- Hellgate
- Lost Dungeon of Souls
- Time
- Woodmir Race
- Deathmatch
- WhiteSands
- Private Quarters

Captain object script reference `30720` verified in target:

- `modules\Arcanum.PATCH7\maps\Arcanum1-024-fixed\G_D3EAA765_963D_4610_92FB_333D39A4B298.mob`

Return mate script reference `30732` verified in target destination maps:

- `modules\Arcanum.PATCH7\maps\Buried Secrets\G_BURIED_SECRETS_CAPTAINS_MATE_PROOF_J.mob`
- `modules\Arcanum.PATCH7\maps\Deathmatch\G_DEATHMATCH_CAPTAINS_MATE_PROOF_Z.mob`
- `modules\Arcanum.PATCH7\maps\Dungeon Crawl 2\G_80AAF23D_957B_11D5_8699_00010246CD90.mob`
- `modules\Arcanum.PATCH7\maps\Dusty Dunes\G_DUSTY_DUNES_CAPTAINS_MATE_PROOF_K.mob`
- `modules\Arcanum.PATCH7\maps\Hellgate\G_HELLGATE_CAPTAINS_MATE_PROOF_L.mob`
- `modules\Arcanum.PATCH7\maps\Lost Dungeon of Souls\G_LOST_DUNGEON_CAPTAINS_MATE_PROOF_N.mob`
- `modules\Arcanum.PATCH7\maps\Present\G_TIME_CAPTAINS_MATE_PROOF_W.mob`
- `modules\Arcanum.PATCH7\maps\Private Quarters\G_00EB0EEB_29C2_912A_06AB_2D850196A233.mob`
- `modules\Arcanum.PATCH7\maps\WhiteSands\G_WHITESANDS_CAPTAINS_MATE_CANARY_A.mob`
- `modules\Arcanum.PATCH7\maps\Woodmir Race\G_WOODMIR_RACE_CAPTAINS_MATE_PROOF_X.mob`

## MapList Validation

`modules\Arcanum.PATCH7\rules\MapList.mes` is present in the CE target and hash-matches source:

`50109F8BB1DBAE6444E0A181ED41FD44EE0478FE5142C6C59AE3646C8E44C629`

Verified destination rows:

- `{5088}{Private Quarters, 97, 101, WorldMap: 0, Area: 19}`
- `{5089}{Vormantown, 1916, 134}`
- `{5090}{Buried Secrets, 2106, 2075}`
- `{5091}{Dusty Dunes, 21, 19}`
- `{5092}{Hellgate, 80, 70}`
- `{5093}{Lost Dungeon of Souls, 2145, 2019}`
- `{5097}{Dungeon Crawl 2, 1339, 428}`
- `{5098}{Woodmir Race, 286, 226}`
- `{5099}{Deathmatch, 364, 161}`
- `{5101}{WhiteSands, 28582, 20836, WorldMap: 0}`
- `{5102}{DarkRiver, 2032, 2079}`
- `{5103}{MoleBurrow, 2029, 2074}`
- `{5104}{UnderCemetary, 2029, 2074}`
- `{5105}{Tull, 632, 574}`
- `{5106}{NewBerg, 2029, 2074}`

## Files Not Imported

Excluded intentionally:

- `data\dlg\60000Debug.dlg`
- `data\scr\60000Debug.scr`

Reason: these are stock/debug menu files found loose under copied roots but not listed in the manifest, and they are not needed for the Module Expansion travel feature.

Not imported:

- Top-level stock/GOG DATs and executables.
- `ScrMaker.exe`
- `WorldEd.exe`
- Source readmes/proof notes.

Reason: this import is a CE test-build loose mod import, not a stock runtime replacement or public package.

Included even though not manifest-listed:

- `modules\Arcanum.PATCH7\maps\Dungeon Crawl 2\G_9E8B6B50_2C34_4C6C_A1B2_74D3E1239D8A.mob`

Reason: this post-manifest Dungeon Crawl 2 object exists in the live source runtime and is not a debug file.

## CE Compatibility Checks

Read-only checks run:

- Verified imported target hashes for key travel files.
- Verified target `modules\Arcanum.PATCH7` exists and contains expected map folders.
- Verified captain and return dialog/script files are present.
- Verified MapList destination rows are present.
- Verified player-home map folder `modules\Arcanum.PATCH7\maps\Private Quarters`.
- Verified return mate object references in destination maps.
- Ran `arcanum-ce-module-check.exe` against target `data`.
- Ran `arcanum-ce-module-check.exe` against target `modules\Arcanum.PATCH7`.
- Ran the same module-check scans against the source `data` and source `modules\Arcanum.PATCH7` for comparison.
- Searched target `.log` files for missing/error terms; no matches were found.

Module-check results:

- Target `modules\Arcanum.PATCH7`: 49 warnings, 0 errors, 1 script parsed, 0 script failures.
- Source `modules\Arcanum.PATCH7`: 49 warnings, 0 errors, same summary as target.
- Target `data`: 2,288 warnings, 0 errors.
- Source `data`: 2,235 warnings, 0 errors.

The checker warnings are strict parser warnings in dialog/message files and are also present in the source mod. No CE engine compatibility issue was proven by these static checks.

No runtime success is claimed; the game was not launched for this import validation.

## Manual Smoke Test Checklist

1. Launch the Arcanum CE test build from `D:\Arcanum\MilestoneBuilds\Arcanum CE Restored 19 Slot Dark Elf Milestone - PASSED 20260702`.
2. Start or load the main Arcanum module.
3. Travel to Tarant.
4. Find the ship captain.
5. Start captain dialog.
6. Confirm travel options appear.
7. Travel to Private Quarters.
8. Return from Private Quarters if supported.
9. Travel to Vormantown.
10. Travel to Dungeon Crawl 2.
11. Travel to Buried Secrets.
12. Travel to Dusty Dunes.
13. Travel to Hellgate.
14. Travel to Lost Dungeon of Souls.
15. Travel to Time.
16. Travel to Woodmir Race.
17. Travel to Deathmatch.
18. Travel to WhiteSands.
19. Save/load after travel.
20. Confirm no missing dialog/script/art/sound errors.
21. Confirm rollback can restore the pre-import target state if needed.

## Rollback Instructions

Rollback dry run:

```powershell
powershell -ExecutionPolicy Bypass -File "D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056\rollback-arcanum-module-expansion-ce-import.ps1" -WhatIf
```

Rollback execute:

```powershell
powershell -ExecutionPolicy Bypass -File "D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-import\20260702-062056\rollback-arcanum-module-expansion-ce-import.ps1"
```

Rollback behavior:

- Restores overwritten files from `backup-manifest.json`.
- Removes files created by this import only when their current SHA256 still matches the imported SHA256.
- Skips created files that were changed after import.
- Does not touch unrelated CE files.

## Git Status

Command:

```powershell
git status --short --branch
```

Output after import/report creation:

```text
## main...publish/main
 M docs/ARCANUM_CE_MODULE_CHECK.md
 M src/tools/module_check.c
?? docs/reports/
```

No files were staged. No commit was made. No push was performed.

## Final Command Output

Import/backup counts from `backup-manifest.json`:

```json
{
  "importFileCount": 8133,
  "copiedFileCount": 8132,
  "mergedFileCount": 1,
  "backupFileCount": 1,
  "importWarnings": 0,
  "importErrors": 0,
  "createdFileCount": 8132,
  "overwrittenFileCount": 1
}
```

Read-only module checker summary for imported patch folder:

```json
{
  "moduleCheckPath": "D:\\Arcanum\\MilestoneBuilds\\Arcanum CE Restored 19 Slot Dark Elf Milestone - PASSED 20260702\\modules\\Arcanum.PATCH7",
  "warningCount": 49,
  "errorCount": 0,
  "issueLine": "Issues: 49"
}
```

Warnings/errors found:

- Import script warnings: 0
- Import script errors: 0
- Target `.log` missing/error search hits: 0
- Module checker warnings in imported `modules\Arcanum.PATCH7`: 49
- Module checker errors in imported `modules\Arcanum.PATCH7`: 0

## 2026-07-02 Save And Launcher Correction

After user review, the first import was missing the Module Expansion save files needed for testing. The target build also still contains the stock `Arcanum.exe`; launching that executable instead of `arcanum-ce.exe` will not use the CE 19-slot pregen path.

Correction applied:

- Copied 12 source save files from `D:\Arcanum\ThrowawayBuilds\Arcanum Module Expansion V1 Public Beta Nora QA 20260625 A\modules\Arcanum\Save` into the target `modules\Arcanum\Save`.
- Added `Launch CE Module Expansion Test.cmd` in the target root.
- The launcher runs:

```cmd
arcanum-ce.exe -mod:Arcanum
```

Backup/rollback metadata for this correction:

- Backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-save-launch-fix\20260702-154640`
- Manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-save-launch-fix\20260702-154640\backup-manifest.json`
- Rollback script: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-save-launch-fix\20260702-154640\rollback-save-launch-fix.ps1`

Save files copied and hash-verified:

- `Slot0004.bmp`
- `Slot0004.tfaf`
- `Slot0004.tfai`
- `Slot0004004 Tarant.gsi`
- `Slot0005.bmp`
- `Slot0005.tfaf`
- `Slot0005.tfai`
- `Slot00051.gsi`
- `SlotAuto.bmp`
- `SlotAuto.tfaf`
- `SlotAuto.tfai`
- `SlotAutoAuto-Save.gsi`

Counts:

- Save files copied: 12
- Launcher files created: 1
- Existing target files overwritten: 0
- Backup files required: 0

CE 19-slot evidence still present in the target:

- `modules\Arcanum\ce-pregen-debug.log` has `rebuild first=16067 last=16085 count=19 menu_count=20`.
- The same log includes slot/index evidence for:
  - idx 13 / proto 16079 / Malek
  - idx 17 / proto 16083 / Sszara Redclaw
  - idx 18 / proto 16084 / Vaela Night
  - idx 19 / proto 16085 / Kaelor Night

Important launch note: use `Launch CE Module Expansion Test.cmd` or `arcanum-ce.exe`, not the stock `Arcanum.exe`, when testing the 13-19 character slots.

## 2026-07-02 Runtime Module Path Correction

After user runtime review, the Tarant travel captain and captain's mate were still missing in-game. Static inspection found the imported captain objects under `modules\Arcanum.PATCH7`, while the active CE module path was `modules\Arcanum`.

Evidence before correction:

- Active `modules\Arcanum\maps` had 0 MOB references to captain script `30720`.
- Active `modules\Arcanum\maps` had 0 MOB references to captain's mate script `30732`.
- Imported `modules\Arcanum.PATCH7\maps` had 46 references to script `30720`.
- Imported `modules\Arcanum.PATCH7\maps` had 10 references to script `30732`.

Correction applied:

- Overlaid files from target `modules\Arcanum.PATCH7` into target `modules\Arcanum`.
- Created files: 7637.
- Unchanged same-hash files: 3.
- Direct binary overwrites: 0.
- `.mes` files merged: 2.

Merged `.mes` files:

- `modules\Arcanum\mes\gamedesc.mes`
- `modules\Arcanum\oemes\oname.mes`

The merge preserved CE rows `{16104}{Vaelira Night}` and `{16105}{Kaelor Night}` and appended the imported Module Expansion rows/comments.

Backup/rollback metadata:

- Backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-runtime-fix\20260702-155733`
- Manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-runtime-fix\20260702-155733\manifest.csv`

Validation after correction:

- Active `modules\Arcanum\maps` now has 46 MOB references to captain script `30720`.
- Active `modules\Arcanum\maps` now has 10 MOB references to captain's mate script `30732`.
- `modules\Arcanum\rules\MapList.mes` exists.
- `modules\Arcanum\dlg\01729Bed_Use.dlg` exists.
- `modules\Arcanum\scr\01729Bed_Use.scr` exists.

Runtime note: a save made after the failed import may already cache the old Tarant map state. Use imported `Slot0005` or a fresh/new load for the captain/mate retest before treating the map overlay as failed.

## 2026-07-02 Malek And Sszara Visual Correction

After user runtime review, Malek and Sszara still displayed as elves when naked. User clarified:

- Malek should use the brown beta/UAP Bedokaan-looking Malek route with restored prowling.
- Sszara should use the stock red Bedokaan/LIZ body, not the elf or human-frame route.

Static evidence before correction:

- Active Malek proto `016079 - PC.pro` had Art ID `0x2C002000`, matching the prior accepted EFM route.
- Active Sszara proto `016083 - PC.pro` had Art ID `0x24002000`, which is female elf.
- The working Sszara source `D:\Arcanum\ThrowawayBuilds\Slot17 Sszara Stock LIZ Canary P\data\proto\016103 - PC.pro` uses Art ID `0xC6002000`.
- Target Malek brown EFM art existed only under `_disabled-bedokaan-lineage`; live `art\critter\efm` paths were missing.
- Target disabled EFM art hash-matched `D:\Arcanum\MilestoneBuilds\Red Malek Dark Red-Brown EFM Visual Milestone - PASSED`.

Correction applied:

- Patched Sszara Art ID from `0x24002000` to `0xC6002000` at proto offsets `0x4C` and `0x14B`.
- Patched all three target Sszara proto copies:
  - `data\proto\016083 - PC.pro`
  - `modules\Arcanum\proto\016083 - PC.pro`
  - `proto\016083 - PC.pro`
- Activated the existing Malek EFM ART set by copying 75 files from each disabled source into each live destination:
  - `data\art\_disabled-bedokaan-lineage\efm` -> `data\art\critter\efm`
  - `modules\Arcanum\art\_disabled-bedokaan-lineage\efm` -> `modules\Arcanum\art\critter\efm`
  - `art\_disabled-bedokaan-lineage\efm` -> `art\critter\efm`
- Total live ART files copied: 225.
- Existing ART overwrites: 0.
- The disabled `hmf` set was not activated because Sszara is now routed to stock LIZ.

Backup/rollback metadata:

- Backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-lizard-visual-fix\20260702-161938`
- Manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-lizard-visual-fix\20260702-161938\manifest.csv`
- Rollback script: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-lizard-visual-fix\20260702-161938\rollback-lizard-visual-fix.ps1`

Validation after correction:

- `data\proto\016083 - PC.pro`: Art ID `0xC6002000`, hash `87982337EC8E4A2002EFF21A375EC2A747D5DFC33D79C1C26B93087FFA8C0E16`.
- `modules\Arcanum\proto\016083 - PC.pro`: Art ID `0xC6002000`, hash `87982337EC8E4A2002EFF21A375EC2A747D5DFC33D79C1C26B93087FFA8C0E16`.
- `proto\016083 - PC.pro`: Art ID `0xC6002000`, hash `87982337EC8E4A2002EFF21A375EC2A747D5DFC33D79C1C26B93087FFA8C0E16`.
- `data\proto\016079 - PC.pro`: Art ID remains `0x2C002000`, hash `3A28FC46278A8CCD3E1E101196E585ECC77F0EB16A3BA5885672BAD7D3577649`.
- `modules\Arcanum\proto\016079 - PC.pro`: Art ID remains `0x2C002000`, hash `3A28FC46278A8CCD3E1E101196E585ECC77F0EB16A3BA5885672BAD7D3577649`.
- `proto\016079 - PC.pro`: Art ID remains `0x2C002000`, hash `3A28FC46278A8CCD3E1E101196E585ECC77F0EB16A3BA5885672BAD7D3577649`.
- Live EFM ART counts are 75 files each in `data\art\critter\efm`, `modules\Arcanum\art\critter\efm`, and `art\critter\efm`.
- Live HMF ART paths remain missing/disabled.
- No CE engine source changes were made.
- Nothing was committed or pushed.

A failed first PowerShell attempt created partial backup folder `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-lizard-visual-fix\20260702-161836` with one proto backup file before stopping at manifest creation. It did not edit the CE target and is superseded by the completed backup folder above.

## 2026-07-02 Captain Missing And Mate Return Crash Triage

User runtime report:

- Captain was not visible on the Tarant ship.
- Captain's mate was visible in the Dungeon Crawl module.
- Choosing the mate return option to Tarant did nothing and the game then crashed.

Log/runtime evidence checked:

- Windows Application Event Log did not record an Arcanum crash entry for the reported window.
- Target runtime files updated around the test: `modules\Arcanum\ce-pregen-debug.log`, `data\ce-pregen-debug.log`, `data\art\missing.dat`, and TIGCache sound files.
- `ce-pregen-debug.log` only contains character picker trace output; it does not include travel/captain crash diagnostics.
- No CE engine source changes were made.

Static root cause found:

- The active CE `modules\Arcanum\rules\MapList.mes` was missing the source mod rows `{5100}{ShopMap, 31, 29, Type: SHOPPING_MAP}` and `{5107}{WorldEd New, 0, 0}`.
- The imported saves contain `maps\ShopMap`, so the missing `5100` row was not a harmless difference.
- Normal game mode does not load loose map `*.mob` files. It loads the compiled/obfuscated map mobile file, then applies save differences.
- The source Module Expansion main-map compiled file `modules\Arcanum\maps\qrmvs-420-1zjcnpgN` had 19 references to captain script `30720`.
- The active CE main-map compiled file had 17 references to captain script `30720`.
- The two missing references match the loose captain MOB evidence, explaining why the captain was not visible even though the loose MOB existed.
- Because save `mobile.md` differences are applied against the compiled base map, mismatched compiled map files can also fail map load during return-to-Tarant teleport.

Corrections applied:

- Backed up and merged the missing MapList rows into active `modules\Arcanum\rules\MapList.mes`.
- Backed up and replaced Module Expansion compiled map files in active `modules\Arcanum\maps` from the source mod for captain/travel/save maps.
- Overwritten compiled map files: 21.
- Created compiled map files: 1 (`Vormantown` / `cldicnzgdK`).
- Missing source compiled map file: 1 (`NewBerg` / `tgrOlrC`), so nothing was imported for that row.

Backup/rollback metadata:

- MapList backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-maplist-fix\20260702-163900`
- MapList manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-maplist-fix\20260702-163900\manifest.csv`
- MapList rollback: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-maplist-fix\20260702-163900\rollback-maplist-fix.ps1`
- Compiled-map backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-compiled-map-fix\20260702-164427`
- Compiled-map manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-compiled-map-fix\20260702-164427\manifest.csv`
- Compiled-map rollback: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-compiled-map-fix\20260702-164427\rollback-compiled-map-fix.ps1`

Validation after correction:

- Active `MapList.mes` now has `{5100}` at line 118 and `{5107}` at line 125.
- Active main map `qrmvs-420-1zjcnpgN` now hash-matches the source Module Expansion compiled map and has 19 references to captain script `30720`.
- Active `Dungeon Crawl 2` compiled map `2 ylngP cdrtcjQ` now hash-matches source and has 2 references to mate script `30732`.
- Active `Private Quarters` compiled map `hgrignjF rinkvgE` now hash-matches source and has 2 references to mate script `30732`.
- Active `Vormantown` compiled map `cldicnzgdK` now exists, hash-matches source, and has 3 references to captain script `30720`.
- CE module checker still reports zero script parse failures. It exits with warnings because full game data contains many pre-existing dialog/message warnings.

Manual retest needed:

1. Launch the CE test build.
2. Load imported `Slot0005` or `SlotAuto`.
3. In Dungeon Crawl 2, talk to the captain's mate and choose return to Tarant.
4. Confirm the map returns to Tarant without a crash.
5. Confirm the Tarant ship captain is visible on the ship.
6. Start captain dialog and test at least Dungeon Crawl 2, Private Quarters, Vormantown, and return-to-Tarant.

No files were pushed, no PR was opened, and no original game assets were committed to Git.

## 2026-07-02 Vormantown Teleport Invisible-Party Triage

User runtime report after the captain/mate fix:

- Captain's mate return from Dungeon Crawl to the Tarant ship now works.
- Talking to the Tarant captain and choosing Vormantown made the player, Virgil, and the captain disappear without arriving anywhere.
- User had to close the game manually.

Static root cause found:

- Captain script `30720` teleports to map row `{5089}{Vormantown, 1916, 134}`.
- CE `map_open_in_game` resolves that row to `maps\Vormantown` inside the active `Arcanum` module repositories.
- The target CE test build had `modules\Vormantown.dat`, but `gamelib_load_module_data("Arcanum")` does not load standalone `Modules\Vormantown.dat` during an in-module teleport.
- The source working Module Expansion runtime has `modules\Arcanum.PATCH6`; the target CE test build was missing that archive.
- Source `modules\Arcanum.PATCH6` contains the active-module `maps\Vormantown` base directory, `maps\Vormantown\map.prp`, 3,012 Vormantown map files, and 1,877 `townmap\VormanTown` files.
- This explains the invisible-party state: the teleport had already flushed/scheduled moving the party, then the destination base map could not be opened from the active module data.

Correction applied:

- Added the missing source archive to the local CE test build:
  - Source: `D:\Arcanum\ThrowawayBuilds\Arcanum Module Expansion V1 Public Beta Nora QA 20260625 A\modules\Arcanum.PATCH6`
  - Target: `D:\Arcanum\MilestoneBuilds\Arcanum CE Restored 19 Slot Dark Elf Milestone - PASSED 20260702\modules\Arcanum.PATCH6`
- Target did not already have `modules\Arcanum.PATCH6`, so this was a created file, not an overwrite.
- SHA256 after copy: `DE2CF0D4BE16406CF5972A77CCF094388F916CD3A2892A86C3090857EE360650`
- Size after copy: 17,710,392 bytes.

Backup/rollback metadata:

- Backup folder: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-patch6-fix\20260702-165822`
- Manifest: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-patch6-fix\20260702-165822\manifest.csv`
- Rollback: `D:\projects\Arcanum CE Restored\local-work\backups\arcanum-module-expansion-ce-patch6-fix\20260702-165822\rollback-patch6-fix.ps1`
- Rollback behavior: removes the created `modules\Arcanum.PATCH6` only if its current SHA256 still matches the manifest, unless `-Force` is passed.

Validation after correction:

- Target `modules\Arcanum.PATCH6` exists and hash-matches the source archive.
- Active `modules\Arcanum\rules\MapList.mes` has `{5089}{Vormantown, 1916, 134}` at line 107.
- Target `modules\Arcanum.PATCH6` contains `maps\Vormantown` as a directory entry.
- Target `modules\Arcanum.PATCH6` contains `maps\Vormantown\map.prp`.
- Target `modules\Arcanum.PATCH6` contains 3,012 `maps\Vormantown` files.
- Target `modules\Arcanum.PATCH6` contains `townmap\VormanTown\VormanTown.tmi`.
- No CE engine source changes were made.

Manual retest needed:

1. Launch the CE test build fresh.
2. Load the save used for the captain/mate test.
3. Use the captain's mate to return from Dungeon Crawl to the Tarant ship.
4. Talk to the Tarant ship captain.
5. Choose Vormantown.
6. Confirm the party arrives in Vormantown and remains visible.
7. Save/load after arrival.
8. Test return travel from Vormantown if the mod supports it.
9. If it still fails, capture the newest `ce-pregen-debug.log`, Windows Application Event Log entry, and the exact save slot used.

No files were pushed, no PR was opened, and no original game assets were committed to Git.
