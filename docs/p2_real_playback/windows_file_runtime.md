# P2-010 Windows File Identity Runtime

Updated: 2026-06-25

## Goal

Implement stable local file identity snapshot using Windows file system APIs.

## File Identity Components

| Component | Source | Stability |
|-----------|--------|-----------|
| file_path | GetFinalPathNameByHandle | Renamed = new identity |
| volume_serial | GetVolumeInformation | Stable per volume |
| file_index | GetFileInformationByHandle | Stable per file |
| file_size | GetFileSizeEx | Changes on write |
| last_modified | GetFileTime | Changes on write |

## Stability Rules

1. Identity is stable when volume_serial + file_index match
2. Path change alone does not break identity (file can be moved)
3. File rename creates new path but same identity
4. File delete + recreate creates new identity

## Implementation Plan

1. Create `FakeFileIdentityService` that produces LocalFileIdentity
2. Tests covering identity construction, stability, and field access
3. Design doc
