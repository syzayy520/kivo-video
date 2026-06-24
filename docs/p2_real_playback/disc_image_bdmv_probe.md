# P2-023 Disc Image / BDMV / Mounted ISO Probe

Updated: 2026-06-25

## Goal

Implement BDMV folder probe and mounted ISO as BDMV folder detection for disc image playback support.

## Scope

This task creates contracts and fake services for probing BDMV folder structures and mounted ISO images.

### BDMV Folder Structure

A BDMV folder contains:
- `BDMV/PLAYLIST/` - Playlist files (.mpls)
- `BDMV/STREAM/` - Stream files (.m2ts)
- `BDMV/CLIPINF/` - Clip info files (.clpi)
- `BDMV/BACKUP/` - Backup files

### Probe Capabilities

1. **BDMV Folder Detection** - Detect if a path is a BDMV folder
2. **Mounted ISO Detection** - Detect if a path is a mounted ISO with BDMV structure
3. **Playlist Discovery** - Discover available playlists
4. **Stream Discovery** - Discover available streams

## Contract Design

### BdmvProbeRequest

```cpp
struct BdmvProbeRequest {
    ContractMetadata metadata;
    std::string probe_id;
    std::string folder_path;
    bool is_mounted_iso{false};
};
```

### BdmvProbeResult

```cpp
struct BdmvProbeResult {
    ContractMetadata metadata;
    std::string probe_id;
    bool success{false};
    bool is_bdmv_folder{false};
    bool is_mounted_iso{false};
    std::string error_message;
    std::vector<std::string> playlist_files;
    std::vector<std::string> stream_files;
    int64_t total_size_bytes{0};
};
```

## Fake Service

Provides deterministic BDMV probe results for testing:

- `build_successful_bdmv_probe()` - Successful BDMV folder probe
- `build_failed_bdmv_probe()` - Failed probe with error message
- `build_mounted_iso_probe()` - Successful mounted ISO probe
- `build_empty_bdmv_probe()` - BDMV folder with no playlists
- `build_bdmv_probe_with_playlists()` - BDMV with specific playlists

## Tests

1. Successful BDMV folder probe
2. Failed BDMV probe (not a BDMV folder)
3. Mounted ISO probe
4. Empty BDMV folder probe
5. BDMV with playlists probe

## Evidence Target

BDMV folder and mounted ISO probe.
