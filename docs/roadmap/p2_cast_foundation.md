# P2 Cast Foundation

Updated: 2026-06-26

This document defines the Cast foundation system for P2, covering Cast/AirPlay abstraction, device identity, session state, and handoff policies.

## Purpose

The Cast foundation system provides a unified abstraction for casting media to external devices, supporting both Google Cast and Apple AirPlay protocols with comprehensive session management.

## Cast Foundation System

### 1. Cast/AirPlay Abstraction

#### Unified Cast Interface
**Protocols**: Google Cast, Apple AirPlay
**Discovery**: mDNS/DNS-SD, Bonjour
**Connection**: TCP/IP, HTTP
**Streaming**: RTP, HTTP Live Streaming

#### Protocol Abstraction
**Device Discovery**: Unified device discovery
**Session Management**: Unified session control
**Media Control**: Play, pause, seek, volume
**Status Monitoring**: Connection status, playback state

### 2. Device Identity

#### Cast Device Identity
**Fields**: Device ID, device name, device type, protocol
**Capabilities**: Supported codecs, resolutions, features
**Status**: Online/offline, busy/idle
**Network**: IP address, port, protocol version

#### AirPlay Device Identity
**Fields**: Device ID, device name, device type, protocol
**Capabilities**: Supported formats, features
**Status**: Online/offline, busy/idle
**Network**: IP address, port, protocol version

### 3. Session State Management

#### Cast Session States
**Idle**: No active session
**Connecting**: Establishing connection
**Connected**: Connection established
**Playing**: Media playing
**Paused**: Media paused
**Buffering**: Media buffering
**Error**: Error occurred

#### Session Transitions
**Start**: Idle → Connecting → Connected
**Play**: Connected → Playing
**Pause**: Playing → Paused
**Stop**: Any → Idle
**Error**: Any → Error

### 4. Handoff Policy

#### Local to Cast Handoff
**Process**: Transfer playback from local to cast device
**State Transfer**: Transfer playback state
**Quality Adaptation**: Adapt to device capabilities
**Fallback**: Fallback to local if cast fails

#### Cast to Local Handoff
**Process**: Transfer playback from cast to local device
**State Transfer**: Transfer playback state
**Quality Adaptation**: Adapt to local capabilities
**Seamless**: Maintain playback continuity

### 5. Local/Cast Split

#### Local Playback
**Processing**: Local device processing
**Rendering**: Local device rendering
**Audio**: Local device audio output
**Control**: Local device control

#### Cast Playback
**Processing**: Cast device processing
**Rendering**: Cast device rendering
**Audio**: Cast device audio output
**Control**: Local device control (remote)

### 6. Inspector Integration

#### Cast Inspector
**Session Info**: Cast session details
**Device Info**: Cast device information
**Playback Info**: Playback state and progress
**Error Info**: Error details and recovery

#### Inspector Data
**Connection**: Connection quality and stability
**Performance**: Streaming performance metrics
**Compatibility**: Device compatibility information
**Health**: Device health status

### 7. DecisionLedger Integration

#### Cast Decisions
**Device Selection**: Cast device selection decision
**Session Start**: Cast session start decision
**Quality Adaptation**: Quality adaptation decision
**Handoff**: Handoff decision

#### Decision Logging
**Timestamp**: Decision timestamp
**Reason**: Decision reason
**Outcome**: Decision outcome
**Impact**: Impact on playback

### 8. Feasibility Assessment

#### Technical Feasibility
**Device Support**: Device supports casting
**Network**: Network supports casting
**Codec**: Codec compatibility
**Bandwidth**: Sufficient bandwidth

#### Protocol Feasibility
**Cast Protocol**: Cast protocol available
**AirPlay Protocol**: AirPlay protocol available
**Version**: Protocol version compatibility
**Features**: Required features supported

### 9. Legal and Protocol Review

#### Legal Considerations
**Licensing**: Codec licensing requirements
**DRM**: DRM compatibility
**Terms**: Service terms compliance
**Privacy**: Privacy requirements

#### Protocol Review
**Compatibility**: Protocol compatibility
**Security**: Security requirements
**Performance**: Performance requirements
**Reliability**: Reliability requirements

### 10. Error Handling

#### Connection Errors
**Timeout**: Connection timeout
**Refused**: Connection refused
**Lost**: Connection lost
**Failed**: Connection failed

#### Streaming Errors
**Buffering**: Buffering issues
**Quality**: Quality degradation
**Dropout**: Audio/video dropout
**Sync**: Audio/video sync issues

#### Session Errors
**Start**: Session start failed
**Control**: Control command failed
**End**: Session end failed
**State**: State inconsistency

### 11. Privacy and Redaction

#### Device Information Redaction
**Rule**: No specific device identifiers
**Method**: Use generic device references
**Example**: `device_id=abc123` → `device_id=***`

#### Network Information Redaction
**Rule**: No specific network addresses
**Method**: Use generic network references
**Example**: `ip=192.168.1.100` → `ip=***.***.***.***`

#### Session Data Redaction
**Rule**: No detailed session data
**Method**: Use generic session references
**Example**: `session_id=xyz789` → `session_id=***`

## Gate Integration

### CAST-001 Gate Requirements
1. Cast/AirPlay abstraction defined
2. Device identity system documented
3. Session state management defined
4. Handoff policy documented
5. Local/cast split defined
6. Inspector integration documented
7. DecisionLedger integration defined
8. Feasibility assessment documented
9. Legal/protocol review completed
10. Error handling strategies defined
11. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks Cast foundation definitions
- All operations must have error handling
- All credentials must be redacted
- Privacy compliance required

### PMX-001 Gate Integration
- Provider Runtime Matrix includes Cast foundation support
- Each cast device must declare capabilities
- Session status must be tracked

## Test Cases

### Positive Test Cases
1. Discover Cast device on network → Successful discovery
2. Establish Cast session → Successful connection
3. Start media playback on cast device → Successful playback
4. Transfer playback from local to cast → Successful handoff
5. Monitor cast session status → Successful monitoring
6. Handle cast device reconnection → Successful reconnection

### Negative Test Cases
1. Discover unavailable device → Graceful failure
2. Establish session with incompatible device → Graceful failure
3. Start playback with unsupported codec → Graceful fallback
4. Transfer playback with network error → Graceful handling
5. Monitor session with connection loss → Graceful error handling
6. Handle device disconnection → Graceful recovery

## Implementation Requirements

### Cast Manager
1. Implement device discovery
2. Handle session management
3. Manage media control
4. Handle errors

### Device Manager
1. Implement device identity
2. Handle device capabilities
3. Manage device status
4. Handle device events

### Session Manager
1. Implement session state
2. Handle session transitions
3. Manage session persistence
4. Handle session errors

### Inspector
1. Implement cast inspection
2. Collect session data
3. Monitor performance
4. Report errors