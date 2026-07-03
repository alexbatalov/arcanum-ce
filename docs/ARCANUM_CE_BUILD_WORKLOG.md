# Arcanum CE Build Worklog

This file records implementation lanes and the evidence required before a lane
is considered ready to commit or push. It is intentionally process-heavy so
future changes do not blend unrelated systems into one diff.

## Worklog Rules

- One lane per branch.
- One dated report under `docs/reports/` per lane.
- No source changes in documentation-only lanes.
- No original game assets in git.
- Use `D:\Arcanum\ThrowawayBuilds\...` for disposable runtime proof.
- Use `D:\Arcanum\MilestoneBuilds\...` only for accepted preserved runtimes.
- Do not claim runtime proof from static inspection alone.
- Do not make big mixed commits.
- Do not push unless the user asks for a push in that turn.

## Evidence Template

```text
Date:
Branch:
Lane:
Files changed:
Source files changed:
Build:
Static checks:
Runtime evidence:
ThrowawayBuilds path:
MilestoneBuilds path:
Pushed:
Notes:
```

## 2026-07-02 - Phase 0 Documentation

Branch: `main`

Lane: Phase 0 docs/process only from `docs/ARCANUM_RESTORED_CE_PLAN.md`.

Files planned:

- `docs/ARCANUM_CE_FIX_PLAN.md`
- `docs/ARCANUM_CE_BUILD_WORKLOG.md`
- `docs/reports/arcanum-ce-multiplayer-surface-inventory.md`
- `docs/reports/arcanum-restored-ce-phase0-result.md`

Source files changed: none planned.

Build: not planned. This lane is documentation only and the user explicitly
asked not to build unless docs tooling requires it.

Static checks planned:

- `git diff --check`
- markdown/lint checks if a markdown tool is available

Runtime evidence: none. Phase 0 does not create or launch a runtime.

ThrowawayBuilds path: not applicable.

MilestoneBuilds path: not applicable.

Pushed: no. The user explicitly said not to push.

Notes:

- Multiplayer remains frozen at inventory only.
- Do not edit `src/net_compat.h`.
- Do not add a transport layer.
- Do not add config, overlay, DAT writer, or QoL implementation work in this
  lane.
