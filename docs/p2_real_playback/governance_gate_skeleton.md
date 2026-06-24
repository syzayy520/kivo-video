# P2-001B Governance Gate Skeleton

Updated: 2026-06-25

## Goal

Extend the P1 governance gate script (`verify_p1_gates.py`) to cover P2 requirements without breaking existing P1 checks.

## New Gate Categories

### 1. Adapter Boundary Hardening

**Gate**: `check_adapter_boundary_hardening()`

Rules:
- No `#include <windows.h>` in core headers
- No `#include <d3d11.h>` / `<dxgi.h>` in core headers
- No `#include <audioclient.h>` / `<mmdeviceapi.h>` in core headers
- No `AVFormatContext` / `AVCodecContext` / `AVFrame` / `AVPacket` in core headers
- No `HWND` / `HANDLE` / `HRESULT` / `COM` types in core headers
- Adapter modules may include platform headers

### 2. Third-Party Dependency Lock

**Gate**: `check_dependency_lock()`

Rules:
- `vcpkg.json` exists and lists all third-party dependencies
- Each dependency has version, license, and purpose
- No dependency is added without lock file update

### 3. Claim Vocabulary Freeze

**Gate**: `check_claim_vocabulary()`

Rules:
- HDR/DV badge strings only in `color_science_core` and `compliance_core`
- Audio badge strings only in `audio_core` and `compliance_core`
- No badge string construction in UI or engine_api
- All badge strings must reference `ComplianceGateResult`

### 4. Schema Migration Policy

**Gate**: `check_schema_migration()`

Rules:
- All persistent structs have `schema_version`
- Schema version changes require migration path documentation
- No silent schema changes

### 5. Database Resilience

**Gate**: `check_database_resilience()`

Rules:
- Each SQLite domain has corruption recovery policy
- No single point of failure across domains
- Cache corruption must not affect library.db

### 6. Parser Security

**Gate**: `check_parser_security()`

Rules:
- FFmpeg probe/demux behind adapter boundary
- Malformed input must not crash core
- Fuzz target shells exist for subtitle/metadata parsers

### 7. Telemetry Hot Path Policy

**Gate**: `check_telemetry_hot_path()`

Rules:
- Telemetry ring buffer is bounded
- No locks on hot path (demux/decode/render)
- Diagnostic bundle redaction is fail-closed

## Implementation Plan

1. Add new check functions to `verify_p1_gates.py` (rename to `verify_gates.py`)
2. Each check function is independent and can fail fast
3. All checks run in `main()` and report PASS/FAIL
4. No changes to existing P1 checks

## Verification

P2-001B is verified when:
1. `python backend/tools/governance/verify_gates.py` passes
2. All existing P1 checks still pass
3. New P2 checks catch intentional violations in test
