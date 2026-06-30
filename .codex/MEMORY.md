# Kivo Video Local Architecture Discipline Memory

This repository has a hard architectural gate.

Before any implementation, the agent must follow this order:

1. Concept
2. Planning design
3. Natural family tree
4. Single-file single-responsibility file plan
5. Implementation
6. Minimal verification
7. Evidence report

## Non-negotiable rules

- Never start coding before the natural family tree is designed.
- Never dump unrelated responsibilities into one large file.
- Every folder is a functional family.
- Every subfolder is a sub-family.
- Every `.cpp`, `.hpp`, `.qml`, `.md`, and test file must have one clear responsibility.
- If a file grows beyond 250 lines, the agent must check whether it should be split.
- If a modified or newly created file exceeds 300 lines, the agent must stop and report unless there is a justified exception.
- Do not create giant `support`, `helper`, `utils`, `manager`, or `runtime` files that swallow multiple responsibilities.
- Do not mix lifecycle, policy, snapshot, query, command, rendering, adapter, UI, and test responsibilities in one file.
- Do not flatten the architecture into a pile of files.
- Do not bypass the natural family tree to finish faster.

## Required pre-implementation output

Before changing code, the agent must output:

1. Current task scope
2. Natural family tree
3. File responsibility table
4. Expected files to modify/create
5. Line budget for each modified/created file
6. Boundary rules
7. Minimal verification plan

If this pre-implementation output is missing, the agent must not write code.

## Required final report

After implementation, the agent must report:

1. Modified/created file list
2. Responsibility of each file
3. Line count of each modified/created file
4. Whether any file exceeds 250 lines
5. Whether any file exceeds 300 lines
6. Whether any giant helper/support/runtime file was created
7. Whether the natural family tree was preserved
8. Build/test/scan evidence

## Kivo Video specific hard locks

- The only allowed workspace is `C:\kivo video`.
- Do not create a new worktree, clone, sibling directory, external build tree, external vcpkg, or external temp workspace.
- Do not switch branches unless explicitly instructed.
- Do not push unless explicitly instructed.
- Do not touch non-task dirty files.
- QML must not directly call P4/P5/P6/P8 internals.
- PlayerRuntimeAdapter must route through P7 public API.
- Adapter must not directly include or call FFmpeg, WASAPI, mpv, P5, P6, or P8 internals.
- Public headers must not leak Qt, FFmpeg, WASAPI, or mpv types.
- Typed contract must never be reported as real runtime connected.
- Fake harness must never be reported as production runtime connected.

## Stop conditions

The agent must stop and report before continuing if:

- The current branch or HEAD does not match the task baseline.
- The task requires a new architectural family but no family tree was designed.
- A file exceeds 300 lines.
- A file starts collecting unrelated responsibilities.
- A helper/support/runtime file becomes a dumping ground.
- The implementation would bypass P7 or directly connect UI/adapter to lower planes.
- Build/test failures require touching out-of-scope files.