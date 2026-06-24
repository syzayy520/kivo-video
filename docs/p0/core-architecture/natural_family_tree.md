# Natural Family Tree

The backend project is organized by responsibility families, not by a flat technical pile.

## Root

- `AGENTS.md`: local Codex rule entry.
- `README.md`: project entry.
- `CMakeLists.txt`: project build entry.
- `.codex/`: local memory.
- `docs/`: P0/P1 architecture and contracts.
- `backend/`: C++ backend core, tests, and governance tools.

## Backend Families

- `backend/include/kivo/cinema_engine/foundation`: schema metadata, contract registry, shared lifecycle contracts.
- `backend/include/kivo/cinema_engine/engine_api`: command, event, snapshot boundary for UI and callers.
- `backend/include/kivo/cinema_engine/session_core`: session identity and aggregate state.
- `backend/include/kivo/cinema_engine/decision_ledger_core`: replay-critical decision chain.
- `backend/include/kivo/cinema_engine/playback_core`: playback state machine and generation contracts.
- `backend/include/kivo/cinema_engine/playback_plan_core`: selected source, track, fallback, and recovery plan.
- `backend/include/kivo/cinema_engine/source_core`: source playable object contract.
- `backend/include/kivo/cinema_engine/remote_object_core`: stable remote object identity.
- `backend/include/kivo/cinema_engine/capability_core`: layered capability join.
- `backend/include/kivo/cinema_engine/policy_core`: policy decisions.
- `backend/include/kivo/cinema_engine/compliance_core`: claim and fail-closed decisions.
- `backend/include/kivo/cinema_engine/feature_gate_core`: snapshotized feature gates.
- `backend/include/kivo/cinema_engine/media_experience_core`: user-visible media experience and badges.
- `backend/include/kivo/cinema_engine/color_science_core`: HDR and DV-aware output matrix.
- `backend/include/kivo/cinema_engine/windows_hdr_core`: Windows HDR output contract.
- `backend/include/kivo/cinema_engine/video_render_core`: presenter and frame pacing contract.
- `backend/include/kivo/cinema_engine/subtitle_core`: subtitle rendering plan.
- `backend/include/kivo/cinema_engine/audio_core`: audio output decision.
- `backend/include/kivo/cinema_engine/playback_inspector_core`: user and developer inspection snapshot.
- `backend/include/kivo/cinema_engine/diagnostic_core`: diagnostic bundle and redaction boundary.
- `backend/include/kivo/cinema_engine/graph_core`: graph runtime execution state.
- `backend/include/kivo/cinema_engine/network_cache_core`: cache extent identity boundary.
- `backend/src`: implementation by the same family names.
- `backend/tests`: family-owned tests.
- `backend/tools/governance`: CI-style gates that prove the freeze rules.

