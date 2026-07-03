# Arcanum CE Overlay

Arcanum CE Restored supports an optional loose overlay folder named `ce/` in
the engine working directory.

The overlay is for CE-owned files only. It must not contain copied original
Arcanum assets, save files, runtime folders, DAT archives, or temporary build
payloads.

## Layout

The allowed loose overlay layout is:

```text
ce/
  mes/
  art/
  rules/
  ui/
  sound/
  data/
```

Phase 3 lane 1 mounts only this loose `ce/` folder. It does not create or read a
packed `arcanum-ce.dat`, does not add DAT writer support, and does not enable
any quality-of-life gameplay features.

## Mounting

When `ce/` is missing, startup logs that the overlay was not found and continues
with the normal stock mount order.

When `ce/` is present, the engine mounts it after stock core data and before
module-specific data. That gives CE-owned core files priority over stock
`arcanum*.dat` and stock loose `data/` files while preserving module-specific
mounts above the CE overlay.

The module scanner reports whether a scanned root contains `ce/` and summarizes
when overlay files shadow stock loose files or DAT entries. Run the scanner with
`--verbose` to list individual overlay shadows.
