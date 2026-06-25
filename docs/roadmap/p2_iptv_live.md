# P2 IPTV Live

Updated: 2026-06-26

This document defines the IPTV live streaming system for P2, covering M3U/M3U8 playlists, HLS live streams, and XMLTV electronic program guides.

## Purpose

The IPTV live streaming system provides access to live television content via internet protocol, with comprehensive playlist management, stream handling, and program guide integration.

## IPTV Live Protocol

### 1. Playlist Formats

#### M3U Format
**Structure**: Plain text playlist
**Fields**: Duration, title, URL, logo, group
**Encoding**: UTF-8
**Use Cases**: Simple playlists, radio streams

#### M3U8 Format
**Structure**: HLS playlist format
**Fields**: Duration, title, URL, logo, group, language
**Encoding**: UTF-8
**Use Cases**: Video playlists, live streams

### 2. HLS Live Stream

#### HLS Protocol
**Protocol**: HTTP Live Streaming
**Structure**: Master playlist → Media playlists → Segments
**Segment Duration**: 2-10 seconds (live), 6-10 seconds (VOD)
**Codec Support**: H.264, H.265/HEVC, AAC, MP3

#### HLS Live Features
**Live Edge**: Near-real-time streaming
**Segment Rotation**: Continuous new segments
**Playlist Update**: Periodic playlist refresh
**Bitrate Switching**: Adaptive bitrate streaming

### 3. Stream Handling

#### Stream Acquisition
**Process**: Download master playlist → select stream → download segments
**Timeout**: 10 seconds per request
**Retry**: 3 attempts with backoff
**Buffer**: 3-5 segments ahead

#### Stream Management
**Start**: Begin playback from live edge
**Pause**: Stop segment download
**Resume**: Reconnect and resume from live edge
**Stop**: Release resources

### 4. XMLTV Electronic Program Guide

#### XMLTV Format
**Structure**: XML-based program guide
**Fields**: Channel, title, start/end time, description, icon
**Encoding**: UTF-8
**Update**: Daily or hourly

#### EPG Features
**Channel Listing**: Available channels
**Program Schedule**: Current and upcoming programs
**Search**: Find programs by title or description
**Favorites**: Mark favorite channels

### 5. Channel Management

#### Channel Grouping
**Groups**: By category (News, Sports, Movies, etc.)
**Custom Groups**: User-defined groups
**Favorites**: Mark favorite channels
**Recent**: Recently watched channels

#### Channel Metadata
**Basic Fields**: Name, logo, URL, group
**Extended Fields**: Language, country, quality
**Program Info**: Current and next program
**Status**: Online/offline status

### 6. HLS Refresh and Updates

#### Playlist Refresh
**Interval**: 2-10 seconds for live streams
**Process**: Re-download playlist, update segment list
**New Segments**: Detect and queue new segments
**Removed Segments**: Skip removed segments

#### Stream Resilience
**Connection Loss**: Automatic reconnection
**Segment Missing**: Skip missing segment
**Playlist Error**: Retry with backoff
**Codec Change**: Handle codec switching

### 7. Segment Fetching

#### Segment Download
**Process**: Download segment sequentially
**Buffering**: Maintain 3-5 segment buffer
**Parallel Downloads**: Optional parallel fetching
**Priority**: Prioritize current segments

#### Segment Processing
**Decrypt**: Handle encrypted segments (AES-128)
**Demux**: Separate audio/video streams
**Timestamp**: Track segment timestamps
**Duration**: Calculate segment duration

### 8. Reconnection Strategy

#### Connection Monitoring
**Detection**: Monitor download progress
**Timeout**: 10 seconds without data
**Retry**: Exponential backoff
**Limit**: Maximum 5 reconnection attempts

#### Reconnection Process
**Step 1**: Detect connection loss
**Step 2**: Wait with backoff
**Step 3**: Re-download playlist
**Step 4**: Resume from live edge

### 9. Channel Switching

#### Switch Process
**Stop**: Stop current stream
**Cleanup**: Release current resources
**Start**: Start new stream
**Buffer**: Rebuild buffer for new stream

#### Switch Optimization
**Pre-buffer**: Pre-buffer next channel
**Cache**: Cache playlist data
**Quick Switch**: Minimize switch time
**State Preservation**: Preserve volume/mute state

### 10. Live Seek Policy

#### Seek Limitations
**Live Edge**: Cannot seek beyond live edge
**Buffer Limit**: Cannot seek before buffer start
**Seek Range**: Limited to buffered content
**Accuracy**: Segment-level accuracy

#### Seek Implementation
**Process**: Find segment containing target time
**Offset**: Calculate offset within segment
**Buffer**: Ensure target is buffered
**Resume**: Resume from seek position

### 11. Error Handling

#### Stream Errors
**Playlist Error**: Invalid or corrupt playlist
**Segment Error**: Failed segment download
**Codec Error**: Unsupported codec
**Network Error**: Network unreachable

#### EPG Errors
**XML Error**: Invalid XMLTV format
**Parse Error**: Failed to parse EPG
**Update Error**: Failed to update EPG
**Data Error**: Missing or corrupt data

### 12. Privacy and Redaction

#### URL Redaction
**Rule**: No full stream URLs
**Method**: Redact server addresses
**Example**: `http://tv.example.com/stream.m3u8` → `http://***.example.com/stream.m3u8`

#### EPG Data Redaction
**Rule**: No personal viewing data
**Method**: Anonymize viewing history
**Example**: `user_watched` → `anonymous_watched`

#### Channel Redaction
**Rule**: No specific channel identifiers
**Method**: Use generic channel references
**Example**: `channel_id=12345` → `channel_id=***`

## Gate Integration

### IPTV-001 Gate Requirements
1. Playlist formats defined (M3U, M3U8)
2. HLS live stream protocol documented
3. Stream handling processes defined
4. XMLTV EPG system documented
5. Channel management defined
6. HLS refresh strategy documented
7. Segment fetching documented
8. Reconnection strategy defined
9. Channel switching documented
10. Live seek policy defined
11. Error handling strategies defined
12. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks IPTV live definitions
- All operations must have error handling
- All credentials must be redacted
- Privacy compliance required

### PMX-001 Gate Integration
- Provider Runtime Matrix includes IPTV live support
- Each stream source must declare capabilities
- Stream health must be monitored

## Test Cases

### Positive Test Cases
1. Parse M3U playlist with valid content → Successful parsing
2. Parse M3U8 playlist with HLS streams → Successful parsing
3. Start HLS live stream with valid playlist → Successful streaming
4. Load XMLTV EPG with valid data → Successful loading
5. Switch channels with minimal delay → Successful switch
6. Reconnect after connection loss → Successful reconnection

### Negative Test Cases
1. Parse M3U playlist with invalid format → Graceful failure
2. Start HLS live stream with invalid playlist → Graceful failure
3. Load XMLTV EPG with invalid XML → Graceful failure
4. Switch channels during high load → Graceful handling
5. Reconnect after multiple failures → Graceful degradation
6. Seek beyond live edge → Graceful limitation

## Implementation Requirements

### Playlist Parser
1. Implement M3U parser
2. Implement M3U8 parser
3. Handle encoding variations
4. Validate playlist format

### Stream Player
1. Implement HLS player
2. Handle live streaming
3. Manage segment buffering
4. Handle codec switching

### EPG Manager
1. Implement XMLTV parser
2. Handle EPG updates
3. Provide search functionality
4. Manage channel metadata

### Channel Manager
1. Implement channel grouping
2. Handle channel switching
3. Manage favorites
4. Track channel status