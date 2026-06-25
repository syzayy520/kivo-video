# V10 Natural Family Tree

Created: 2026-06-25
Baseline: a44fae9 (P2-REAL-001 PASS)

V10-specific natural family tree for local real playback closure. No flat files.

---

## Source Family

```
source/
  local_file_identity          (existing)
  local_byte_stream            (existing)
```

**Responsibility**: Local file identity and stable byte stream for reading.

---

## Probe Family

```
probe/
  real_probe_runtime           (existing)
```

**Responsibility**: FFmpeg-based media probing for container and stream detection.

---

## Demux Family

```
demux/
  real_demux_runtime           (existing)
  kivo_packet                  (existing)
```

**Responsibility**: FFmpeg demuxing of compressed packets from media container.

---

## Decode Family

```
decode/
  real_software_decode_runtime (existing)
  decoded_frame                (existing)
```

**Responsibility**: FFmpeg software decoding of video/audio frames.

---

## Video Upload Family

```
video_upload/
  decoded_video_frame_uploader   (NEW)
  yuv_rgb_conversion             (NEW)
  d3d11_device_context           (NEW)
  d3d11_texture_upload           (NEW)
```

**Responsibility**: Upload decoded video frames to D3D11 texture (hardware or WARP).

---

## Audio Output Family

```
audio_output/
  decoded_audio_frame_converter  (NEW)
  wasapi_shared_pcm_writer       (NEW)
```

**Responsibility**: Convert decoded audio to PCM and write to WASAPI shared mode.

---

## Clock Family

```
clock/
  master_clock                 (existing)
  av_sync_policy               (existing)
```

**Responsibility**: Master clock for AV synchronization and scheduling decisions.

---

## Graph Family

```
graph/
  playback_graph               (NEW minimal)
```

**Responsibility**: Minimal graph connecting demux→decode→output nodes.

---

## Presenter Family

```
presenter/
  presenter_boundary_shell     (NEW)
```

**Responsibility**: Presenter boundary: receive texture, record first-frame proof.

---

## Lifecycle Family

```
lifecycle/
  playback_session_controller  (NEW)
```

**Responsibility**: Lifecycle management: open→play→pause→resume→seek→stop→close.

---

## Inspector Family

```
inspector/
  playback_inspector_snapshot  (NEW)
  decision_ledger_entry        (NEW)
```

**Responsibility**: Inspector snapshot and decision ledger entries for playback evidence.

---

## Test Family

```
test/
  local_real_playback_integration (NEW)
  local_playback_micro_soak       (NEW)
  failure_injection               (NEW)
```

**Responsibility**: Integration tests for real playback, micro soak, and failure injection.

---

## Dependency Direction

```
video_upload → decode (DecodedFrame)
video_upload → third_party_adapter (D3D11 opaque handle)
audio_output → decode (DecodedFrame)
audio_output → third_party_adapter (WASAPI opaque handle)
graph → decode, demux, clock
presenter → video_upload
lifecycle → graph, presenter, audio_output, inspector
inspector ← all modules (read-only observer)
```

No circular dependencies. All D3D11/WASAPI types stay inside adapter boundary.

---

## Natural Flow

```
Local File → Byte Stream → Probe → Demux → Decode → Video Upload → Presenter
                                         ↓
                                     Audio Output → WASAPI
                                         ↓
                                   Clock → AV Sync
                                         ↓
                                   Graph → Lifecycle
                                         ↓
                                   Inspector → Decision Ledger
```

---

## Allowed Paths

All V10 production and test code must reside within:
- `backend/include/kivo/cinema_engine/<family>/`
- `backend/src/<family>/`
- `backend/tests/<family>/`

No root-level files except project entry (CMakeLists.txt, CMakePresets.json).

---

## Single-File Responsibility

Each file in this tree owns exactly one responsibility. No combined files.

---

## Genealogy Gate

This tree is subject to the V10 genealogy gate:
- No flat files
- No combined responsibilities
- Each family directory has clear boundary
- Dependency direction enforced