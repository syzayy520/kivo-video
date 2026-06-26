# P2 Remaining Coverage V8 Execution Cards

Updated: 2026-06-25

Canonical roadmap name: `P2-REMAINING-COVERAGE-V8`.
Next executable task: `P2-LOCAL-REPAIR-DELTA-001`.

This file converts the V8 roadmap into task cards an assistant can execute directly. Do not treat this file as proof by itself. Each card must produce evidence, update `docs/roadmap/backend_task_checklist.md`, and keep `docs/roadmap/current_state.md` honest.

## Global Execution Rules

- Follow the iron gate: conception -> planning design -> natural family tree -> implementation -> single file single responsibility -> no flat files.
- Do not start code for a card until its planning draft exists.
- Do not report `P2 FOUNDATION COMPLETE` until `PFFG-001` and `PFFG-002` pass.
- Do not report `APRV-001 final verification` until `APRV-001` passes.
- Do not report `RUNTIME_PASS` from stub, fake, simulator, state-only, log-only, or mock-only proof.
- Use only these evidence statuses: `RUNTIME_PASS`, `CONTRACT_PASS`, `FAIL`, `BLOCKED_ENV`, `NOT_IMPLEMENTED`, `SKIPPED_TEST_ONLY`.
- Put evidence under `artifacts/p2/evidence/<task-id>.json` or document why the artifact path is blocked.
- Keep generated outputs out of the repository root.
- Update the roadmap checklist after each completed card.

## Direct Execution Order

1. `V8-001`
2. `V8-002`
3. `V8-003`
4. `V8-004`
5. `V8-005`
6. `V8-006`
7. `V8-007`
8. `V8-008`
9. `V8-009`
10. `LRD-001` through `LRD-013`
11. `SGF-001` through `SGF-008`
12. `RGF-001` through `RGF-005`
13. Foundation family cards in dependency order: `FMT`, `MID`, `SUB`, `SRC`, `NET`, `OBJ`, `WDA`, `MSV`, `CLD`, `IPTV`, `PST`, `ACP`, `CAST`, `PIP`, `CIN`, `TRK`
14. `PMX-001`, `PMX-002`
15. `PFFG-001`, `PFFG-002`
16. `APRV-001` only after all provider runtime gaps are resolved.

## Card Template

Each task card must be executed with these fields:

- Task ID
- Task Family
- Depends On
- Goal
- Non-goals
- Already Done / Do Not Repeat
- Allowed Files
- Forbidden Files
- Runtime Proof Required
- Contract Proof Allowed
- Provider Matrix Impact
- Evidence Output
- Inspector Output
- DecisionLedger Output
- Schema Gate
- Redline Gate
- Anti-fake Gate
- Clean Workspace Gate
- Final Report Rules

## V8 Global Gate Cards

### V8-001 Branch / Workspace / Root Clean Gate

- Task Family: governance.
- Depends On: none.
- Goal: prove current branch, HEAD, remote, workspace, and root file hygiene before any V8 implementation.
- Non-goals: do not delete user files without explicit approval.
- Already Done / Do Not Repeat: do not rerun old V10 implementation work.
- Allowed Files: `docs/roadmap/current_state.md`, `docs/roadmap/backend_task_checklist.md`, planned evidence files.
- Forbidden Files: random root outputs such as `build_output.txt`, `build_err.txt`, pasted shell fragments, or generated executables.
- Runtime Proof Required: `git status --short`, branch name, HEAD, remote, root inventory, governance script result.
- Contract Proof Allowed: no.
- Provider Matrix Impact: none.
- Evidence Output: `artifacts/p2/evidence/V8-001.json`.
- Inspector Output: not required.
- DecisionLedger Output: governance decision entry required if ledger foundation exists.
- Schema Gate: evidence must use V8 status vocabulary.
- Redline Gate: no false PASS if root is dirty.
- Anti-fake Gate: no ignoring root temporary files.
- Clean Workspace Gate: required.
- Final Report Rules: if dirty, classify as `FAIL` or blocked root hygiene; do not continue implementation.

### V8-002 Current Checklist Consistency Gate

- Task Family: governance.
- Depends On: `V8-001`.
- Goal: reconcile V10 DONE claims, V10 remaining TODO subsections, V10 final report language, and V8 anti-fake rules.
- Non-goals: do not rewrite runtime code.
- Already Done / Do Not Repeat: keep V10 history; do not erase prior evidence.
- Allowed Files: roadmap docs, current state, audit evidence.
- Forbidden Files: production implementation files.
- Runtime Proof Required: line-level audit of V10 claims mentioning stub, simulator, placeholder PCM, state-only, or incomplete artifacts.
- Contract Proof Allowed: yes, for audit-only conclusions.
- Provider Matrix Impact: none.
- Evidence Output: `artifacts/p2/evidence/V8-002.json`.
- Inspector Output: not required.
- DecisionLedger Output: record downgrade/repair decisions if ledger foundation exists.
- Schema Gate: every audited item must classify as `RUNTIME_PASS`, `CONTRACT_PASS`, `FAIL`, `BLOCKED_ENV`, or `NEEDS_REPAIR` inside the audit; final V8 evidence status cannot use `NEEDS_REPAIR`.
- Redline Gate: report/source conflicts are failures.
- Anti-fake Gate: stub/simulator cannot remain runtime proof.
- Clean Workspace Gate: required.
- Final Report Rules: state exact claim allowed after audit.

### V8-003 No Duplicate Work Gate

- Task Family: governance.
- Depends On: `V8-002`.
- Goal: document work that must not be repeated and what may only be regressed, audited, or repaired.
- Non-goals: no new runtime features.
- Already Done / Do Not Repeat: FFmpeg probe/demux/decode skeleton, presenter abort repair, D3D11 unavailable skip semantics, `wasapi_internal.hpp`, `d3d11_internal.hpp`, FfmpegFrameResult frame data / linesize / plane count.
- Allowed Files: roadmap docs and evidence.
- Forbidden Files: source rewrites for already accepted skeletons unless a later repair card proves a defect.
- Runtime Proof Required: source/file inventory and test references.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: none.
- Evidence Output: `artifacts/p2/evidence/V8-003.json`.
- Inspector Output: not required.
- DecisionLedger Output: optional governance entry.
- Schema Gate: evidence lists duplicate-work protections.
- Redline Gate: no task may claim duplicate skeleton work as new runtime proof.
- Anti-fake Gate: no reheated old PASS.
- Clean Workspace Gate: required.
- Final Report Rules: list protected work and allowed repair scope.

### V8-004 Natural Family Tree Gate

- Task Family: planning.
- Depends On: `V8-003`.
- Goal: map all P2 remaining families before implementation.
- Non-goals: no source code changes.
- Already Done / Do Not Repeat: reuse existing natural family tree style.
- Allowed Files: `docs/roadmap/*`, `docs/p0/*`.
- Forbidden Files: backend implementation files.
- Runtime Proof Required: not required.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: provider families must be mapped.
- Evidence Output: `artifacts/p2/evidence/V8-004.json`.
- Inspector Output: not required.
- DecisionLedger Output: not required.
- Schema Gate: family names must match task IDs.
- Redline Gate: no `common`, `misc`, or utility dumping-ground families.
- Anti-fake Gate: family tree cannot mark runtime PASS.
- Clean Workspace Gate: required.
- Final Report Rules: publish tree path and dependency direction.

### V8-005 Allowed / Forbidden Claims Gate

- Task Family: governance.
- Depends On: `V8-004`.
- Goal: enforce claim vocabulary before new evidence is generated.
- Non-goals: do not certify providers.
- Already Done / Do Not Repeat: keep V10 local closure as child history, not full P2.
- Allowed Files: roadmap docs, redline config, evidence.
- Forbidden Files: unrelated runtime files.
- Runtime Proof Required: scan docs/evidence for forbidden claims.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: provider runtime claims are matrix-owned.
- Evidence Output: `artifacts/p2/evidence/V8-005.json`.
- Inspector Output: not required.
- DecisionLedger Output: optional.
- Schema Gate: claim status must map to V8 status vocabulary.
- Redline Gate: fail on premature `P2 readiness`, `APRV-001 final verification`, `P3 capability`, `HDR capability`, `Atmos bitstream capability`, or `AirPlay capability`.
- Anti-fake Gate: no single test can close P2.
- Clean Workspace Gate: required.
- Final Report Rules: allowed claim after this gate is only governance readiness for V8, not P2 completion.

### V8-006 Status Vocabulary Gate

- Task Family: schema foundation.
- Depends On: `V8-005`.
- Goal: enforce the six V8 evidence statuses.
- Non-goals: no provider implementation.
- Already Done / Do Not Repeat: do not reuse ad hoc statuses as final statuses.
- Allowed Files: schema/gate docs and evidence.
- Forbidden Files: unrelated code.
- Runtime Proof Required: local validation command or scripted scan.
- Contract Proof Allowed: yes until schema implementation.
- Provider Matrix Impact: provider matrix uses `RUNTIME_PASS`, `BLOCKED_ENV`, `FAIL`, `NOT_IMPLEMENTED` for runtime status.
- Evidence Output: `artifacts/p2/evidence/V8-006.json`.
- Inspector Output: not required.
- DecisionLedger Output: not required.
- Schema Gate: pass/fail must be machine checked.
- Redline Gate: unknown statuses fail.
- Anti-fake Gate: `SKIPPED_TEST_ONLY` cannot become PASS.
- Clean Workspace Gate: required.
- Final Report Rules: state vocabulary and scan result.

### V8-007 Evidence Path Gate

- Task Family: evidence foundation.
- Depends On: `V8-006`.
- Goal: define artifact paths and prevent source-root evidence dumping.
- Non-goals: no runtime feature implementation.
- Already Done / Do Not Repeat: do not use root temporary outputs.
- Allowed Files: `artifacts/p2/evidence/**`, roadmap docs, schema docs.
- Forbidden Files: evidence JSON in source root unless explicitly blocked and documented.
- Runtime Proof Required: path creation/validation command.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: all provider evidence paths must be stable.
- Evidence Output: `artifacts/p2/evidence/V8-007.json`.
- Inspector Output: path only.
- DecisionLedger Output: path only.
- Schema Gate: evidence path required.
- Redline Gate: generated root files fail.
- Anti-fake Gate: missing evidence cannot be narrated as PASS.
- Clean Workspace Gate: required or explicitly documented.
- Final Report Rules: include artifact root.

### V8-008 Provider Runtime Matrix Gate Plan

- Task Family: provider matrix.
- Depends On: `V8-007`.
- Goal: define provider matrix schema, provider list, and honest runtime semantics before provider tasks.
- Non-goals: no provider runtime verification yet.
- Already Done / Do Not Repeat: do not mark all providers PASS by default.
- Allowed Files: roadmap docs, schema docs, evidence.
- Forbidden Files: provider runtime implementation.
- Runtime Proof Required: not yet.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: creates matrix plan.
- Evidence Output: `artifacts/p2/evidence/V8-008.json`.
- Inspector Output: not required.
- DecisionLedger Output: not required.
- Schema Gate: planned fields must include provider kind, contract status, runtime status, environment kind, environment availability, blocked reason.
- Redline Gate: missing environment cannot be `RUNTIME_PASS`.
- Anti-fake Gate: controlled protocol server cannot be mislabeled as cloud account.
- Clean Workspace Gate: required.
- Final Report Rules: matrix is planned, not complete.

### V8-009 Final Gate Sequencing Gate

- Task Family: governance.
- Depends On: `V8-008`.
- Goal: lock final gates until all dependencies are complete.
- Non-goals: do not run final gate.
- Already Done / Do Not Repeat: V10 local child history is not P2 Foundation Final Gate.
- Allowed Files: roadmap docs and evidence.
- Forbidden Files: runtime code.
- Runtime Proof Required: dependency list and gating rule.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: final gate must read provider matrix later.
- Evidence Output: `artifacts/p2/evidence/V8-009.json`.
- Inspector Output: not required.
- DecisionLedger Output: optional.
- Schema Gate: final gate evidence must be absent or `NOT_IMPLEMENTED` until dependencies exist.
- Redline Gate: fail premature final claims.
- Anti-fake Gate: no narrative-only final gate.
- Clean Workspace Gate: required.
- Final Report Rules: next allowed card is `LRD-001`.

## P2-LOCAL-REPAIR-DELTA-001 Cards

### LRD-001 Planning Draft

- Task Family: local repair delta.
- Depends On: `V8-009`.
- Goal: write the planning draft for local repair before code.
- Non-goals: no runtime code changes.
- Already Done / Do Not Repeat: do not recreate FFmpeg skeletons or previous V10 docs.
- Allowed Files: `docs/roadmap/p2_local_repair_delta_001_planning.md`, roadmap/current state, evidence.
- Forbidden Files: backend implementation and tests.
- Runtime Proof Required: tree inventory, V10 evidence audit plan, allowed/forbidden files, dependency direction, single-file responsibility table, tests, runtime risks, anti-fake plan, schema plan, redline plan.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: local_file entry plan only.
- Evidence Output: `artifacts/p2/evidence/LRD-001.json`.
- Inspector Output: planned.
- DecisionLedger Output: planned.
- Schema Gate: plan must name evidence schema.
- Redline Gate: plan must identify stub/simulator risk.
- Anti-fake Gate: plan must forbid fake runtime proof.
- Clean Workspace Gate: required.
- Final Report Rules: no runtime claim yet.

### LRD-002 V10 Evidence Downgrade / Repair Audit

- Task Family: local repair delta.
- Depends On: `LRD-001`.
- Goal: classify V10-007 through V10-028 against V8 rules.
- Non-goals: no code repair yet.
- Allowed Files: audit doc, checklist, current state, evidence.
- Forbidden Files: runtime source.
- Runtime Proof Required: inspect source/tests/evidence text, not only checklist.
- Contract Proof Allowed: yes.
- Provider Matrix Impact: local_file preliminary status.
- Evidence Output: `artifacts/p2/evidence/LRD-002.json`.
- Inspector Output: audit references.
- DecisionLedger Output: downgrade decisions.
- Schema Gate: each V10 item gets a status and reason.
- Redline Gate: stub/simulator/runtime conflict fails audit.
- Anti-fake Gate: no silent upgrade.
- Clean Workspace Gate: required.
- Final Report Rules: identify repair cards that remain.

### LRD-003 D3D11 Device Reality Audit

- Task Family: local repair delta/video.
- Depends On: `LRD-002`.
- Goal: prove real D3D11 device/context or honest environment block.
- Non-goals: no hardware decode, HDR, tone mapping, or P3 rendering.
- Allowed Files: `backend/include/kivo/cinema_engine/video_upload/**`, `backend/src/video_upload/**`, `backend/tests/video_upload/**`, evidence.
- Forbidden Files: UI, HDR output, hardware decode, remote providers.
- Runtime Proof Required: real D3D11 device/context creation when environment supports it; WARP/hardware classification.
- Contract Proof Allowed: only if environment unavailable and evidence says so.
- Provider Matrix Impact: local_file video boundary.
- Evidence Output: `artifacts/p2/evidence/LRD-003.json`.
- Inspector Output: D3D11 capability snapshot.
- DecisionLedger Output: device selection decision.
- Schema Gate: environment kind required.
- Redline Gate: no fake device PASS.
- Anti-fake Gate: no dummy texture.
- Clean Workspace Gate: required.
- Final Report Rules: classify as runtime, contract, blocked, or fail.

### LRD-004 D3D11 Real Decoded Frame Upload

- Task Family: local repair delta/video.
- Depends On: `LRD-003`.
- Goal: upload real decoded frame data into a D3D11 texture boundary.
- Non-goals: no hardware decode or zero-copy renderer.
- Allowed Files: video upload family, decode integration tests, evidence.
- Forbidden Files: unrelated presenter rewrites or UI.
- Runtime Proof Required: frame planes, linesize, dimensions, pixel format, upload result, texture metadata.
- Contract Proof Allowed: only when D3D11 unavailable, with blocked/contract status.
- Provider Matrix Impact: local_file runtime proof.
- Evidence Output: `artifacts/p2/evidence/LRD-004.json`.
- Inspector Output: decoded frame upload snapshot.
- DecisionLedger Output: upload decision.
- Schema Gate: no bool-only upload proof.
- Redline Gate: no dummy/empty frame PASS.
- Anti-fake Gate: upload must use decoded data.
- Clean Workspace Gate: required.
- Final Report Rules: cannot claim real first frame if upload is stubbed.

### LRD-005 Presenter Boundary Reality Audit

- Task Family: local repair delta/presenter.
- Depends On: `LRD-004`.
- Goal: prove presenter boundary receives the uploaded texture.
- Non-goals: no final renderer polish.
- Allowed Files: presenter family, video upload tests, evidence.
- Forbidden Files: UI, HDR/tone mapping, P3 renderer.
- Runtime Proof Required: boundary receives texture identity, dimensions, timestamp, source generation.
- Contract Proof Allowed: if graphics device unavailable.
- Provider Matrix Impact: local_file runtime proof.
- Evidence Output: `artifacts/p2/evidence/LRD-005.json`.
- Inspector Output: presenter snapshot.
- DecisionLedger Output: presenter first-frame decision.
- Schema Gate: texture identity required.
- Redline Gate: unavailable device cannot become PASS.
- Anti-fake Gate: no log-only first frame.
- Clean Workspace Gate: required.
- Final Report Rules: state exactly what presenter proof reached.

### LRD-006 PCM Converter Reality Audit

- Task Family: local repair delta/audio.
- Depends On: `LRD-002`.
- Goal: prove PCM bytes derive from real decoded audio frame data.
- Non-goals: no passthrough, bitstream, WASAPI exclusive.
- Allowed Files: audio output family, decode integration tests, evidence.
- Forbidden Files: exclusive audio, AVR/HDMI, passthrough.
- Runtime Proof Required: source audio frame format, sample rate, channels, sample count, converted PCM bytes.
- Contract Proof Allowed: only if sample lacks audio.
- Provider Matrix Impact: local_file audio proof.
- Evidence Output: `artifacts/p2/evidence/LRD-006.json`.
- Inspector Output: audio conversion snapshot.
- DecisionLedger Output: PCM conversion decision.
- Schema Gate: real source frame reference required.
- Redline Gate: no generated silence PASS.
- Anti-fake Gate: placeholder PCM is not runtime proof.
- Clean Workspace Gate: required.
- Final Report Rules: distinguish converter contract from runtime conversion.

### LRD-007 WASAPI Shared Runtime Audit

- Task Family: local repair delta/audio.
- Depends On: `LRD-006`.
- Goal: prove shared-mode WASAPI render client boundary.
- Non-goals: no exclusive mode or bitstream.
- Allowed Files: audio output family, Windows audio tests, evidence.
- Forbidden Files: passthrough/exclusive implementation.
- Runtime Proof Required: COM init, endpoint, shared format, `IAudioClient`, `IAudioRenderClient`, `GetBuffer`, `ReleaseBuffer`, `Start`, `Stop`.
- Contract Proof Allowed: if audio endpoint unavailable.
- Provider Matrix Impact: local_file audio output.
- Evidence Output: `artifacts/p2/evidence/LRD-007.json`.
- Inspector Output: WASAPI snapshot.
- DecisionLedger Output: audio route decision.
- Schema Gate: endpoint/environment fields required.
- Redline Gate: no fake WASAPI PASS.
- Anti-fake Gate: no silence-buffer-only proof.
- Clean Workspace Gate: required.
- Final Report Rules: report endpoint availability honestly.

### LRD-008 Local E2E Runtime Audit

- Task Family: local repair delta/e2e.
- Depends On: `LRD-004`, `LRD-005`, `LRD-007`.
- Goal: prove local E2E touches the real runtime chain.
- Non-goals: no remote provider or UI workflow.
- Allowed Files: local e2e tests, runtime orchestration family, evidence.
- Forbidden Files: remote/cloud/IPTV/provider foundation.
- Runtime Proof Required: source -> byte stream -> FFmpeg probe -> demux -> decode -> upload/presenter -> PCM/WASAPI -> clock -> seek -> lifecycle.
- Contract Proof Allowed: no for internal local foundation unless environment is honestly blocked.
- Provider Matrix Impact: local_file entry.
- Evidence Output: `artifacts/p2/evidence/LRD-008.json`.
- Inspector Output: E2E snapshot.
- DecisionLedger Output: E2E decisions.
- Schema Gate: runtime chain fields required.
- Redline Gate: state-machine-only playback fails.
- Anti-fake Gate: no fake local playback.
- Clean Workspace Gate: required.
- Final Report Rules: do not close local repair if any required boundary is contract-only.

### LRD-009 Micro Soak Runtime Audit

- Task Family: local repair delta/soak.
- Depends On: `LRD-008`.
- Goal: prove 30s continuous local runtime soak.
- Non-goals: no long flagship soak or 4K gate.
- Allowed Files: soak tests, metrics evidence.
- Forbidden Files: root generated outputs, remote providers.
- Runtime Proof Required: duration, frames/audio buffers, queue high-water, memory high-water, no repeated open/close.
- Contract Proof Allowed: only if sample duration missing, with blocked status.
- Provider Matrix Impact: local_file reliability proof.
- Evidence Output: `artifacts/p2/evidence/LRD-009.json`.
- Inspector Output: high-water snapshot.
- DecisionLedger Output: soak start/end decision.
- Schema Gate: metrics required.
- Redline Gate: `SoakSimulator` cannot be runtime proof.
- Anti-fake Gate: no repeated open/close fake soak.
- Clean Workspace Gate: required.
- Final Report Rules: report actual duration and sample identity redacted.

### LRD-010 Evidence Artifact Audit

- Task Family: local repair delta/evidence.
- Depends On: `LRD-008`, `LRD-009`.
- Goal: prove required artifacts exist and are generated in artifact directories.
- Non-goals: no runtime feature repair.
- Allowed Files: `artifacts/p2/evidence/**`, schema validation, roadmap docs.
- Forbidden Files: source root evidence outputs.
- Runtime Proof Required: artifact listing and schema validation.
- Contract Proof Allowed: no for artifact existence.
- Provider Matrix Impact: local_file evidence references.
- Evidence Output: `artifacts/p2/evidence/LRD-010.json`.
- Inspector Output: `inspector_snapshot.json`.
- DecisionLedger Output: `decision_ledger.json`.
- Schema Gate: all required artifacts validate.
- Redline Gate: no fake/status conflict.
- Anti-fake Gate: artifacts must reference real tests.
- Clean Workspace Gate: required.
- Final Report Rules: list artifact paths.

### LRD-011 Final Report Consistency Audit

- Task Family: local repair delta/report.
- Depends On: `LRD-010`.
- Goal: ensure report, source, tests, and evidence agree.
- Non-goals: no code repair unless a conflict is found.
- Allowed Files: final report, checklist, current state, evidence.
- Forbidden Files: unrelated source.
- Runtime Proof Required: compare final report claims to evidence.
- Contract Proof Allowed: yes for audit.
- Provider Matrix Impact: local_file claim.
- Evidence Output: `artifacts/p2/evidence/LRD-011.json`.
- Inspector Output: referenced.
- DecisionLedger Output: referenced.
- Schema Gate: report/evidence conflict fails.
- Redline Gate: false runtime claims fail.
- Anti-fake Gate: stub/contract not called real runtime.
- Clean Workspace Gate: required.
- Final Report Rules: exact claim vocabulary only.

### LRD-012 Regression Protection

- Task Family: local repair delta/regression.
- Depends On: `LRD-011`.
- Goal: prove accepted baseline does not regress.
- Non-goals: no new feature scope.
- Allowed Files: tests, evidence, checklist.
- Forbidden Files: remote/provider foundation.
- Runtime Proof Required: P2-REAL-001 regression, presenter abort repair, D3D11 unavailable skip semantics, FFmpeg ON/OFF behavior.
- Contract Proof Allowed: no for baseline tests unless environment blocked.
- Provider Matrix Impact: local_file confidence.
- Evidence Output: `artifacts/p2/evidence/LRD-012.json`.
- Inspector Output: referenced.
- DecisionLedger Output: referenced.
- Schema Gate: test commands captured.
- Redline Gate: skipped test cannot count as PASS unless explicitly skip-only.
- Anti-fake Gate: no hiding failures.
- Clean Workspace Gate: required.
- Final Report Rules: list exact commands and counts.

### LRD-013 Classification

- Task Family: local repair delta/classification.
- Depends On: `LRD-012`.
- Goal: classify `P2-LOCAL-REPAIR-DELTA-001`.
- Non-goals: do not claim full P2 Foundation.
- Allowed Files: checklist, current state, evidence, final local repair report.
- Forbidden Files: runtime changes.
- Runtime Proof Required: all required local runtime boundaries pass.
- Contract Proof Allowed: only if classification is not `RUNTIME_PASS`.
- Provider Matrix Impact: local_file entry updated or planned.
- Evidence Output: `artifacts/p2/evidence/LRD-013.json`.
- Inspector Output: final local snapshot reference.
- DecisionLedger Output: final local ledger reference.
- Schema Gate: classification evidence validates.
- Redline Gate: no false final claims.
- Anti-fake Gate: no partial PASS as full closure.
- Clean Workspace Gate: required.
- Final Report Rules: allowed outcomes are `RUNTIME_PASS`, `CONTRACT_PASS`, `FAIL`, or `BLOCKED_ENV` with explicit reason.

## Queued Foundation Cards

The following cards are queued after `LRD-013`. Each must be expanded into the same template before implementation if the task is still broad.

- `SGF-001` through `SGF-008`: Schema Gate Foundation.
- `RGF-001` through `RGF-005`: Redline Gate Foundation.
- `FMT-001`: Format Compatibility.
- `MID-001`: Media Identity.
- `SUB-001`: Subtitle and Chapter.
- `SRC-001`: Source Core.
- `NET-001`: Network File Sources.
- `OBJ-001`: Object Storage.
- `WDA-001`: WebDAV and AList.
- `MSV-001`: Media Server.
- `CLD-001`: Cloud Drive.
- `IPTV-001`: IPTV Live.
- `PST-001`: Playback State.
- `ACP-001`: Audio Codec Policy.
- `CAST-001`: Cast Foundation.
- `PIP-001`: PiP Foundation.
- `CIN-001`: Cinema Foundation.
- `TRK-001`: Trakt Foundation.
- `PMX-001`: Provider Runtime Matrix Generation.
- `PMX-002`: Matrix Honesty Gate.
- `PFFG-001`: Foundation Final Gate Reader.
- `PFFG-002`: Foundation Final Report.
- `APRV-001`: All Providers Runtime Verification Gate.
