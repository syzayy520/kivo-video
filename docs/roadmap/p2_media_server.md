# P2 Media Server

Updated: 2026-06-26

This document defines the media server system for P2, covering Emby, Plex, and Jellyfin server integration.

## Purpose

The media server system provides access to media content hosted on media servers (Emby, Plex, Jellyfin) with comprehensive server integration, media identity, and playback capabilities.

## Media Server Protocol

### 1. Server Profiles

#### Emby Server Profile
**Protocol**: Emby REST API
**Use Cases**: Personal media server, home media management
**Authentication**: Token-based
**Features**:
- Library browsing
- Media streaming
- Device management
- User management

#### Plex Server Profile
**Protocol**: Plex Media Server API
**Use Cases**: Personal media server, home media management
**Authentication**: Token-based
**Features**:
- Library browsing
- Media streaming
- Device management
- User management

#### Jellyfin Server Profile
**Protocol**: Jellyfin REST API
**Use Cases**: Open source media server
**Authentication**: Token-based
**Features**:
- Library browsing
- Media streaming
- Device management
- User management

### 2. Server Discovery

#### Local Network Discovery
**Methods**: SSDP, mDNS, network scan
**Discovery Protocols**: UPnP/DLNA, Bonjour
**Timeout**: 5 seconds
**Retry**: 2 attempts

#### Manual Configuration
**Fields**: Server URL, port, API key
**Validation**: Connection test
**Storage**: Local configuration

### 3. Server Authentication

#### Token-Based Authentication
**Process**: Login with username/password → receive token
**Token Lifetime**: Server-specific (typically 30 days)
**Refresh**: Automatic on expiration
**Revocation**: Manual logout

#### API Key Authentication
**Process**: Generate API key in server settings
**Lifetime**: Permanent until revoked
**Usage**: HTTP header or query parameter
**Security**: Server-specific

### 4. Library System

#### Library Structure
**Organization**: Folders, collections, playlists
**Metadata**: Title, year, rating, genre, cast
**Images**: Poster, backdrop, logo
**Media Info**: Codec, resolution, bitrate

#### Library Browsing
**Methods**: List, search, filter
**Pagination**: Page-based
**Sorting**: Title, year, rating, date added
**Filtering**: Genre, year, rating, status

### 5. Media Identity

#### Server Media Identity
**Fields**: Server ID, media ID, media type
**Uniqueness**: Server + media ID combination
**Stability**: Persistent across server restarts
**Mapping**: Server-specific to universal

#### Media Metadata
**Standard Fields**: Title, year, duration, rating
**Server-Specific**: Library metadata, tags
**User Data**: Watch progress, ratings, favorites
**Sync**: Cross-device synchronization

### 6. Streaming Capabilities

#### Direct Play
**Process**: Client plays media directly without transcoding
**Requirements**: Compatible codecs, container format
**Advantages**: No server load, highest quality
**Limitations**: Requires compatible client

#### Direct Stream
**Process**: Server remuxes container without transcoding
**Requirements**: Compatible video/audio codecs
**Advantages**: Minimal server load, compatible format
**Limitations**: Container format may change

#### Transcoding
**Process**: Server transcodes media to compatible format
**Requirements**: Server CPU/GPU resources
**Advantages**: Universal compatibility
**Limitations**: Server load, quality loss

### 7. Playback Integration

#### Playback Session
**Creation**: Start playback session
**Tracking**: Progress, position, duration
**Control**: Play, pause, seek, stop
**Termination**: End session, mark watched

#### Progress Sync
**Upload**: Client reports progress to server
**Download**: Client receives progress from server
**Conflict Resolution**: Latest timestamp wins
**Offline**: Queue progress for later sync

#### Watch Status
**Watched**: Mark as watched when completed
**Unwatched**: Mark as unwatched
**Resume Point**: Last playback position
**History**: Playback history per user

### 8. Error Handling

#### Connection Errors
**Timeout**: Connection timeout (default: 10 seconds)
**Refused**: Server refused connection
**DNS**: DNS resolution failed
**Network**: Network unreachable

#### Authentication Errors
**Invalid Token**: Token expired or invalid
**Permission Denied**: Insufficient permissions
**Rate Limiting**: Too many requests
**Server Error**: Internal server error

#### Media Errors
**Not Found**: Media not available
**Unavailable**: Media temporarily unavailable
**Corrupted**: Media file corrupted
**Unsupported**: Media format not supported

### 9. Privacy and Redaction

#### Server URL Redaction
**Rule**: No internal server URLs
**Method**: Redact server addresses
**Example**: `http://192.168.1.100:8096` → `http://***.***.***.***:8096`

#### Token Redaction
**Rule**: No tokens or API keys
**Method**: Remove or mask credentials
**Example**: `token=abc123` → `token=***`

#### Path Redaction
**Rule**: No full file paths
**Method**: Use redacted paths
**Example**: `/media/movies/movie.mkv` → `movie.mkv`

#### User Data Redaction
**Rule**: No user identifiers
**Method**: Anonymize user data
**Example**: `user_id=12345` → `user_id=***`

## Gate Integration

### MSV-001 Gate Requirements
1. Server profiles defined (Emby, Plex, Jellyfin)
2. Server discovery mechanisms documented
3. Authentication methods defined
4. Library system documented
5. Media identity system defined
6. Streaming capabilities documented
7. Playback integration defined
8. Error handling strategies defined
9. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks media server definitions
- All operations must have error handling
- All credentials must be redacted
- Privacy compliance required

### PMX-001 Gate Integration
- Provider Runtime Matrix includes media server support
- Each server must declare capabilities
- Authentication status must be tracked

## Test Cases

### Positive Test Cases
1. Discover Emby server on local network → Successful discovery
2. Authenticate with valid token → Successful authentication
3. Browse library with valid token → Successful listing
4. Get media metadata with valid ID → Successful metadata
5. Start direct play with compatible media → Successful playback
6. Sync progress across devices → Successful sync

### Negative Test Cases
1. Connect to unavailable server → Graceful failure
2. Authenticate with expired token → Graceful failure
3. Browse library with insufficient permissions → Graceful failure
4. Get media metadata with invalid ID → Graceful failure
5. Start direct play with incompatible media → Graceful fallback
6. Sync progress with network error → Graceful queue

## Implementation Requirements

### Server Client
1. Implement server discovery
2. Handle authentication
3. Browse library
4. Stream media

### Media Identity
1. Map server media to universal identity
2. Handle metadata synchronization
3. Track playback progress
4. Manage watch status

### Playback Integration
1. Create playback sessions
2. Track progress
3. Sync across devices
4. Handle conflicts

### Error Handling
1. Implement connection retry
2. Handle authentication failures
3. Manage media errors
4. Report errors appropriately