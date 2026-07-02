# Arcanum Module Expansion CE Import Audit

Date: 2026-07-02

## Scope

This is a local test-build import audit for Harley's stock/GOG Arcanum Module Expansion mod. The user prompt still contained placeholder paths, so the paths below were inferred from the latest local Module Expansion evidence and then inspected directly.

No files were copied to the CE target during this audit phase.

## Paths Inspected

- Source stock/GOG Module Expansion runtime: `D:\Arcanum\ThrowawayBuilds\Arcanum Module Expansion V1 Public Beta Nora QA 20260625 A`
- Target Arcanum CE test build: `D:\Arcanum\MilestoneBuilds\Arcanum CE Restored 19 Slot Dark Elf Milestone - PASSED 20260702`
- Arcanum CE repo: `D:\projects\Arcanum CE Restored`
- Clean stock comparison path used for orientation: `D:\Arcanum\dont touch\Arcanum-gogstock`

## Source Identification

The source runtime is a loose-file stock/GOG-style Arcanum build with a Module Expansion manifest:

- `manifest.json`
  - SHA256: `071C529BD99A83EA3A07FD0A6C118EE335EBF098B7CF68ED286FA796C9A54708`
  - `payloadId`: `module-expansion-v1-private-quarters-player-bedroom`
  - `title`: `Arcanum Restored V1 Module Expansion - WhiteSands, Deathmatch, Private Quarters, and Dusty Dunes Patch`
  - `datRepacked`: `false`
  - `copiedRoots`: `modules/Arcanum.PATCH7`, `data/dlg`, `data/scr`, `data/mes`
- `README-MODULE-EXPANSION-V1-PUBLIC-BETA-NORA-QA.txt` confirms the Tarant ship captain travel workflow and Dungeon Crawl 2/Nora QA context.

The source also contains normal stock/GOG files, DATs, and tools such as `Arcanum.exe`, `Arcanum5.dat`, `ScrMaker.exe`, and `WorldEd.exe`. Those are not part of the planned import.

## Detected Mod Layout

The mod is installed as loose overrides, not as a new DAT:

- `data\dlg`: 164 files in source root, 163 manifest-listed import files.
- `data\scr`: 315 files in source root, 314 manifest-listed import files.
- `data\mes`: 14 manifest-listed import files.
- `modules\Arcanum.PATCH7`: 7,642 files in source root, 7,641 manifest-listed import files.

Three loose files under copied roots are not listed in the manifest:

- `data\dlg\60000Debug.dlg`
- `data\scr\60000Debug.scr`
- `modules\Arcanum.PATCH7\maps\Dungeon Crawl 2\G_9E8B6B50_2C34_4C6C_A1B2_74D3E1239D8A.mob`

The `60000Debug` dialog/script pair is a debug menu and should not be imported. The extra Dungeon Crawl 2 `.mob` was created after the manifest and is not a debug file, so it is planned for import to preserve the live source runtime behavior.

Planned import set: 8,133 files.

## Map And Module Files Found

`modules\Arcanum.PATCH7\maps` contains these map folders:

- `Arcanum1-024-fixed`
- `Buried Secrets`
- `DarkRiver`
- `Deathmatch`
- `Dungeon Crawl 2`
- `Dusty Dunes`
- `Future`
- `Hellgate`
- `Lost Dungeon of Souls`
- `MoleBurrow`
- `Past`
- `Present`
- `Private Quarters`
- `Tull`
- `UnderCemetary`
- `WhiteSands`
- `Woodmir Race`

`Private Quarters` appears to be the player-home map. Return captain's mate objects using script/dialog `30732` were found in multiple destination map folders, including `Private Quarters`, `Deathmatch`, `Dungeon Crawl 2`, `Dusty Dunes`, `Hellgate`, `Lost Dungeon of Souls`, `Present`, `WhiteSands`, and `Woodmir Race`.

## Captain And Travel Chain

Primary captain travel files:

- `data\dlg\30720CaptainTravelMenu.dlg`
  - SHA256: `061BF35748F2B1A35694FEFE636B8EA7D3A4863543C41E4DA03F2CD6BFEFE50E`
- `data\scr\30720CaptainTravelMenu.scr`
- `data\dlg\30732CaptainsMateReturnDialog.dlg`
  - SHA256: `3E420EFFD72FF77DF71107BC0BF2C30B060882FFFECE8E56635B8EC55BA34803`
- `data\scr\30732CaptainsMateReturnDialog.scr`
- `modules\Arcanum.PATCH7\rules\MapList.mes`
  - SHA256: `50109F8BB1DBAE6444E0A181ED41FD44EE0478FE5142C6C59AE3646C8E44C629`

Captain menu destinations in `30720CaptainTravelMenu.dlg`:

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

The captain object was identified by compiled script reference `30720` in:

- `modules\Arcanum.PATCH7\maps\Arcanum1-024-fixed\G_D3EAA765_963D_4610_92FB_333D39A4B298.mob`

This is the Tarant/Arcanum map patch object for the travel hub. Exact object coordinates were not decoded from the binary `.mob` in this audit.

Return dialog chain:

- `30732CaptainsMateReturnDialog.dlg` offers `Return to Tarant`.
- Compiled script reference `30732` appears in the destination map `.mob` files listed above.

No loose `.ssl` script source was found in the source runtime. Global flag/variable usage was therefore not inferred from compiled `.scr` binaries.

## MapList Rows

Relevant source `modules\Arcanum.PATCH7\rules\MapList.mes` rows:

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

## Target Conflict Audit

The target CE build currently has no `modules\Arcanum.PATCH7` folder, so the patch module layer is almost entirely new to the target.

Dry-run comparison of the planned manifest import against the target found:

- Existing target files in planned import set: 1
- Existing target files with different hash: 1
- New target files: 8,131 from manifest, plus 1 extra non-debug Dungeon Crawl 2 `.mob`
- Missing source files from manifest: 0

The only overwrite/merge candidate is:

- `data\mes\gamedesc.mes`
  - Source SHA256: `2C8D5E6D6EBC19F73960DB2C8A5AD8FC5F050886735A44A59C523ED2BBA1D2BA`
  - Target SHA256: `D216625164A7BBAAE4999B12174ED194ADA39987AF9523D61976AC74863D16E6`
  - Target currently contains CE restored descriptor rows `{16104}{Vaelira Night}` and `{16105}{Kaelor Night}`.

The Malek/Sszara lizard pregen files are not in the Module Expansion import set:

- `data\proto\016079 - PC.PRO`
- `data\proto\016083 - PC.PRO`

This import plan should not overwrite those files.

## CE Compatibility Risks

- `data\mes\gamedesc.mes` must be merged, not blindly overwritten, so existing CE 19-slot descriptor rows are preserved.
- The mod relies on loose `modules\Arcanum.PATCH7` layout. The CE target already uses loose `data` and `modules\Arcanum` data, so this is the safest initial layout, but runtime verification is still required.
- Compiled `.scr` files were not decompiled in this audit. Any global/PC variable conflicts can only be proven through runtime testing or script decompilation.
- Module Expansion source metadata is slightly stale: `manifest.fileCount` is 8,179, but the manifest `files` array has 8,132 entries and copied roots contain 8,135 files. The two debug files are intentionally excluded; the one post-manifest Dungeon Crawl 2 `.mob` is included.
- No CE engine compatibility issue has been proven during the audit.

## Safe Import Plan

1. Use loose-file import into the target CE test build.
2. Build the copy list from `manifest.json` `files`, then add `modules\Arcanum.PATCH7\maps\Dungeon Crawl 2\G_9E8B6B50_2C34_4C6C_A1B2_74D3E1239D8A.mob`.
3. Exclude `data\dlg\60000Debug.dlg` and `data\scr\60000Debug.scr`.
4. Back up every existing target file before writing.
5. Merge `data\mes\gamedesc.mes` by preserving target rows and adding source rows not already present.
6. Copy all other planned files into matching relative paths.
7. Validate target file hashes for copied files.
8. Verify captain dialog/script files, MapList rows, destination maps, and return mate objects are present in the CE test build.
9. Create a rollback script from the backup manifest.
10. Do not edit CE engine source unless a small, proven path/layout issue appears. No engine change is currently planned.

Proceeding to Phase 2 is reasonable because the mod files, travel chain, target conflict, and safe import layout have been identified with high confidence.
