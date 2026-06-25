# P2-026 BDMV / Mounted ISO Main Title Playback Gate

Updated: 2026-06-25

## Goal

Implement playback gate for BDMV main title playback, including m2ts sequence playback and encrypted disc fail-closed gate.

## Scope

This task creates contracts and fake services for gating BDMV main title playback.

### Playback Gate Scenarios

1. **Valid BDMV Folder** - BDMV folder with valid main title playlist
2. **Mounted ISO** - Mounted ISO with BDMV structure
3. **Encrypted Disc** - Encrypted disc that should fail-closed
4. **Missing Streams** - BDMV folder with missing stream files
5. **Invalid Playlist** - BDMV folder with invalid playlist

### Gate Rules

1. **m2ts Sequence** - Verify m2ts sequence can be played
2. **Encrypted Disc** - Fail-closed for encrypted discs
3. **Stream Integrity** - Verify all streams are present
4. **Playlist Validity** - Verify playlist is valid
5. **DRM Bypass** - No DRM bypass allowed

## Contract Design

### BdmvPlaybackGateRequest

```cpp
struct BdmvPlaybackGateRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string folder_path;
    bool is_mounted_iso{false};
    std::string selected_playlist;
    bool is_encrypted{false};
};
```

### BdmvPlaybackGateResult

```cpp
struct BdmvPlaybackGateResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    bool playback_allowed{false};
    std::string gate_reason;
    std::string failure_reason;
};
```

## Fake Service

Provides deterministic BDMV playback gate results for testing:

- `build_valid_bdmv_gate()` - Valid BDMV folder playback allowed
- `build_mounted_iso_gate()` - Mounted ISO playback allowed
- `build_encrypted_disc_gate()` - Encrypted disc playback denied
- `build_missing_streams_gate()` - Missing streams playback denied
- `build_invalid_playlist_gate()` - Invalid playlist playback denied

## Tests

1. Valid BDMV folder playback allowed
2. Mounted ISO playback allowed
3. Encrypted disc playback denied
4. Missing streams playback denied
5. Invalid playlist playback denied

## Evidence Target

m2ts sequence playback and encrypted disc fail-closed gate.
