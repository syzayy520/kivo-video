# Current State

Updated: 2026-06-24

## Done

- Local Codex memory created for the Kivo Video iron gate.
- Backend goal persisted in `.codex/memory/backend-goal.md`.
- Detailed execution checklist created at `docs/roadmap/backend_task_checklist.md`.
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

## Verification

- `backend/tools/governance/verify_p1_gates.py`: PASS.
- `cmake --preset ninja-debug`: PASS through Visual Studio Developer Command Prompt.
- `cmake --build --preset ninja-debug`: PASS through Visual Studio Developer Command Prompt.
- `ctest --preset ninja-debug --output-on-failure`: PASS, 9/9 tests after Audio/Subtitle fake services.

## Next Work

Follow `docs/roadmap/backend_task_checklist.md`.

Immediate next task:

1. Add PlaybackInspectorCore fake inspector service.
2. Add DiagnosticCore fake bundle manifest export service.
3. Add `kivo_inspector_diagnostic_test`.
4. Extend `verify_p1_gates.py`.
5. Run governance, build, and CTest verification.
