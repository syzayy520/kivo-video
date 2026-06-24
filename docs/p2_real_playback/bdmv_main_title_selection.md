# P2-025 BDMV Main Title Selection

Updated: 2026-06-25

## Goal

Implement main playlist selection for BDMV folders to automatically identify and select the main title playlist for playback.

## Scope

This task creates contracts and fake services for selecting the main title playlist from BDMV folder structures.

### Main Title Selection Criteria

1. **Longest Duration** - Select the playlist with the longest total duration
2. **Most Chapters** - Select the playlist with the most chapters
3. **Largest File Size** - Select the playlist with the largest total file size
4. **Playlist Number** - Select the playlist with the lowest number (often the main title)
5. **User Preference** - Allow user to override automatic selection

### Selection Priority

1. **User Override** - If user has selected a playlist, use that
2. **Longest Duration** - If no user override, select by longest duration
3. **Most Chapters** - If durations are equal, select by most chapters
4. **Largest File Size** - If chapters are equal, select by largest file size
5. **Playlist Number** - If all else is equal, select by lowest playlist number

## Contract Design

### BdmvMainTitleRequest

```cpp
struct BdmvMainTitleRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::vector<std::string> playlist_files;
    std::vector<int64_t> playlist_durations;
    std::vector<int> playlist_chapters;
    std::vector<int64_t> playlist_sizes;
    std::string user_override_playlist;
};
```

### BdmvMainTitleResult

```cpp
struct BdmvMainTitleResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    std::string selected_playlist;
    std::string selection_reason;
    std::vector<std::string> all_playlists;
};
```

## Fake Service

Provides deterministic main title selection results for testing:

- `build_longest_duration_selection()` - Select by longest duration
- `build_most_chapters_selection()` - Select by most chapters
- `build_largest_size_selection()` - Select by largest file size
- `build_lowest_number_selection()` - Select by lowest playlist number
- `build_user_override_selection()` - Select user override playlist

## Tests

1. Select by longest duration
2. Select by most chapters
3. Select by largest file size
4. Select by lowest playlist number
5. Select user override playlist

## Evidence Target

Main playlist selection reason in Inspector.
