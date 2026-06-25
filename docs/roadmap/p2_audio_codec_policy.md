# P2 Audio Codec Policy

Updated: 2026-06-26

This document defines the audio codec policy system for P2, covering codec inventory, routing decisions, and passthrough eligibility.

## Purpose

The audio codec policy system manages audio codec selection, routing decisions, and passthrough eligibility, ensuring optimal audio playback across different hardware configurations.

## Audio Codec Policy System

### 1. Audio Codec Inventory

#### Dolby Codecs
**DD (Dolby Digital)**: AC3, 5.1 channel, up to 640 kbps
**DD+ (Dolby Digital Plus)**: EAC3, 7.1 channel, up to 6 Mbps
**TrueHD**: Lossless, 7.1 channel, up to 18 Mbps
**Atmos**: Object-based audio, immersive sound

#### DTS Codecs
**DTS**: 5.1 channel, up to 1.5 Mbps
**DTS-HD**: Lossless, 7.1 channel, up to 24.5 Mbps
**DTS:X**: Object-based audio, immersive sound

#### PCM and Lossless Codecs
**PCM**: Uncompressed, various sample rates and bit depths
**FLAC**: Lossless compression, various sample rates
**AAC**: Lossy compression, up to 5.1 channel
**Opus**: Lossy compression, low latency

### 2. Codec Capabilities

#### Channel Support
**Mono**: 1 channel
**Stereo**: 2 channels
**Surround**: 5.1, 7.1 channels
**Object-based**: Atmos, DTS:X

#### Sample Rate Support
**Standard**: 44.1 kHz, 48 kHz
**High Resolution**: 88.2 kHz, 96 kHz, 176.4 kHz, 192 kHz
**Professional**: 32 kHz, 64 kHz

#### Bit Depth Support
**Standard**: 16-bit
**High Resolution**: 24-bit
**Professional**: 32-bit float

### 3. Routing Decision Logic

#### Device Capability Detection
**HDMI**: Detect connected audio设备
**S/PDIF**: Detect optical/coaxial connection
**Analog**: Detect analog output
**USB**: Detect USB audio device

#### Codec Selection Algorithm
**Step 1**: Detect device capabilities
**Step 2**: Check codec support
**Step 3**: Evaluate quality preferences
**Step 4**: Select optimal codec
**Step 5**: Apply fallback if needed

#### Routing Priorities
**Priority 1**: Native codec support
**Priority 2**: Highest quality codec
**Priority 3**: Most compatible codec
**Priority 4**: Fallback to PCM

### 4. PCM Fallback Reasons

#### Fallback Triggers
**No Support**: Device doesn't support target codec
**Quality**: PCM provides better quality
**Compatibility**: PCM is universally compatible
**Bandwidth**: Limited bandwidth connection

#### Fallback Strategy
**Downmix**: Mix to stereo if needed
**Resample**: Adjust sample rate if needed
**Bit Depth**: Adjust bit depth if needed
**Channel**: Adjust channel count if needed

### 5. Passthrough Eligibility

#### Passthrough Requirements
**Codec Support**: Device supports target codec
**Bandwidth**: Sufficient bandwidth available
**Licensing**: Proper licensing for codec
**Hardware**: Hardware decoding capability

#### Passthrough Restrictions
**Protected Content**: DRM restrictions
**Bandwidth Limitations**: Insufficient bandwidth
**Device Limitations**: Hardware limitations
**Licensing Issues**: Missing licenses

#### Passthrough Preview
**Eligibility Check**: Pre-check passthrough eligibility
**Quality Assessment**: Assess passthrough quality
**Compatibility Check**: Check device compatibility
**Recommendation**: Provide recommendation

### 6. Metadata Handling

#### Dolby Metadata
**Dialog Normalization**: Maintain dialog level
**Dynamic Range**: Preserve dynamic range
**Downmix coefficients**: For stereo downmix
**Metadata Version**: Track metadata version

#### DTS Metadata
**Profile**: DTS profile selection
**Extensions**: DTS-HD extensions
**Object Metadata**: DTS:X object data
**Bitstream Information**: Codec details

#### General Metadata
**Channel Layout**: Channel configuration
**Sample Rate**: Audio sample rate
**Bit Depth**: Audio bit depth
**Bitrate**: Audio bitrate

### 7. Quality Assessment

#### Audio Quality Metrics
**Bitrate**: Audio bitrate
**Sample Rate**: Audio sample rate
**Bit Depth**: Audio bit depth
**Channel Count**: Number of channels

#### Quality Comparison
**Lossless vs Lossy**: Lossless preferred
**High Resolution vs Standard**: High resolution preferred
**Channel Count**: More channels preferred
**Bitrate**: Higher bitrate preferred

#### Quality Recommendations
**Best Quality**: Highest available quality
**Balanced**: Balance quality and compatibility
**Compatibility**: Maximum compatibility
**Efficiency**: Minimum bandwidth usage

### 8. Error Handling

#### Codec Errors
**Unsupported**: Codec not supported
**Corrupted**: Codec data corrupted
**License**: Missing codec license
**Hardware**: Hardware decoding failed

#### Routing Errors
**No Device**: No audio device detected
**Connection**: Device connection lost
**Configuration**: Device misconfigured
**Conflict**: Multiple device conflicts

#### Fallback Errors
**Fallback Failed**: PCM fallback failed
**Quality Degradation**: Unacceptable quality loss
**Compatibility**: Fallback not compatible
**Resource**: Insufficient resources

### 9. Privacy and Redaction

#### Codec Information Redaction
**Rule**: No detailed codec information
**Method**: Use generic codec references
**Example**: `codec=ac3` → `codec=dolby_digital`

#### Device Information Redaction
**Rule**: No specific device identifiers
**Method**: Use generic device references
**Example**: `device_id=abc123` → `device_id=***`

#### Quality Data Redaction
**Rule**: No detailed quality metrics
**Method**: Use quality categories
**Example**: `bitrate=640kbps` → `quality=high`

## Gate Integration

### ACP-001 Gate Requirements
1. Audio codec inventory defined
2. Codec capabilities documented
3. Routing decision logic defined
4. PCM fallback reasons documented
5. Passthrough eligibility defined
6. Metadata handling documented
7. Quality assessment defined
8. Error handling strategies defined
9. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks audio codec policy definitions
- All operations must have error handling
- All credentials must be redacted
- Privacy compliance required

### PMX-001 Gate Integration
- Provider Runtime Matrix includes audio codec policy support
- Each provider must declare codec support
- Codec selection status must be tracked

## Test Cases

### Positive Test Cases
1. Detect Dolby Digital support on HDMI → Successful detection
2. Select optimal codec for device → Successful selection
3. Check passthrough eligibility → Successful check
4. Apply PCM fallback when needed → Successful fallback
5. Handle Dolby Atmos metadata → Successful handling
6. Assess audio quality → Successful assessment

### Negative Test Cases
1. Detect unsupported codec → Graceful fallback
2. Handle device connection loss → Graceful error handling
3. Check passthrough with insufficient bandwidth → Graceful limitation
4. Handle corrupted codec data → Graceful error handling
5. Apply fallback with quality degradation → Graceful warning
6. Handle missing codec license → Graceful limitation

## Implementation Requirements

### Codec Manager
1. Implement codec inventory
2. Detect device capabilities
3. Handle codec selection
4. Manage codec metadata

### Routing Engine
1. Implement routing decisions
2. Handle device detection
3. Apply fallback logic
4. Manage routing conflicts

### Quality Assessor
1. Assess audio quality
2. Compare codec quality
3. Provide recommendations
4. Handle quality metrics

### Passthrough Manager
1. Check passthrough eligibility
2. Handle passthrough restrictions
3. Manage passthrough preview
4. Handle passthrough errors