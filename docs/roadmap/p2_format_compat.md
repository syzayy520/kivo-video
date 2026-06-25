# P2 Format Compatibility Matrix

Updated: 2026-06-26

This document defines the format compatibility matrix for P2, documenting supported container formats and codec combinations with real sample verification status.

## Purpose

The format compatibility matrix tracks which container formats and codec combinations are supported, verified with real samples, and documented for user reference.

## Format Matrix

### 1. Container Formats

#### MP4 (MPEG-4 Part 14)
- **Extensions**: .mp4, .m4v, .m4a, .3gp, .3g2
- **Codec Support**: H.264, H.265/HEVC, AAC, MP3, AC3, EAC3, FLAC
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### MKV (Matroska Video)
- **Extensions**: .mkv, .webm
- **Codec Support**: H.264, H.265/HEVC, VP8, VP9, AV1, AAC, MP3, AC3, EAC3, FLAC, Opus
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### AVI (Audio Video Interleave)
- **Extensions**: .avi
- **Codec Support**: H.264, MPEG-4, MP3, AC3
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### MOV (QuickTime Movie)
- **Extensions**: .mov
- **Codec Support**: H.264, H.265/HEVC, AAC, MP3, AC3
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### TS (MPEG Transport Stream)
- **Extensions**: .ts, .m2ts
- **Codec Support**: H.264, H.265/HEVC, AAC, AC3, EAC3
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### FLV (Flash Video)
- **Extensions**: .flv
- **Codec Support**: H.264, VP6, AAC, MP3
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### WMV (Windows Media Video)
- **Extensions**: .wmv
- **Codec Support**: WMV9, VC-1, WMA
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### MPEG (MPEG-1/MPEG-2)
- **Extensions**: .mpg, .mpeg
- **Codec Support**: MPEG-1, MPEG-2, MP2, MP3
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

#### OGG (Ogg Container)
- **Extensions**: .ogv, .oga
- **Codec Support**: Theora, VP8, Vorbis, Opus
- **Sample Verification**: Real samples available
- **Runtime Status**: RUNTIME_PASS
- **Sample Gap**: None

### 2. Codec Support Matrix

#### Video Codecs
| Codec | MP4 | MKV | AVI | MOV | TS | FLV | WMV | MPEG | OGG |
|-------|-----|-----|-----|-----|----|----|-----|------|-----|
| H.264 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ |
| H.265/HEVC | ✅ | ✅ | ❌ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| VP8 | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| VP9 | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| AV1 | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| MPEG-4 | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| WMV9 | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ |
| VC-1 | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ |
| Theora | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| MPEG-1 | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ |
| MPEG-2 | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ | ✅ | ❌ |

#### Audio Codecs
| Codec | MP4 | MKV | AVI | MOV | TS | FLV | WMV | MPEG | OGG |
|-------|-----|-----|-----|-----|----|----|-----|------|-----|
| AAC | ✅ | ✅ | ❌ | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ |
| MP3 | ✅ | ✅ | ✅ | ✅ | ❌ | ✅ | ❌ | ✅ | ❌ |
| AC3 | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| EAC3 | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ |
| FLAC | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Opus | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| WMA | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ |
| Vorbis | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| MP2 | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ |

### 3. Sample Verification Status

#### Verified Formats (RUNTIME_PASS)
1. **MP4 H.264 AAC**: Real sample verified, full playback pipeline
2. **MKV H.264 AAC**: Real sample verified, full playback pipeline
3. **MKV H.265 HEVC AAC**: Real sample verified, full playback pipeline
4. **AVI MPEG-4 MP3**: Real sample verified, full playback pipeline
5. **MOV H.264 AAC**: Real sample verified, full playback pipeline
6. **TS H.264 AAC**: Real sample verified, full playback pipeline
7. **FLV H.264 AAC**: Real sample verified, full playback pipeline
8. **WMV WMV9 WMA**: Real sample verified, full playback pipeline
9. **MPEG MPEG-1 MP2**: Real sample verified, full playback pipeline
10. **OGG Theora Vorbis**: Real sample verified, full playback pipeline

#### Contract-Pass Formats (CONTRACT_PASS)
1. **MP4 H.265 HEVC AAC**: Contract pass, real sample not available
2. **MKV VP8 Vorbis**: Contract pass, real sample not available
3. **MKV VP9 Opus**: Contract pass, real sample not available
4. **MKV AV1 Opus**: Contract pass, real sample not available
5. **TS H.265 HEVC EAC3**: Contract pass, real sample not available
6. **3GP H.264 AAC**: Contract pass, real sample not available
7. **OGV VP8 Vorbis**: Contract pass, real sample not available

#### Missing Formats (NOT_IMPLEMENTED)
1. **WMV H.264 AAC**: Not implemented, WMV9 only
2. **AVI H.265 HEVC**: Not implemented, legacy codec only
3. **MOV VP8 Vorbis**: Not implemented, QuickTime codec only

## Sample Gap List

### High Priority Gaps
1. **H.265/HEVC MP4 sample**: Need real sample for full runtime verification
2. **VP8 MKV sample**: Need real sample for full runtime verification
3. **VP9 MKV sample**: Need real sample for full runtime verification

### Medium Priority Gaps
1. **AV1 MKV sample**: Need real sample for full runtime verification
2. **EAC3 TS sample**: Need real sample for full runtime verification
3. **Opus MKV sample**: Need real sample for full runtime verification

### Low Priority Gaps
1. **FLAC MP4 sample**: Need real sample for full runtime verification
2. **3GP H.264 sample**: Need real sample for full runtime verification
3. **OGV VP8 sample**: Need real sample for full runtime verification

## Verification Requirements

### Runtime Verification
1. Real sample must be present in test environment
2. Full playback pipeline must work (probe → demux → decode → render)
3. No stub or fake implementations allowed
4. Real FFmpeg must be used for probe/demux/decode

### Contract Verification
1. Format detection must work
2. Stream discovery must work
3. Codec identification must work
4. Basic metadata extraction must work

### Sample Requirements
1. Sample must be > 30 seconds duration
2. Sample must have both video and audio streams
3. Sample must be valid and playable
4. Sample must not be corrupted or damaged

## Gate Integration

### FMT-001 Gate Requirements
1. Format compatibility matrix defined
2. Sample verification status documented
3. Sample gap list documented
4. Runtime verification requirements defined
5. Contract verification requirements defined

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks format compatibility matrix
- All verified formats must have RUNTIME_PASS status
- All missing formats must have CONTRACT_PASS status with sample gap list

### PMX-001 Gate Integration
- Provider Runtime Matrix includes format compatibility status
- Each provider must declare supported formats
- Format support must match verification status

## Test Cases

### Positive Test Cases
1. Scan format matrix with all verified formats → PASS
2. Scan format matrix with all contract-pass formats → PASS
3. Scan format matrix with sample gap list → PASS
4. Scan format matrix with verification requirements → PASS
5. Scan format matrix with gate integration → PASS
6. Scan format matrix with test cases → PASS

### Negative Test Cases
1. Scan format matrix with unverified format → VIOLATION
2. Scan format matrix with fake runtime pass → VIOLATION
3. Scan format matrix with missing sample gap → VIOLATION
4. Scan format matrix with incorrect codec support → VIOLATION
5. Scan format matrix with missing verification requirements → VIOLATION
6. Scan format matrix with premature completion claim → VIOLATION

## Implementation Requirements

### Matrix Maintenance
1. Update matrix when new formats are verified
2. Update matrix when new samples are available
3. Update matrix when new codecs are supported
4. Update matrix when verification status changes

### Sample Management
1. Track sample availability
2. Track sample verification status
3. Track sample gaps
4. Track sample quality

### Verification Tracking
1. Track runtime verification status
2. Track contract verification status
3. Track sample verification status
4. Track gate integration status