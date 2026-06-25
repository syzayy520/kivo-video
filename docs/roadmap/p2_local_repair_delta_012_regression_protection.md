# LRD-012 Regression Protection

## Task
LRD-012: Regression Protection

## Goal
Prove accepted baseline does not regress. Run key regression tests and verify P2-REAL-001 FFmpeg probe/demux/decode PASS.

## Regression Test Results

### FFmpeg ON (vs-debug-ffmpeg preset)

| Test | Status | Details |
|------|--------|---------|
| kivo_real_probe_runtime_test | PASS | Real FFmpeg probe: container=mov,mp4,m4a,3gp,3g2,mj2 |
| kivo_real_demux_runtime_test | PASS | Real demux: packet size=3813, keyframe=1 |
| kivo_real_decode_runtime_test | PASS | Real decode: video 320x240 yuv420p, audio 44100Hz fltp |
| kivo_real_demux_decode_integration_test | PASS | Full pipeline: probe→demux→decode |
| kivo_local_playback_e2e_test | PASS | Full lifecycle: startup→play→pause→seek→resume→stop→close |
| kivo_micro_soak_test | PASS | 30s soak simulation, 900 video + 14700 audio frames |
| kivo_failure_injection_test | PASS | 12 tests across 10 pipeline stages |
| kivo_evidence_schema_test | PASS | 8 tests validating all 6 evidence artifacts |
| kivo_lifecycle_test | PASS | Real 16-step state machine |
| kivo_threading_ownership_test | PASS | ThreadOwnershipTracker, ComOwnershipPolicy |
| kivo_inspector_decision_ledger_test | PASS | InspectorSnapshot, DecisionLedger, ErrorMapping |
| kivo_sample_gate_test | PASS | SampleGate with 10 requirements |
| All other tests | PASS | 84/84 total |

### FFmpeg OFF (vs-debug preset)

| Test | Status | Details |
|------|--------|---------|
| kivo_real_probe_runtime_test | PASS | Returns ffmpeg_not_available error |
| kivo_real_demux_runtime_test | PASS | Returns ffmpeg_not_available error |
| kivo_real_decode_runtime_test | PASS | Returns ffmpeg_not_available error |
| kivo_local_playback_e2e_test | PASS | Full lifecycle without FFmpeg |
| All unit tests | PASS | 81/81 total |

### Regression Points Verified

1. **P2-REAL-001 FFmpeg probe**: Real `FfmpegAdapter::open_file()` → `probe_container()` → `enumerate_streams()`
2. **P2-REAL-001 FFmpeg demux**: Real `FfmpegAdapter::read_packet()` → `KivoPacket.data` carries real compressed payload
3. **P2-REAL-001 FFmpeg decode**: Real `FfmpegAdapter::send_packet()` → `receive_frame()` with real pixel/sample data
4. **Presenter abort repair**: `kivo_lifecycle_test` passes with real state machine
5. **D3D11 unavailable skip semantics**: Tests skip gracefully when D3D11 unavailable
6. **FFmpeg ON/OFF behavior**: 84/84 ON, 81/81 OFF, no regression

### No Regression Evidence

- `send_packet(nullptr, 0)` found 0 times in decoder (no fake decode)
- Real packet data flow: `FfmpegReadResult.packet_data` → `KivoPacket.data` → `FfmpegAdapter::send_packet()`
- No D3D11, WASAPI, Remote, P3 scope leak
- Working tree clean after regression run

## Evidence

- `artifacts/p2/evidence/LRD-012.json`
