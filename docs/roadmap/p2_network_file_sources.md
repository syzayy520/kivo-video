# P2 Network File Sources

Updated: 2026-06-26

This document defines the network file sources system for P2, covering SMB, NFS, FTP, FTPS, SFTP, HTTP/HTTPS, and DLNA/UPnP sources.

## Purpose

The network file sources system provides access to media files stored on network servers, file shares, and web servers with appropriate protocol support and error handling.

## Network Source Types

### 1. SMB (Server Message Block)
**Protocol**: SMB/CIFS
**Use Cases**: Windows file shares, NAS devices
**Authentication**: NTLM, Kerberos
**Features**:
- File browsing
- Range requests
- File locking
- Unicode support

**Capabilities**:
- `supports_range`: true
- `supports_seek`: true
- `supports_streaming`: true
- `max_concurrent_connections`: 16

### 2. NFS (Network File System)
**Protocol**: NFS v3/v4
**Use Cases**: Linux/Unix file shares, NAS devices
**Authentication**: Kerberos, AUTH_SYS
**Features**:
- File browsing
- Range requests
- Symbolic links
- File attributes

**Capabilities**:
- `supports_range`: true
- `supports_seek`: true
- `supports_streaming`: true
- `max_concurrent_connections`: 32

### 3. FTP (File Transfer Protocol)
**Protocol**: FTP/FTPS
**Use Cases**: Traditional file servers
**Authentication**: Username/password
**Features**:
- File listing
- Passive mode
- Binary transfer
- Resume support

**Capabilities**:
- `supports_range`: false
- `supports_seek`: false
- `supports_streaming`: true
- `max_concurrent_connections`: 8

### 4. SFTP (SSH File Transfer Protocol)
**Protocol**: SFTP over SSH
**Use Cases**: Secure file transfers
**Authentication**: SSH keys, password
**Features**:
- Encrypted transfer
- File browsing
- Resume support
- Directory listing

**Capabilities**:
- `supports_range`: false
- `supports_seek`: false
- `supports_streaming`: true
- `max_concurrent_connections**: 4

### 5. HTTP/HTTPS Direct Media
**Protocol**: HTTP/HTTPS
**Use Cases**: Direct media file URLs
**Authentication**: None, Basic, Token
**Features**:
- Range requests
- Content-Length
- ETag support
- Cache-Control

**Capabilities**:
- `supports_range`: true
- `supports_seek`: true
- `supports_streaming`: true
- `max_concurrent_connections**: 32

### 6. HTTP/HTTPS Stream
**Protocol**: HTTP/HTTPS
**Use Cases**: Live streams, HLS, DASH
**Authentication**: None, Token
**Features**:
- Chunked transfer
- Streaming protocol
- Adaptive bitrate
- Segment fetching

**Capabilities**:
- `supports_range`: false
- `supports_seek`: false
- `supports_streaming`: true
- `max_concurrent_connections**: 16

### 7. DLNA/UPnP
**Protocol**: DLNA/UPnP AV
**Use Cases**: Media servers, smart devices
**Authentication**: None
**Features**:
- Device discovery
- Content browsing
- Streaming
- Rendering

**Capabilities**:
- `supports_range`: false
- `supports_seek`: false
- `supports_streaming`: true
- `max_concurrent_connections**: 8

## Source Capabilities

### 1. Connection Management
**Connection Pooling**: Reuse connections
**Connection Limits**: Per-server limits
**Connection Timeout**: Configurable timeouts
**Keep-Alive**: Connection keep-alive

### 2. Authentication
**Credential Storage**: Secure storage
**Credential Rotation**: Periodic rotation
**Multi-Factor**: Support for MFA
**Token Refresh**: Automatic token refresh

### 3. Error Handling
**Retry Logic**: Exponential backoff
**Circuit Breaker**: Failure isolation
**Fallback**: Alternative sources
**Graceful Degradation**: Reduced functionality

### 4. Performance Optimization
**Caching**: Local caching
**Prefetching**: Predictive prefetching
**Parallel Transfers**: Multi-segment transfers
**Bandwidth Management**: Rate limiting

## Missing Environment Handling

### 1. BLOCKED_ENV Status
**Condition**: Real network environment not available
**Behavior**: Mark as BLOCKED_ENV in Provider Matrix
**Evidence**: Document missing environment
**Next Steps**: Provide environment setup instructions

### 2. Not Runtime PASS
**Rule**: Missing environment cannot be RUNTIME_PASS
**Alternative**: CONTRACT_PASS with evidence
**Documentation**: Document what would be tested
**Verification**: Test with mock environment

### 3. Environment Requirements
**Minimum Requirements**: Network connectivity
**Optional Requirements**: Authentication credentials
**Test Requirements**: Test media files
**Documentation Requirements**: Setup instructions

## Privacy and Redaction

### 1. Server Address Redaction
**Rule**: No internal IP addresses or hostnames
**Method**: Redact server addresses
**Example**: `192.168.1.100` → `***.***.***.***`

### 2. Credential Redaction
**Rule**: No tokens, passwords, or credentials
**Method**: Remove or mask sensitive fields
**Example**: `password=secret` → `password=***`

### 3. Path Redaction
**Rule**: No full file paths
**Method**: Use relative paths or redacted names
**Example**: `/share/videos/movie.mp4` → `movie.mp4`

## Gate Integration

### NET-001 Gate Requirements
1. Network source types defined
2. Source capabilities documented
3. Missing environment handling defined
4. Privacy/redaction rules enforced
5. Error handling strategies defined

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks network source definitions
- All network sources must have capability documentation
- Missing environments must be BLOCKED_ENV

### PMX-001 Gate Integration
- Provider Runtime Matrix includes network source support
- Each network source must declare capabilities
- Missing environments must be documented

## Test Cases

### Positive Test Cases
1. Connect to SMB share with valid credentials → Successful connection
2. Connect to NFS share with valid authentication → Successful connection
3. Connect to FTP server with valid login → Successful connection
4. Connect to HTTP media URL with range request → Successful read
5. Connect to DLNA device with discovery → Successful discovery
6. Handle network timeout with retry → Successful retry

### Negative Test Cases
1. Connect to SMB share with invalid credentials → Graceful failure
2. Connect to NFS share with network timeout → Graceful failure
3. Connect to FTP server with permission denied → Graceful failure
4. Connect to HTTP URL with server error → Graceful failure
5. Connect to DLNA device with no response → Graceful failure
6. Handle missing network environment → BLOCKED_ENV status

## Implementation Requirements

### Protocol Implementation
1. Implement protocol-specific clients
2. Handle protocol version differences
3. Support protocol extensions
4. Handle protocol errors

### Connection Management
1. Connection pooling
2. Connection health monitoring
3. Connection recovery
4. Connection cleanup

### Error Handling
1. Protocol-specific error mapping
2. Retry logic implementation
3. Circuit breaker pattern
4. Graceful degradation