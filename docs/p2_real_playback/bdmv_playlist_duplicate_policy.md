# P2-024 BDMV Playlist Duplicate Policy

Updated: 2026-06-25

## Goal

Implement duplicate playlist collapse policy for BDMV folder playback to avoid presenting redundant playlist entries to the user.

## Scope

This task creates contracts and fake services for detecting and collapsing duplicate BDMV playlists.

### Duplicate Playlist Scenarios

1. **Identical Playlists** - Playlists with identical stream sequences
2. **Subset Playlists** - Playlists that are subsets of other playlists
3. **Superset Playlists** - Playlists that contain all streams of other playlists
4. **Similar Playlists** - Playlists with similar but not identical stream sequences

### Duplicate Policy Rules

1. **Keep Longest** - Keep the playlist with the most streams
2. **Keep Main Title** - Keep the playlist marked as main title
3. **Keep First** - Keep the first playlist in case of ties
4. **Collapse All** - Collapse all duplicates into a single entry

## Contract Design

### BdmvPlaylistDuplicateRequest

```cpp
struct BdmvPlaylistDuplicateRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::vector<std::string> playlist_files;
    std::vector<std::vector<std::string>> playlist_streams;
};
```

### BdmvPlaylistDuplicateResult

```cpp
struct BdmvPlaylistDuplicateResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    std::vector<std::string> unique_playlists;
    std::vector<std::string> collapsed_playlists;
    std::string collapse_reason;
};
```

## Fake Service

Provides deterministic duplicate playlist policy results for testing:

- `build_no_duplicates_policy()` - No duplicates found
- `build_identical_playlists_policy()` - Identical playlists collapsed
- `build_subset_playlists_policy()` - Subset playlists collapsed
- `build_superset_playlists_policy()` - Superset playlists collapsed
- `build_similar_playlists_policy()` - Similar playlists collapsed

## Tests

1. No duplicates found
2. Identical playlists collapsed
3. Subset playlists collapsed
4. Superset playlists collapsed
5. Similar playlists collapsed

## Evidence Target

Duplicate playlist collapse policy and tests.
