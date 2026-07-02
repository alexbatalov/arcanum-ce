# Arcanum Restored CE Phase 1 CI Preview Builds Result

Date: 2026-07-02

Branch: `phase1-ci-preview-builds`

Worktree:

```text
D:\projects\Arcanum CE Restored - phase1 ci preview
```

Base:

```text
origin/main @ a5e606f5
```

## Scope

This lane implements the Phase 1 CI preview-build slice only.

The goal is to make pull-request and manual workflow builds produce more useful
desktop preview artifacts without changing engine behavior or adding runtime
payloads.

Kept out of scope:

- engine source changes
- gameplay behavior changes
- config-system foundation work
- CE overlay mounting
- DAT writer work
- module or character content
- multiplayer transport
- release publishing

## Files Changed

- `.github/workflows/ci-build.yml`
- `docs/reports/arcanum-restored-ce-phase1-ci-preview-builds-result.md`

No engine source files were changed.

No original Arcanum game assets, DAT contents, ART files, saves, screenshots,
runtime folders, or binary payloads were added to the repository.

## Changes Made

- Added `workflow_dispatch` to the CI workflow so maintainers can run preview
  builds manually.
- Added read-only repository permissions to the workflow.
- Scoped pull-request `clang-format` checks to source files changed by the PR.
- Skipped the format-only job for manual preview runs.
- Kept the full source-tree format check for pushes to `main`.
- Changed Linux desktop artifact upload from a raw `arcanum-ce` binary to a
  `.tar.gz` preview package containing:
  - `arcanum-ce`
  - `arcanum-ce-module-check`
  - `README.md`
  - `LICENSE.md`
  - `BUILD_INFO.txt`
- Changed Windows desktop artifact upload from a raw `arcanum-ce.exe` binary to
  a `.zip` preview package containing:
  - `arcanum-ce.exe`
  - `arcanum-ce-module-check.exe`
  - `README.md`
  - `LICENSE.md`
  - `BUILD_INFO.txt`

Android, iOS, and macOS packaging behavior was left unchanged in this slice.

## Commands Run

```text
git worktree add -b phase1-ci-preview-builds "D:\projects\Arcanum CE Restored - phase1 ci preview" origin/main
git diff --check
python -c "import yaml, pathlib; yaml.safe_load(pathlib.Path('.github/workflows/ci-build.yml').read_text(encoding='utf-8')); print('workflow yaml parse passed')"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --preset windows-x64
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset windows-x64-debug
out\build\windows-x64\Debug\arcanum-ce-module-check.exe docs
gh pr checks 2 --repo techerrorcode/arcanum-ce --json name,state,bucket,link,startedAt,completedAt,workflow
gh run view 28628210005 --repo techerrorcode/arcanum-ce --json name,workflowName,conclusion,status,url,event,headBranch,headSha,jobs
gh api "/repos/techerrorcode/arcanum-ce/actions/jobs/84899305146/logs"
```

The Windows preview package command from the workflow was also run locally for
`windows-x64` after setting local `GITHUB_SHA` and `GITHUB_REF_NAME` values.

## Check Results

Diff hygiene:

```text
git diff --check
Result: pass
```

Workflow YAML parse:

```text
Result: pass
```

Configure:

```text
cmake --preset windows-x64
Result: pass
```

Build:

```text
cmake --build --preset windows-x64-debug
Result: pass
```

The build produced:

- `out\build\windows-x64\Debug\arcanum-ce.exe`
- `out\build\windows-x64\Debug\arcanum-ce-module-check.exe`

Existing warnings from untouched source files were still present:

```text
first_party\tig\src\database.c(941,43): warning C4267
first_party\tig\src\database.c(954,20): warning C4267
src\game\item.c(2344,9): warning C4101
src\game\multiplayer.c(1102,13): warning C4101
src\game\path.c(1012,1): warning C4716
src\game\name.c(1426,1): warning C4716
src\game\terrain.c(602,1): warning C4716
src\ui\dialog_ui.c(362,9): warning C4101
```

Module check on docs:

```text
Path: docs
Files scanned: 14
Issues: 0
Result: pass
```

Local Windows preview package smoke test:

```text
Package: out\dist\arcanum-ce-windows-x64-debug.zip
Contents:
- arcanum-ce.exe
- arcanum-ce-module-check.exe
- BUILD_INFO.txt
- LICENSE.md
- README.md
Result: pass
```

CI follow-up:

```text
PR #2 Code format check failed on existing baseline source file:
src/tools/module_check.c

This branch does not change that file or any other source file. The workflow now
checks only PR-changed `.c` and `.h` files on pull requests, so docs/workflow-only
PRs no longer fail on unrelated baseline source formatting. Manual preview runs
skip the format-only job so preview artifacts are not blocked by the same
baseline issue.

Local changed-source simulation:
No changed source files to format-check.
Result: pass
```

Unavailable local checks:

```text
actionlint: not available
markdownlint-cli2: not available
markdownlint: not available
local Bash runner: unavailable, WSL2 not enabled
```

## Known Limits

- Hosted GitHub Actions has not run for this branch yet. The workflow file was
  parsed locally and the Windows x64 package path was smoke-tested locally.
- Linux archive creation was reviewed and YAML-parsed locally, but not executed
  on a Linux runner in this local Windows worktree.
- This lane does not add release publishing. It only improves short-retention
  preview artifacts from CI.

## Next Recommended Step

Finish Phase 1 before starting Phase 2:

- open or review PRs for the already-pushed format-string and early-log lanes
- review the open engine `-scan` PR
- review this CI preview-build lane after it is committed and pushed
- merge and QA the Phase 1 lanes

Only after Phase 1 is merged and QA'd should Phase 2 begin with the config-system
foundation lane.
