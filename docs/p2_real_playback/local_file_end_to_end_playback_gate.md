# P2-022 Local File End-to-End Playback Gate

Updated: 2026-06-25

## Goal

Implement end-to-end playback gate for local files, verifying the complete playback pipeline from source identification through rendering for common container/codec combinations.

## Scope

This task creates a hermetic test that exercises the full P2.1 local playback path using fake services, verifying that all components work together correctly.

### Container / Codec Combinations

1. **MP4 (H.264/AAC SDR)** - Most common combination
2. **MKV (H.264/AC3 SDR)** - Secondary common combination

### Playback Scenarios

For each container/codec combination, test the following operations:

1. **Play** - Start playback from beginning
2. **Pause** - Pause during playback
3. **Seek** - Seek to specific timestamp
4. **Resume** - Resume from pause
5. **Stop** - Stop playback

## Architecture

### Components Exercised

The end-to-end gate exercises these P2.1 components:

- `SourceCore` - File identity, local file source
- `ContentIntegrityCore` - Content verification
- `DemuxCore` - Probe, stream inventory, demux runtime
- `DecodeCore` - Decode runtime, frame queue
- `GraphCore` - Graph runtime, node/edge management
- `MediaTimelineCore` - Timebase normalization, master clock, AV sync
- `PresentationTimingCore` - Frame pacing, vsync policy
- `VideoRenderCore` - Presenter, SDR upload, visual quality, flip presenter
- `AudioCore` - Audio output decision, audio endpoint
- `SubtitleCore` - Subtitle render plan
- `ResourceCore` - Memory budget
- `SchedulingCore` - Scheduling policy
- `PlaybackCore` - Playback state machine, invariant checker
- `PlaybackInspectorCore` - Inspector runtime
- `DiagnosticCore` - Diagnostic bundle, redaction gate
- `PolicyCore` - Policy decisions
- `ComplianceCore` - Compliance gates
- `FeatureGateCore` - Feature gates
- `CapabilityCore` - Playback eligibility
- `SessionCore` - Session store
- `DecisionLedgerCore` - Decision ledger, replay manifest
- `EngineApi` - Command queue, event bus, state snapshot store
- `Foundation` - Contract registry

### Test Design

Each test follows this pattern:

1. **Setup** - Create fake service instances for all components
2. **Source Identification** - Identify the file and its characteristics
3. **Probe** - Probe the file to get stream inventory
4. **Playback Plan** - Create a playback plan based on capabilities
5. **Execute Operation** - Perform the playback operation (play/pause/seek/resume/stop)
6. **Verify State** - Verify the expected state after the operation
7. **Teardown** - Clean up resources

### Test Data

Tests use deterministic fake data:

- **MP4 file identity**: `test_video_sdr.mp4` with H.264 video, AAC audio
- **MKV file identity**: `test_video_sdr.mkv` with H.264 video, AC3 audio
- **Playback timestamps**: 0, 10s, 30s, 60s, 120s
- **Seek positions**: Various positions within duration

## Test Cases

### MP4 (H.264/AAC SDR)

1. `mp4_play_from_beginning` - Start playback at t=0
2. `mp4_pause_during_playback` - Play then pause at t=10s
3. `mp4_seek_forward` - Seek from t=10s to t=30s
4. `mp4_seek_backward` - Seek from t=30s to t=10s
5. `mp4_resume_from_pause` - Pause at t=10s, resume
6. `mp4_stop_during_playback` - Play then stop at t=30s
7. `mp4_full_playback_sequence` - Play → pause → seek → resume → stop

### MKV (H.264/AC3 SDR)

1. `mkv_play_from_beginning` - Start playback at t=0
2. `mkv_pause_during_playback` - Play then pause at t=10s
3. `mkv_seek_forward` - Seek from t=10s to t=30s
4. `mkv_seek_backward` - Seek from t=30s to t=10s
5. `mkv_resume_from_pause` - Pause at t=10s, resume
6. `mkv_stop_during_playback` - Play then stop at t=30s
7. `mkv_full_playback_sequence` - Play → pause → seek → resume → stop

## Evidence Target

Local H.264/AAC SDR MP4 and H.264/AC3 SDR MKV play/pause/seek/resume/stop gates.
