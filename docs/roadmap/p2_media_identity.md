# P2 Media Identity

Updated: 2026-06-26

This document defines the media identity system for P2, covering identity types, fingerprinting, metadata extraction, and content identification.

## Purpose

The media identity system provides stable, unique identification for media content across different sources, formats, and contexts. It ensures consistent identification for playback, cataloging, and synchronization.

## Identity Types

### 1. MediaIdentity
**Purpose**: Global unique identifier for media content
**Fields**:
- `media_id`: UUID v4 (stable across sources)
- `content_hash`: SHA-256 hash of content fingerprint
- `media_type`: Movie, Episode, Clip, Live, Unknown
- `created_at`: ISO 8601 timestamp
- `source_count`: Number of sources providing this media

**Stability Rules**:
- Same content from different sources produces same MediaIdentity
- Content changes produce new MediaIdentity
- Metadata changes do not affect MediaIdentity

### 2. FileIdentity
**Purpose**: Identity for local/remote files
**Fields**:
- `file_path`: Redacted path (no full path leakage)
- `file_size`: File size in bytes
- `file_hash`: SHA-256 hash of file header
- `container_format`: MP4, MKV, AVI, etc.
- `creation_time`: File creation timestamp
- **Modification Rules**: Path redaction required, no username/path segment leakage

### 3. StreamIdentity
**Purpose**: Identity for individual streams within media
**Fields**:
- `stream_index`: Stream index in container
- `stream_type`: Video, Audio, Subtitle, Data
- `codec_id`: Codec identifier (h264, aac, etc.)
- `language`: ISO 639-2 language code
- `default_flag`: Is default stream
- `forced_flag**: Is forced stream

### 4. ServerItemIdentity
**Purpose**: Identity for items on media servers (Emby, Plex, Jellyfin)
**Fields**:
- `server_id`: Server identifier
- `item_id`: Server-specific item ID
- `server_type`: Emby, Plex, Jellyfin
- `library_id`: Library identifier
- `series_id`: Series identifier (for episodes)
- **Redaction Rules**: No server credentials, no API tokens

### 5. CloudItemIdentity
**Purpose**: Identity for items in cloud storage (Google Drive, OneDrive)
**Fields**:
- `cloud_provider`: Google Drive, OneDrive
- `cloud_item_id`: Provider-specific item ID
- `parent_folder_id`: Parent folder identifier
- `owner_id`: Owner identifier (redacted)
- **Redaction Rules**: No OAuth tokens, no refresh tokens

### 6. PlaylistItemIdentity
**Purpose**: Identity for playlist items
**Fields**:
- `playlist_id`: Playlist identifier
- `playlist_name`: Playlist name (redacted if private)
- `item_position`: Position in playlist
- `item_source`: Source of playlist item
- **Redaction Rules**: No private playlist names without consent

### 7. Episode/Movie Candidate Identity
**Purpose**: Identity for episode/movie matching
**Fields**:
- `series_name`: Series name
- `season_number`: Season number
- `episode_number`: Episode number
- `movie_title`: Movie title
- `release_date`: Release date
- `imdb_id`: IMDb identifier (optional)
- **Matching Rules**: Fuzzy matching for titles, exact matching for IDs

### 8. SampleIdentity
**Purpose**: Identity for test samples
**Fields**:
- `sample_id`: Sample identifier
- `sample_path`: Redacted sample path
- `sample_format`: Container format
- `sample_codecs`: Video/audio codecs
- `sample_duration`: Duration in seconds
- **Privacy Rules**: No full paths, no user-specific paths

## Fingerprinting System

### 1. Content Fingerprint
**Algorithm**: Perceptual hashing + metadata hashing
**Components**:
- Video fingerprint: Visual hash of keyframes
- Audio fingerprint: Audio hash of audio track
- Metadata hash: Hash of key metadata fields

**Stability**:
- Same content with different encoding → Same fingerprint
- Same content with different metadata → Different fingerprint
- Content edits → Different fingerprint

### 2. File Fingerprint
**Algorithm**: File header hash + file size
**Components**:
- Header hash: SHA-256 of first 1MB
- File size: Exact file size in bytes
- Container signature: Magic bytes of container

**Stability**:
- Same file copied → Same fingerprint
- File re-encoded → Different fingerprint
- File truncated → Different fingerprint

### 3. Stream Fingerprint
**Algorithm**: Stream header hash + codec parameters
**Components**:
- Stream header hash: Hash of stream header
- Codec parameters: Codec-specific parameters
- Stream layout: Channel layout, sample rate, etc.

**Stability**:
- Same stream copied → Same fingerprint
- Stream re-encoded → Different fingerprint
- Stream parameters changed → Different fingerprint

## Metadata Extraction

### 1. Container Metadata
**Fields**: title, artist, album, year, genre, track, disc, description, comment
**Extraction**: FFprobe container metadata
**Privacy**: No personal information, no location data

### 2. Stream Metadata
**Fields**: codec_name, codec_long_name, profile, level, width, height, sample_rate, channels
**Extraction**: FFprobe stream metadata
**Accuracy**: Must match actual stream parameters

### 3. Technical Metadata
**Fields**: bitrate, duration, format_name, format_long_name, size
**Extraction**: FFprobe format metadata
**Precision**: Duration to milliseconds, bitrate to bps

## Content Identification

### 1. Manual Identification
**Process**: User provides title, year, season, episode
**Matching**: Fuzzy matching against database
**Confidence**: High confidence for exact matches

### 2. Automatic Identification
**Process**: Content fingerprint matching
**Matching**: Perceptual hash similarity
**Confidence**: Medium confidence for similar content

### 3. Server Identification
**Process**: Server metadata matching
**Matching**: Server-specific ID mapping
**Confidence**: High confidence for server matches

## Privacy and Redaction

### 1. Path Redaction
**Rule**: No full file paths in identity
**Method**: Replace with redacted path (filename only)
**Example**: `C:\Users\user\Videos\movie.mp4` → `movie.mp4`

### 2. Credential Redaction
**Rule**: No tokens, passwords, or credentials
**Method**: Remove or mask sensitive fields
**Example**: `api_key=abc123` → `api_key=***`

### 3. Personal Information Redaction
**Rule**: No usernames, email addresses, or personal data
**Method**: Remove or anonymize personal fields
**Example**: `john@example.com` → `***@***.com`

## Gate Integration

### MID-001 Gate Requirements
1. Media identity types defined
2. Fingerprinting system documented
3. Metadata extraction rules defined
4. Privacy/redaction rules enforced
5. Identity stability rules documented

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks media identity definitions
- All identity types must have privacy rules
- All fingerprinting must be documented

### PMX-001 Gate Integration
- Provider Runtime Matrix includes identity support
- Each provider must declare identity capabilities
- Identity support must match verification status

## Test Cases

### Positive Test Cases
1. Generate MediaIdentity for same content from different sources → Same identity
2. Generate FileIdentity with redacted path → No full path leakage
3. Generate StreamIdentity with correct codec parameters → Accurate identification
4. Generate ServerItemIdentity with redacted credentials → No credential leakage
5. Generate CloudItemIdentity with redacted tokens → No token leakage
6. Generate SampleIdentity with redacted path → Privacy compliant

### Negative Test Cases
1. Generate MediaIdentity with content changes → Different identity
2. Generate FileIdentity with full path → Privacy violation
3. Generate StreamIdentity with incorrect parameters → Inaccurate identification
4. Generate ServerItemIdentity with credentials → Security violation
5. Generate CloudItemIdentity with tokens → Security violation
6. Generate SampleIdentity with personal information → Privacy violation

## Implementation Requirements

### Identity Generation
1. UUID v4 for MediaIdentity
2. SHA-256 for content/file hashing
3. Perceptual hashing for content fingerprinting
4. Metadata extraction via FFprobe

### Privacy Enforcement
1. Automatic path redaction
2. Automatic credential redaction
3. Automatic personal information redaction
4. Privacy validation before identity storage

### Stability Verification
1. Test same content from different sources
2. Test content changes produce new identity
3. Test metadata changes do not affect identity
4. Test file copies produce same identity