# P2-006 User Preference Contract Skeleton

Updated: 2026-06-25

## Goal

Define the `PreferenceSnapshot` contract that captures user playback preferences for subtitle, audio, video quality, and HDR settings.

## Preference Categories

| Category | Fields | Default |
|----------|--------|---------|
| Subtitle | language, style, position | Off |
| Audio | language, channel_layout, volume | System default |
| Video | max_resolution, hdr_preference, deinterlace | Auto |
| Playback | auto_play, auto_next, resume | Enabled |

## Implementation Plan

1. Create `PreferenceSnapshot` contract struct
2. Create `FakePreferenceService` that produces default preferences
3. Tests covering construction, field access, and defaults
4. Design doc
