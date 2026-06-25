# Current State

Updated: 2026-06-25

## Done

- Local Codex memory created for the Kivo Video iron gate.
- Backend goal persisted in `.codex/memory/backend-goal.md`.
- Detailed execution checklist created at `docs/roadmap/backend_task_checklist.md`.
- P2 Real Playback Backend Implementation Spec V1.5 added to `docs/roadmap/backend_task_checklist.md`.
- P2 Real Local Closure V10 added to `docs/roadmap/backend_task_checklist.md` as `P2-REAL-LOCAL-CLOSURE-V10`, backed by PDF, pasted text, and docx sources.
- P2 Remaining Full Coverage Roadmap V8 added to `docs/roadmap/backend_task_checklist.md` as `P2-REMAINING-COVERAGE-V8`; next executable child task is `P2-LOCAL-REPAIR-DELTA-001`.
- V8 direct execution cards created at `docs/roadmap/p2_remaining_coverage_v8_execution_cards.md`; next assistant should execute cards in order starting at `V8-001`.
- Latest checklist cross-check found a V8/V10 issue: some current V10 runtime claims mention stub/simulator evidence, which V8 forbids as `RUNTIME_PASS`; next work starts with audit, downgrade, or real-runtime repair before remote/provider work.
- Latest governance verification is currently blocked by root-level temporary build outputs such as `build_err.txt`, which the no-flat-files gate reports as unexpected.
- V10-014 lifecycle abort popup fixed: `kivo_lifecycle_test` now follows the real startup state-machine path and passes; full `ctest --preset vs-debug --output-on-failure --timeout 30` passes 74/74.
- Natural family tree created in `docs/p0/core-architecture/natural_family_tree.md`.
- First 12 P0 contract docs created.
- Second batch P0 docs created for subtitle, audio, inspector, and diagnostic manifest.
- CMake project entry created.
- C++20 contract headers created for the V1.1 required contract surface.
- ContractRegistry implementation created.
- P1 fake direct-play scenario created.
- Minimal C++ fake session test created.
- Playback invariant checker created.
- Diagnostic redaction gate created.
- Engine API P1 boundary created for command queue, event subscription, state snapshot store, and cancellation token.
- SessionStore and DecisionLedgerStore created for P1 in-memory query/export.
- ReplayManifest created and wired into fake Engine API scenario.
- PlaybackCore fake state machine created with invariant-backed transitions.
- GraphCore fake runtime created with edge capacity, lifecycle, backpressure, seek barrier, flush generation, and stop invariants.
- P1 media pipeline contract shells created for fake cache read, stream inventory, and decode path.
- Color/HDR/Presenter fake services created.
- Audio/Subtitle fake services created.
- Governance gate script created and passed.
- P2-001A/B/C completed: 5 design docs, 20 minimal slice headers, 10 placeholder headers, governance gate updated with P2 checks.

## Verification

- `backend/tools/governance/verify_p1_gates.py`: PASS (P0/P1/P2 governance gates).
- `cmake --preset ninja-debug`: PASS.
- `cmake --build --preset ninja-debug`: PASS (70 targets).
- `ctest --preset ninja-debug --output-on-failure`: PASS, 12/12 tests.
- `cmake --build --preset vs-debug`: PASS.
- `ctest --preset vs-debug --output-on-failure --timeout 30`: PASS, 74/74 tests.
- Latest `backend/tools/governance/verify_p1_gates.py`: blocked by root temporary outputs (build_output.txt, tmp/). V8-001 cleaned root: build_output.txt and tmp/ removed, workspace documented, evidence generated at artifacts/p2/evidence/V8-001.json.

## Next Work

Follow `docs/roadmap/backend_task_checklist.md`.

Immediate next task:

1. Open `docs/roadmap/p2_remaining_coverage_v8_execution_cards.md`.
2. Execute `V8-001` Branch / Workspace / Root Clean Gate.
3. Execute `V8-002` Current Checklist Consistency Gate and reconcile V10 DONE claims against V8 anti-fake rules.
4. Execute `LRD-001` Local Repair Delta Planning Draft before runtime code.
5. Audit D3D11 / WASAPI / PCM / local E2E / micro soak for real runtime proof before remote, cloud, IPTV, Cast, PiP, Cinema, Trakt, Foundation Final Gate, or All Providers Runtime Verified Gate work.
