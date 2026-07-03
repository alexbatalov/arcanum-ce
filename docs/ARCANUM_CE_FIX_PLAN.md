# Arcanum CE Fix Plan

Date: 2026-07-02

This is the working process plan for Arcanum Restored CE implementation lanes. It
exists to keep fixes small, reviewable, and backed by evidence. It is not a
feature roadmap by itself; feature ordering remains in
`docs/ARCANUM_RESTORED_CE_PLAN.md`.

## Lane Rules

1. One lane per branch.
2. One phase per lane unless the plan explicitly says a slice spans phases.
3. No big mixed commits.
4. No drive-by source cleanup in documentation lanes.
5. No engine source changes in Phase 0.
6. No original game assets, DATs, saves, screenshots, or private runtime data in
   the repository.
7. Default single-player behavior must remain unchanged unless the change is
   explicitly gated or configured.

If a change wants to touch multiplayer transport, settings writeback, CE overlay
mounting, DAT writing, and QoL UI in one patch, split it before coding.

## Branch Convention

Use a short branch name that names one lane:

```text
phase1-diagnostic-format-strings
phase1-early-log-buffer
phase3-ce-loose-overlay
```

Do not reuse a lane branch for a new phase after it lands. Start a fresh branch
so the diff tells one story.

## Reports

Every lane gets a dated report under `docs/reports/`.

Required report fields:

- date and branch
- files changed
- source files changed, or confirmation that none changed
- commands run
- runtime evidence path, if any
- known limits and rollback notes
- next recommended slice

Use a result report for the final status of the lane. Use extra inventory or
audit reports only when the lane needs supporting evidence.

## Evidence Expectations

`D:\Arcanum\ThrowawayBuilds\...` is for disposable runtime QA. A throwaway build
is valid evidence only when the report includes:

- exact path
- command or manual action used
- date of the run
- result
- hashes or file counts when the result depends on packaged content

`D:\Arcanum\MilestoneBuilds\...` is for accepted, preserved runtime evidence.
Only promote a throwaway to a milestone after the user or a documented QA gate
accepts it. The repo should store the milestone note or lock report, not copied
game assets.

## Asset Boundary

Never commit original Arcanum game assets, DAT contents, extracted stock files,
private module payloads, runtime saves, or throwaway runtime folders. Generated
fixtures belong under ignored build or temp output paths such as `out/tmp/`.

Docs can name external runtime paths when needed, but they should not embed
private asset contents.

## Single-Player Compatibility

Default single-player behavior is the protected baseline. Any player-visible
change must be one of:

- documentation only
- inactive by default
- behind a config gate
- explicitly approved as a default behavior change

Multiplayer work is especially sensitive: until the transport phase is opened,
default builds must keep `tig_net_is_active()` as a constant false result.

## Current Phase 0 Scope

Phase 0 is documentation and process only:

- create this fix plan
- create or update `docs/ARCANUM_CE_BUILD_WORKLOG.md`
- inventory the existing multiplayer surface in
  `docs/reports/arcanum-ce-multiplayer-surface-inventory.md`

Do not implement yet:

- multiplayer transport
- `net_compat.h` edits
- config systems
- CE overlay mounting
- DAT writer work
- QoL features

## Next Phase 1 Candidates

Recommended first Phase 1 slice: format-string diagnostics hardening over
`tig_debug_printf` and fatal/error call sites. It is narrow, mechanical, and
does not require runtime assets.

Keep early-log buffering, engine `-scan`, and CI preview builds as separate
Phase 1 lanes.
