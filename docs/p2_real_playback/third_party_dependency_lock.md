# P2-003 Third Party Dependency Lock

Updated: 2026-06-25

## Goal

Lock all third-party dependencies with explicit version, license, and purpose declarations. No dependency may be added without updating the lock file and passing governance checks.

## Dependency Declaration

All third-party dependencies must be declared in `vcpkg.json` at the project root. Each dependency entry must include:

```json
{
  "name": "ffmpeg",
  "version": "7.1",
  "license": "LGPL-2.1",
  "purpose": "Video/audio demuxing, decoding, and format detection"
}
```

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Package name in vcpkg registry |
| `version` | string | Exact version (no ranges) |
| `license` | string | SPDX license identifier |
| `purpose` | string | Why this dependency is needed |

## Allowed Dependencies (P2.1 Local Playback)

| Dependency | Version | License | Purpose |
|------------|---------|---------|---------|
| `ffmpeg` | 7.1 | LGPL-2.1 | Video/audio demuxing, decoding, format detection |
| `catch2` | 3.5.2 | BSL-1.0 | Unit testing framework |

## Dependency Lock Rules

1. **No wildcard versions**: All versions must be exact (e.g., `7.1`, not `^7.0`).
2. **No transitive dependencies without declaration**: If a dependency pulls in sub-dependencies, those must be documented.
3. **License compatibility**: All licenses must be compatible with the project's license model.
4. **Security audit**: Dependencies with known CVEs at the declared version are blocked.
5. **Purpose justification**: Each dependency must have a clear purpose statement.

## Governance Gate

The `check_dependency_lock()` gate verifies:

1. `vcpkg.json` exists at project root
2. Each dependency has all required fields
3. No dependency is added without lock file update
4. License identifiers are valid SPDX

## Verification

P2-003 is verified when:
1. `vcpkg.json` exists with all required fields
2. `python backend/tools/governance/verify_p1_gates.py` passes
3. No undeclared dependencies in CMakeLists.txt
