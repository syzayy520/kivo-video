# P2 WebDAV and AList

Updated: 2026-06-26

This document defines the WebDAV and AList systems for P2, covering WebDAV protocol operations and AList cloud storage aggregation.

## Purpose

The WebDAV and AList systems provide access to files stored on WebDAV servers and cloud storage services aggregated through AList, with comprehensive protocol support and error handling.

## WebDAV System

### 1. WebDAV Protocol
**Protocol**: WebDAV (Web Distributed Authoring and Versioning)
**Use Cases**: File sharing, cloud storage, document management
**Authentication**: Basic, Digest, OAuth
**Features**:
- Directory listing
- File upload/download
- File properties
- Locking

### 2. WebDAV Operations

#### PROPFIND
**Purpose**: Get file/directory properties
**Request Headers**:
- `Depth`: 0 (target only), 1 (target and children), infinity (recursive)
- `Content-Type`: application/xml

**Response Properties**:
- `getcontentlength`: File size
- `getlastmodified`: Last modification time
- `resourcetype`: Collection or resource
- `getetag`: Entity tag

#### HEAD
**Purpose**: Get file metadata without download
**Response Headers**:
- `Content-Length`: File size
- `Content-Type`: MIME type
- `ETag`: Entity tag
- `Last-Modified`: Last modification time

#### GET
**Purpose**: Download file
**Parameters**:
- `Range`: Byte range (optional)
- `If-None-Match`: ETag conditional
- `If-Modified-Since`: Time conditional

**Use Cases**:
- Full file download
- Partial content download
- Conditional download

### 3. WebDAV Capabilities
**Range Requests**: Supported via HTTP Range header
**Seek**: Supported via range requests
**Streaming**: Supported via chunked transfer
**Authentication**: Basic, Digest, OAuth

### 4. WebDAV Error Handling
**Common Errors**:
- `401 Unauthorized`: Authentication required
- `403 Forbidden`: Access denied
- `404 Not Found`: Resource not found
- `405 Method Not Allowed`: Operation not supported
- `409 Conflict`: Resource conflict
- `507 Insufficient Storage`: Not enough space

**Handling Strategies**:
- Retry with backoff
- Fallback to alternative methods
- Error reporting

## AList System

### 1. AList Protocol
**Protocol**: AList API
**Use Cases**: Cloud storage aggregation
**Authentication**: Token-based
**Features**:
- Multi-cloud support
- File listing
- Direct links
- Cache management

### 2. AList Operations

#### Profile
**Purpose**: Get server capabilities
**Response Fields**:
- `version`: AList version
- `features`: Supported features
- `drivers`: Available storage drivers

#### List
**Purpose**: List directory contents
**Parameters**:
- `path`: Directory path
- `password`: Directory password (optional)
- `page`: Page number
- `per_page`: Items per page

**Response Fields**:
- `content`: Directory listing
- `total`: Total items
- `readme`: Directory readme

#### Direct Link
**Purpose**: Get direct download link
**Parameters**:
- `path`: File path
- `password`: File password (optional)

**Response Fields**:
- `url`: Direct download URL
- `expires`: Link expiration time
- `headers`: Required headers

### 3. AList Capabilities
**Multi-Cloud**: Supports 20+ cloud providers
**Direct Links**: Temporary download URLs
**Caching**: Server-side caching
**Authentication**: Token-based authentication

### 4. AList Error Handling
**Common Errors**:
- `401 Unauthorized`: Invalid token
- `403 Forbidden`: Access denied
- `404 Not Found`: Resource not found
- `500 Internal Server Error`: Server error
- `502 Bad Gateway`: Upstream error
- `503 Service Unavailable`: Service unavailable

**Handling Strategies**:
- Token refresh
- Retry with backoff
- Fallback to alternative providers

## Source Health

### 1. Health Monitoring
**Metrics**:
- `latency`: Response latency
- `availability`: Uptime percentage
- `error_rate`: Error rate
- `bandwidth`: Available bandwidth

**Monitoring Intervals**:
- Active: Every 30 seconds
- Passive: On each request
- Background: Every 5 minutes

### 2. Health Status
**Status Values**:
- `healthy`: Normal operation
- `degraded`: Reduced performance
- `unhealthy`: Service unavailable
- `unknown`: Status unknown

**Health Checks**:
- Connection test
- Authentication test
- Permission test
- Performance test

## Direct Link Management

### 1. Link Generation
**Process**: Request direct link from server
**Parameters**: File path, expiration, permissions
**Response**: Direct URL, expiration time, headers

### 2. Link Refresh
**Process**: Refresh expiring links
**Trigger**: Link expiration within threshold
**Method**: Request new direct link
**Validation**: Verify new link works

### 3. Link Expiration
**Tracking**: Monitor link expiration times
**Warning**: Alert before expiration
**Auto-refresh**: Automatically refresh links
**Fallback**: Use alternative links

## Privacy and Redaction

### 1. Server URL Redaction
**Rule**: No internal server URLs
**Method**: Redact server addresses
**Example**: `https://dav.example.com` → `https://***.example.com`

### 2. Credential Redaction
**Rule**: No tokens or passwords
**Method**: Remove or mask credentials
**Example**: `token=abc123` → `token=***`

### 3. Path Redaction
**Rule**: No full file paths
**Method**: Use redacted paths
**Example**: `/dav/files/movie.mp4` → `movie.mp4`

## Gate Integration

### WDA-001 Gate Requirements
1. WebDAV operations documented
2. AList operations documented
3. Source health monitoring defined
4. Direct link management defined
5. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks WebDAV/AList definitions
- All operations must have error handling
- All credentials must be redacted

### PMX-001 Gate Integration
- Provider Runtime Matrix includes WebDAV/AList support
- Each provider must declare capabilities
- Source health must be monitored

## Test Cases

### Positive Test Cases
1. PROPFIND directory with valid credentials → Successful listing
2. HEAD file with valid credentials → Successful metadata
3. GET file with range request → Successful partial download
4. AList list directory with valid token → Successful listing
5. AList get direct link with valid token → Successful link
6. Monitor source health with active checks → Successful monitoring

### Negative Test Cases
1. PROPFIND directory with invalid credentials → Graceful failure
2. HEAD file with permission denied → Graceful failure
3. GET file with invalid range → Graceful failure
4. AList list directory with expired token → Graceful failure
5. AList get direct link with invalid path → Graceful failure
6. Monitor source health with service unavailable → Graceful failure

## Implementation Requirements

### WebDAV Client
1. Implement WebDAV operations
2. Handle authentication
3. Support range requests
4. Handle errors

### AList Client
1. Implement AList API operations
2. Handle token authentication
3. Support direct links
4. Handle errors

### Health Monitoring
1. Implement health checks
2. Track health metrics
3. Handle health status changes
4. Report health status