# P2-040 HDR / Color Output Runtime

## Purpose

Model the HDR and color output runtime: detecting HDR10/HLG metadata from media sources, providing color space information for rendering, and ensuring no false Dolby Vision claims are made.

## Scope

- HDR10 metadata detection and reporting
- HLG metadata detection and reporting
- Color space descriptor for rendering
- Dolby Vision detection (with strict validation to prevent false claims)

## Family Tree (color_science_core/)

```
color_science_core/
├── hdr_detection_result.hpp      — Result of HDR metadata detection
├── color_space_descriptor.hpp    — Color space info for rendering
├── fake_hdr_color_service.hpp    — Fake service for testing
```

## Contract Types

### HdrDetectionResult

| Field | Type | Description |
|-------|------|-------------|
| detection_id | string | Unique detection identifier |
| source_id | string | Media source identifier |
| is_hdr10 | bool | Whether HDR10 metadata is present |
| is_hlg | bool | Whether HLG metadata is present |
| is_dolby_vision | bool | Whether DV metadata is present |
| hdr10_max_luminance | int64_t | HDR10 max luminance (nits) |
| hdr10_min_luminance | int64_t | HDR10 min luminance (nits) |
| color_primaries | string | Color primaries (bt709, bt2020, etc.) |

### ColorSpaceDescriptor

| Field | Type | Description |
|-------|------|-------------|
| descriptor_id | string | Unique descriptor identifier |
| color_space | string | Color space name |
| transfer_function | string | Transfer function (pq, hlg, srgb) |
| primaries | string | Color primaries |
| is_hdr | bool | Whether this is an HDR color space |
| max_cll | int64 Max Content Light Level (nits) |
| max_fall | int64 Max Frame Average Light Level (nits) |

### HdrClaimValidation

| Field | Type | Description |
|-------|------|-------------|
| validation_id | string | Unique validation identifier |
| claim_type | string | Claim type (dolby_vision, hdr10, hlg) |
| is_valid | bool | Whether the claim is valid |
| evidence_source | string | Source of validation evidence |
| failure_reason | string | Reason if claim is invalid |

## Tests

10 tests in `backend/tests/color_science_core/hdr_color_runtime_test.cpp`:
1. HDR10 detection result
2. HLG detection result
3. Dolby Vision detection result
4. Color space descriptor for SDR
5. Color space descriptor for HDR10
6. Dolby Vision claim validation (valid)
7. Dolby Vision claim validation (false claim)
8. HDR10 luminance tracking
9. Color primaries tracking
10. Transfer function tracking
