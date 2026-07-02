# Arcanum CE Module Check

`arcanum-ce-module-check` is a read-only companion CLI for scanning loose Arcanum module and data files without launching the game.

It is the first implementation slice of the compatibility-scanner work. The goal is to make script, dialog, message, and asset-route problems
visible before a user has to guess from runtime behavior.

## Usage

```console
$ arcanum-ce-module-check [--verbose] <module-or-data-path>
```

The command exits with a non-zero status when it finds structural issues or cannot scan the requested path.

## Current Coverage

- Recursively inventories loose `.scr`, `.dlg`, `.mes`, `.pro`, `.mob`, `.sec`, `.map`, `.art`, sound, `.dat`, and other files.
- Validates `.scr` structure using Arcanum CE's serialized script layout.
- Reports script condition and action ID histograms.
- Flags script condition IDs outside `ScriptConditionType`.
- Flags script action IDs outside `ScriptActionType`.
- Parses `.dlg` brace fields and checks duplicate lines.
- Checks PC dialog responses for missing or non-NPC jump targets, matching the existing runtime `dialog_check` behavior.
- Parses `.mes` brace fields and checks duplicate message numbers.

## Boundaries

The tool does not modify inputs, initialize the game, run scripts, extract DAT archives, resolve object attachments, or prove runtime behavior.
It reports static evidence only.

Future scanner work should add DAT index reading, object/prototype script attachment checks, MES reference validation, and art/sound route resolution.
