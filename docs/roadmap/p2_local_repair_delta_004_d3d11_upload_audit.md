# P2-LOCAL-REPAIR-DELTA-004 D3D11 Real Decoded Frame Upload

Created: 2026-06-26
Task Family: local repair delta/video
Depends On: LRD-003 D3D11 Device Reality Audit
Status: AUDIT

---

## 1. Audit Goal

Upload real decoded frame data into a D3D11 texture boundary. No empty texture, dummy frame, bool-only proof, or log-only proof.

### Non-Goals
- No hardware decode
- No zero-copy renderer

---

## 2. Current DecodedFrame Structure

### File: `backend/include/kivo/cinema_engine/decode_core/decoded_frame.hpp`
- `DecodedFrame` struct with:
  - `frame_id`, `stream_kind`, `pts`, `dts`, `duration`
  - `width`, `height`, `pixel_format`
  - `sample_rate`, `channels`, `sample_format`
  - `is_eof`, `frame_ref`
  - `frame_data`: actual pixel/sample data bytes
  - `linesize[4]`: linesize/stride per plane for video frames

### Current D3D11 Upload Interface
- `d3d11_texture_upload.hpp`: Interface for texture creation and upload
- Current status: CONTRACT_PASS (stub implementation)

---

## 3. Real Decoded Frame Upload Requirements

### Frame Data Requirements
1. **Real Pixel Data**: `frame_data` must contain actual decoded pixels
2. **Real Linesize**: `linesize[4]` must match actual frame layout
3. **Real Dimensions**: `width` and `height` must match actual frame
4. **Real Pixel Format**: `pixel_format` must match actual format (yuv420p, nv12, etc.)

### D3D11 Texture Upload Requirements
1. **Texture Creation**: Real `ID3D11Texture2D` with correct dimensions/format
2. **Data Upload**: Real `UpdateSubresource` or map/unmap with actual frame data
3. **Format Conversion**: YUV→RGB/NV12 conversion if needed
4. **Upload Verification**: Confirm texture contains uploaded data

### Forbidden Proofs
- empty texture ≠ real upload
- dummy frame ≠ real decoded data
- bool-only proof ≠ real upload
- log-only proof ≠ real upload

---

## 4. D3D11 Texture Upload Strategy

### Upload Pipeline
```
DecodedFrame → YUV→RGB/NV12 → D3D11 Texture → Upload → Verify
```

### Format Handling
1. **YUV420P**: 3 planes (Y, U, V) with separate linesizes
2. **NV12**: 2 planes (Y, UV interleaved)
3. **RGB24**: 1 plane (interleaved RGB)
4. **Other formats**: Conversion required

### Upload Methods
1. **UpdateSubresource**: Simple upload, may be slower
2. **Map/Unmap**: Faster, requires staging texture
3. **CopyResource**: For GPU-resident textures

---

## 5. D3D11 Upload Test Strategy

### Required Reality Tests
1. **YUV420P Upload Test**: Upload real YUV420P frame data
2. **NV12 Upload Test**: Upload real NV12 frame data
3. **RGB24 Upload Test**: Upload real RGB24 frame data
4. **Format Conversion Test**: YUV→RGB conversion proof
5. **Upload Verification Test**: Verify texture contains uploaded data
6. **Linesize Handling Test**: Handle different linesize values

### Test Evidence Requirements
- Real frame data in `frame_data` vector
- Real linesize in `linesize[4]` array
- Real texture metadata (width, height, format)
- Real upload result (success/failure)
- Real upload verification (data integrity)

---

## 6. D3D11 Upload Inspector Snapshot Requirements

### Decoded Frame Upload Snapshot
```json
{
  "decoded_frame_upload": {
    "frame_id": "frame_001",
    "stream_kind": "video",
    "pts": 0,
    "width": 320,
    "height": 240,
    "pixel_format": "yuv420p",
    "frame_data_size": 115200,
    "linesize": [320, 160, 160, 0],
    "upload_success": true,
    "texture_width": 320,
    "texture_height": 240,
    "texture_format": "DXGI_FORMAT_NV12",
    "upload_method": "UpdateSubresource",
    "upload_verification": true,
    "timestamp": "2026-06-26T00:50:00Z"
  }
}
```

---

## 7. D3D11 Upload DecisionLedger Requirements

### Upload Decision
```json
{
  "decision_id": "d3d11_frame_upload",
  "timestamp": "2026-06-26T00:50:00Z",
  "input": {
    "frame_id": "frame_001",
    "pixel_format": "yuv420p",
    "width": 320,
    "height": 240,
    "frame_data_size": 115200
  },
  "output": {
    "upload_success": true,
    "texture_format": "DXGI_FORMAT_NV12",
    "upload_method": "UpdateSubresource",
    "upload_verification": true
  },
  "runtime_status": "RUNTIME_PASS",
  "environment_kind": "hardware|warp",
  "environment_available": true
}
```

---

## 8. D3D11 Upload Evidence Artifact Requirements

### Evidence File: `artifacts/p2/evidence/LRD-004.json`
```json
{
  "task_id": "LRD-004",
  "task_family": "local repair delta/video",
  "execution_timestamp": "2026-06-26T00:50:00Z",
  "status": "RUNTIME_PASS|BLOCKED_ENV",
  "goal": "upload real decoded frame data into D3D11 texture",
  "decoded_frame": {
    "frame_id": "frame_001",
    "pixel_format": "yuv420p",
    "width": 320,
    "height": 240,
    "frame_data_size": 115200,
    "linesize": [320, 160, 160, 0]
  },
  "d3d11_texture_upload": {
    "texture_width": 320,
    "texture_height": 240,
    "texture_format": "DXGI_FORMAT_NV12",
    "upload_success": true,
    "upload_method": "UpdateSubresource",
    "upload_verification": true
  },
  "format_conversion": {
    "source_format": "yuv420p",
    "target_format": "nv12",
    "conversion_success": true
  },
  "inspector_snapshot": "...",
  "decision_ledger": "...",
  "schema_validation": "PASS",
  "anti_fake_assertions": {
    "real_frame_data": true,
    "real_linesize": true,
    "real_texture_upload": true,
    "no_empty_texture": true,
    "no_dummy_frame": true
  },
  "next_task": "LRD-005 Presenter Boundary Reality Audit"
}
```

---

## 9. D3D11 Upload Schema Gate Requirements

### Schema Validation
1. **Real Frame Data Required**: frame_data_size > 0
2. **Real Linesize Required**: linesize values must match dimensions
3. **Upload Success Required**: upload_success must be true for RUNTIME_PASS
4. **Upload Verification Required**: upload_verification must be true
5. **Anti-Fake Validation**: no empty texture or dummy frame

### Schema Negative Tests
1. **Empty Texture + RUNTIME_PASS**: fails (empty texture not real upload)
2. **Dummy Frame + RUNTIME_PASS**: fails (dummy frame not real data)
3. **Bool-Only Proof**: fails (insufficient evidence)
4. **Missing Upload Verification**: fails (verification required)

---

## 10. D3D11 Upload Redline Gate Requirements

### Forbidden Terms in D3D11 Upload Code
- empty texture, dummy frame, placeholder data, stub upload
- bool-only proof, log-only proof, interface contract only
- format conversion failed but upload success

### Allowlist for D3D11 Upload Terms
- real frame data, actual pixels, decoded frame
- texture upload, UpdateSubresource, map/unmap
- format conversion, YUV→RGB, NV12
- RUNTIME_PASS, BLOCKED_ENV, CONTRACT_PASS

### Scan Targets
1. `backend/include/kivo/cinema_engine/video_upload/*.hpp`
2. `backend/src/video_upload/*.cpp`
3. `backend/tests/video_upload/*.cpp`
4. `artifacts/p2/evidence/LRD-004.json`

---

## 11. D3D11 Upload Anti-Fake Gate Requirements

### Real Upload Proof Requirements
1. **Real Frame Data**: `frame_data` contains actual decoded pixels
2. **Real Linesize**: `linesize[4]` matches actual frame layout
3. **Real Texture Creation**: Real `ID3D11Texture2D` creation
4. **Real Data Upload**: Real `UpdateSubresource` call
5. **Real Upload Verification**: Texture contains uploaded data

### Forbidden Fake Proofs
- empty texture → CONTRACT_PASS only
- dummy frame data → CONTRACT_PASS only
- bool-only upload → insufficient proof
- log-only upload → insufficient proof

---

## 12. D3D11 Upload Clean Workspace Gate

### Workspace Requirements
- No root-level texture outputs
- No generated texture files in source root
- Evidence file in `artifacts/p2/evidence/`
- Test outputs in `backend/tests/video_upload/`

### Build Artifacts
- D3D11 upload test executables in `build/` directory
- No D3D11 artifacts in source tree
- CMake registration proper

---

## 13. D3D11 Upload Final Report Rules

### Classification Options
1. **RUNTIME_PASS**: Real decoded frame data uploaded to real D3D11 texture
2. **BLOCKED_ENV**: D3D11 environment unavailable, honest block
3. **CONTRACT_PASS**: Interface contract proven but not runtime
4. **FAIL**: Required proof missing or conflicting

### Report Requirements
- Branch: kivo-video-p2-real-001-repair-ffmpeg-proof
- HEAD: commit hash after D3D11 upload audit
- Frame data proof: real decoded pixels in frame_data
- Texture upload proof: real D3D11 texture with uploaded data
- Evidence artifact: LRD-004.json
- Inspector snapshot: decoded frame upload snapshot
- DecisionLedger: upload decision

### Forbidden Claims
- Never claim real upload if empty texture
- Never claim real upload if dummy frame
- Never claim runtime PASS if BLOCKED_ENV
- Never claim upload verification if not verified

---

## 14. Immediate Next Steps

1. Create D3D11 upload reality audit test
2. Implement real YUV420P upload
3. Implement real NV12 upload
4. Implement real RGB24 upload
5. Implement format conversion
6. Implement upload verification
7. Generate LRD-004.json evidence
8. Update backend_task_checklist.md
9. Update current_state.md

---

## 15. Self-Check Checklist

- [ ] Real frame data in frame_data vector
- [ ] Real linesize in linesize[4] array
- [ ] Real D3D11 texture creation
- [ ] Real data upload with UpdateSubresource
- [ ] Real upload verification
- [ ] Format conversion working
- [ ] Inspector snapshot generated
- [ ] DecisionLedger entries created
- [ ] Evidence artifact generated
- [ ] Schema validation passed
- [ ] Redline scan passed
- [ ] Anti-fake gate passed
- [ ] Clean workspace verified
- [ ] Checklist updated
- [ ] Current state updated

---

Classification: D3D11_REAL_DECODED_FRAME_UPLOAD_PLANNED
Evidence Output: artifacts/p2/evidence/LRD-004.json
Next Task: LRD-005 Presenter Boundary Reality Audit