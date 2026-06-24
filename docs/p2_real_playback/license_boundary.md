# P2-003 License Boundary

Updated: 2026-06-25

## Goal

Define license boundaries to ensure third-party dependency licenses do not contaminate the project's license model.

## License Categories

### Permissive Licenses (OK)

| License | SPDX ID | Dependencies |
|---------|---------|--------------|
| MIT License | `MIT` | — |
| Apache License 2.0 | `Apache-2.0` | — |
| Boost Software License 1.0 | `BSL-1.0` | Catch2 |
| BSD 2-Clause | `BSD-2-Clause` | — |
| BSD 3-Clause | `BSD-3-Clause` | — |
| ISC License | `ISC` | — |
| zlib License | `Zlib` | — |

### Weak Copyleft (OK with conditions)

| License | SPDX ID | Conditions |
|---------|---------|------------|
| GNU LGPL 2.1 | `LGPL-2.1` | Dynamic linking only, no static linking |
| GNU LGPL 3.0 | `LGPL-3.0` | Dynamic linking only, no static linking |

### Strong Copyleft (BLOCKED)

| License | SPDX ID | Reason |
|---------|---------|--------|
| GNU GPL 2.0 | `GPL-2.0` | Requires derivative works to be GPL |
| GNU GPL 3.0 | `GPL-3.0` | Requires derivative works to be GPL |
| GNU AGPL 3.0 | `AGPL-3.0` | Network copyleft |

## License Boundary Rules

1. **No GPL in core**: Core modules (`include/kivo/cinema_engine/`, `src/`) must not link against GPL-licensed code.
2. **LGPL dynamic linking only**: LGPL dependencies (e.g., FFmpeg) must be dynamically linked.
3. **No license contamination**: Adapter modules must not introduce GPL dependencies into the core boundary.
4. **License documentation**: Each dependency's license must be documented in `vcpkg.json`.

## Governance Gate

The `check_license_boundary()` gate verifies:

1. All licenses in `vcpkg.json` are valid SPDX identifiers
2. No GPL/AGPL licenses are declared
3. LGPL dependencies are documented with dynamic linking requirement
4. License compatibility with project license

## Verification

P2-003 is verified when:
1. `vcpkg.json` has valid license declarations
2. No GPL/AGPL dependencies exist
3. `python backend/tools/governance/verify_p1_gates.py` passes
