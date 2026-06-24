# P2-008 Master Clock / AV Sync Policy

Updated: 2026-06-25

## Goal

Implement audio-master clock policy and AV sync controller skeleton for local playback.

## Clock Sources

| Source | Description | Use Case |
|--------|-------------|----------|
| Audio | Audio endpoint drives clock | Local playback (primary) |
| Video | VSync drives clock | External display sync |
| External | NTP/PTP clock | Network streaming |
| Adaptive | Switches based on conditions | Future use |

## AV Sync Correction Strategies

| Strategy | Description | When Used |
|----------|-------------|-----------|
| skip_frame | Drop video frames to catch up | Video behind audio |
| repeat_frame | Repeat video frames to slow down | Video ahead of audio |
| adjust_speed | Slightly speed up/slow down | Small drift |

## Implementation Plan

1. Create `FakeMasterClockService` that produces MasterClock and AvSyncPolicy
2. Tests covering clock state, sync policy defaults, and drift detection
3. Design doc
