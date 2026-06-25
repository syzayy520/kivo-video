# P2 Source Core

Updated: 2026-06-26

This document defines the source core system for P2, covering source identity, media source kinds, readable byte streams, and source capabilities.

## Purpose

The source core system provides foundational source abstraction for media playback, covering local files, remote streams, and various media source types with consistent interfaces and capabilities.

## Source Identity System

### 1. SourceIdentity
**Purpose**: Unique identifier for media sources
**Fields**:
- `source_id`: UUID v4 (stable across sessions)
- `source_type`: Local, Remote, Stream, Disc
- `created_at`: ISO 8601 timestamp
- `session_count`: Number of sessions using this source

**Stability Rules**:
- Same source file produces same SourceIdentity
- Different copies of same content produce different SourceIdentity
- Source modifications produce new SourceIdentity

### 2. MediaSourceKind
**Purpose**: Classification of media source types
**Values**:
- `LocalFile`: Local file system
- `NetworkFile`: Network file (SMB, NFS, FTP)
- `WebDAV`: WebDAV server
- `AList`: AList cloud storage
- `MediaServer`: Emby, Plex, Jellyfin
- `CloudDrive`: Google Drive, OneDrive
- `DiscImage`: BDMV, ISO
- `IPTV`: Live TV streams
- `Stream`: HTTP/HTTPS streams

### 3. ReadableByteStream
**Purpose**: Abstract interface for reading byte streams
**Operations**:
- `read(offset, size)`: Read bytes at offset
- `seek(offset, whence)`: Seek to position
- `tell()`: Get current position
- `size()`: Get total size
- `close()`: Close stream

**Capabilities**:
- `can_seek`: Supports seeking
- `can_read_range`: Supports range requests
- `can_read_multiple`: Supports concurrent reads

## Source Capabilities

### 1. Range Capabilities
**Feature**: Range request support
**Fields**:
- `supports_range`: Boolean
- `max_range_size`: Maximum range size in bytes
- `range_granularity`: Required range alignment

**Use Cases**:
- Partial content download
- Resumable downloads
- Efficient large file access

### 2. Seek Capabilities
**Feature**: Seeking support
**Fields**:
- `supports_seek`: Boolean
- `seek_granularity`: Minimum seek unit
- `seek_precision`: Seek accuracy

**Use Cases**:
- Chapter navigation
- Resume playback
- Skip intro/outro

### 3. Credential Profile
**Purpose**: Credential management for sources
**Fields**:
- `credential_type`: None, Basic, Token, OAuth
- `credential_storage`: Memory, Keychain, File
- `credential_rotation`: Never, Periodic, OnDemand
- `redact_in_logs`: Boolean

**Privacy Rules**:
- Never log credentials
- Redact in Inspector snapshots
- Secure storage required

### 4. Source Health
**Purpose**: Monitor source availability and performance
**Fields**:
- `health_status`: Healthy, Degraded, Unhealthy, Unknown
- `latency_ms`: Response latency in milliseconds
- `bandwidth_mbps`: Available bandwidth in Mbps
- `error_count`: Recent error count
- `last_error`: Last error message

**Monitoring**:
- Periodic health checks
- Error rate tracking
- Performance monitoring

## Direct Play/Stream Decision

### 1. Direct Play Decision
**Purpose**: Decide if source can be played directly
**Inputs**:
- Source capabilities
- Client capabilities
- Network conditions
- Content requirements

**Decision Logic**:
```
if source_supports_all_codecs AND
   source_bandwidth足够 AND
   client_can_decode_all AND
   no_transcoding_required:
   decision = DIRECT_PLAY
else:
   decision = DIRECT_STREAM_OR_TRANSCODE
```

### 2. Direct Stream Decision
**Purpose**: Decide if source can be streamed directly
**Inputs**:
- Source streaming capabilities
- Network stability
- Buffer requirements
- Error recovery needs

**Decision Logic**:
```
if source_supports_streaming AND
   network_stable AND
   buffer_adequate AND
   error_recovery_possible:
   decision = DIRECT_STREAM
else:
   decision = CACHE_OR_DOWNLOAD
```

## Cache/Retry/Reconnect

### 1. Cache Strategy
**Purpose**: Cache remote content for performance
**Cache Types**:
- `MemoryCache`: In-memory cache
- `DiskCache`: Persistent disk cache
- `HybridCache`: Memory + disk cache

**Cache Policies**:
- `LRU`: Least Recently Used
- `LFU`: Least Frequently Used
- `TTL`: Time To Live
- `SizeLimit`: Maximum cache size

### 2. Retry Strategy
**Purpose**: Handle transient failures
**Retry Policies**:
- `ImmediateRetry`: Retry immediately
- `ExponentialBackoff`: Exponential backoff
- `LinearBackoff`: Linear backoff
- `FixedDelay`: Fixed delay between retries

**Retry Limits**:
- `max_retries`: Maximum retry attempts
- `retry_timeout`: Total retry timeout
- `retry_on`: Specific error codes

### 3. Reconnect Strategy
**Purpose**: Reestablish broken connections
**Reconnect Policies**:
- `AutoReconnect`: Automatic reconnection
- `ManualReconnect`: User-initiated reconnection
- `SessionReconnect`: Reconnect with new session

**Reconnect Conditions**:
- Connection timeout
- Server error
- Network interruption
- Authentication failure

## Inspector Integration

### 1. Source Inspector
**Purpose**: Monitor source behavior during playback
**Fields**:
- `source_identity`: Source being inspected
- `read_operations`: Number of read operations
- `seek_operations`: Number of seek operations
- `error_operations`: Number of error operations
- `average_latency`: Average read latency

### 2. Decision Ledger
**Purpose**: Record source-related decisions
**Entry Types**:
- `SourceSelection`: Source selection decision
- `DirectPlayDecision`: Direct play/stream decision
- `CacheDecision`: Cache strategy decision
- `RetryDecision`: Retry strategy decision

### 3. Error Mapping
**Purpose**: Map source errors to user-friendly messages
**Error Categories**:
- `SourceNotFound`: Source file not found
- `AccessDenied`: Permission denied
- `NetworkError`: Network connectivity issue
- `ServerError`: Server-side error
- `TimeoutError`: Operation timeout
- `CorruptedSource`: Source file corrupted

## Privacy and Redaction

### 1. Source Path Redaction
**Rule**: No full paths in source references
**Method**: Use redacted paths or source IDs
**Example**: `C:\Users\user\Videos\movie.mp4` → `movie.mp4`

### 2. Credential Redaction
**Rule**: No tokens, passwords, or credentials
**Method**: Remove or mask sensitive fields
**Example**: `api_key=abc123` → `api_key=***`

### 3. Network Information Redaction
**Rule**: No internal IP addresses or hostnames
**Method**: Redact network information
**Example**: `192.168.1.100` → `***.***.***.***`

## Gate Integration

### SRC-001 Gate Requirements
1. Source identity system defined
2. Media source kinds documented
3. Readable byte stream interface defined
4. Source capabilities documented
5. Direct play/stream decision logic defined
6. Cache/retry/reconnect strategies defined
7. Inspector/DecisionLedger/ErrorMapping integration defined
8. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks source core definitions
- All source types must have identity support
- All capabilities must be documented

### PMX-001 Gate Integration
- Provider Runtime Matrix includes source core support
- Each provider must declare source capabilities
- Source capabilities must match verification status

## Test Cases

### Positive Test Cases
1. Generate SourceIdentity for same source → Same identity
2. Generate SourceIdentity for different copies → Different identity
3. Read byte stream with range request → Successful read
4. Seek byte stream to position → Successful seek
5. Make direct play decision with capable source → DIRECT_PLAY
6. Cache remote content for performance → Successful caching

### Negative Test Cases
1. Generate SourceIdentity with path leakage → Privacy violation
2. Read byte stream with invalid offset → Graceful failure
3. Seek byte stream to invalid position → Graceful failure
4. Make direct play decision with incapable source → DIRECT_STREAM_OR_TRANSCODE
5. Retry with max retries exceeded → Graceful failure
6. Reconnect with permanent failure → Graceful failure

## Implementation Requirements

### Source Identity
1. UUID v4 generation for stable identity
2. Identity persistence across sessions
3. Identity deduplication for same source
4. Identity versioning for source changes

### Byte Stream
1. Efficient range request handling
2. Concurrent read support
3. Seek operation optimization
4. Memory-efficient buffering

### Capabilities
1. Capability discovery from source
2. Capability matching with client
3. Capability caching for performance
4. Capability update on change