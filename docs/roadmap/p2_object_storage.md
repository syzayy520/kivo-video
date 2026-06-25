# P2 Object Storage

Updated: 2026-06-26

This document defines the object storage system for P2, covering S3-compatible and MinIO endpoints with comprehensive error handling and privacy protection.

## Purpose

The object storage system provides access to media files stored in S3-compatible and MinIO object storage services with appropriate authentication, error handling, and privacy protection.

## Object Storage Types

### 1. S3-Compatible Storage
**Protocol**: AWS S3 API
**Use Cases**: Cloud storage, media archives
**Authentication**: AWS Signature V4
**Features**:
- Bucket/object operations
- Range requests
- Multipart uploads
- Versioning

**Capabilities**:
- `supports_range`: true
- `supports_seek`: true
- `supports_streaming`: true
- `max_concurrent_connections`: 32

### 2. MinIO
**Protocol**: S3-compatible API
**Use Cases**: Self-hosted object storage
**Authentication**: AWS Signature V4
**Features**:
- S3-compatible API
- Erasure coding
- Versioning
- Lifecycle policies

**Capabilities**:
- `supports_range`: true
- `supports_seek`: true
- `supports_streaming`: true
- `max_concurrent_connections`: 32

## Endpoint and Object Identity

### 1. Endpoint Identity
**Fields**:
- `endpoint_url`: Service endpoint URL
- `region`: Service region
- `bucket_name`: Bucket name
- `path_style`: Path style or virtual-hosted style

**Redaction Rules**:
- No internal endpoints in logs
- Redact endpoint URLs in Inspector

### 2. Object Identity
**Fields**:
- `object_key`: Object key/path
- `object_size`: Object size in bytes
- `object_etag`: Object ETag
- `last_modified`: Last modification time

**Redaction Rules**:
- No full object paths in logs
- Redact object keys in Inspector

## Operations

### 1. HEAD Object
**Purpose**: Get object metadata without downloading
**Response Headers**:
- `Content-Length`: Object size
- `Content-Type`: Object MIME type
- `ETag`: Object ETag
- `Last-Modified`: Last modification time

**Use Cases**:
- Check object existence
- Get object size
- Verify object unchanged

### 2. GET Object
**Purpose**: Download object or range
**Parameters**:
- `Range`: Byte range (optional)
- `If-None-Match`: ETag conditional
- `If-Modified-Since`: Time conditional

**Use Cases**:
- Download full object
- Download partial object
- Conditional download

### 3. Range Requests
**Purpose**: Download object in parts
**Range Format**: `bytes=start-end`
**Examples**:
- `bytes=0-1023`: First 1024 bytes
- `bytes=1024-`: From byte 1024 to end
- `bytes=-1024`: Last 1024 bytes

**Use Cases**:
- Resumable downloads
- Partial content access
- Parallel downloads

## Metadata

### 1. ETag
**Purpose**: Object integrity check
**Format**: MD5 hash or multipart ETag
**Use Cases**:
- Verify object unchanged
- Cache validation
- Upload verification

### 2. Last-Modified
**Purpose**: Object modification time
**Format**: ISO 8601 timestamp
**Use Cases**:
- Cache freshness
- Change detection
- Version comparison

### 3. Content-Type
**Purpose**: Object MIME type
**Format**: Standard MIME type
**Use Cases**:
- Content handling
- Format detection
- Player selection

## Error Handling

### 1. Permission Errors
**Error Types**:
- `AccessDenied`: No permission to access
- `Forbidden`: Authentication failed
- `InvalidAccessKey`: Invalid credentials

**Handling**:
- Check credentials
- Verify permissions
- Request access

### 2. Not Found Errors
**Error Types**:
- `NoSuchBucket`: Bucket doesn't exist
- `NoSuchKey`: Object doesn't exist
- `NotFound`: Resource not found

**Handling**:
- Verify bucket name
- Verify object key
- Check path

### 3. Throttling Errors
**Error Types**:
- `Throttling`: Rate limit exceeded
- `RequestLimitExceeded`: Request quota exceeded
- `SlowDown`: Too many requests

**Handling**:
- Implement backoff
- Reduce request rate
- Use request queuing

### 4. TLS Errors
**Error Types**:
- `SSLHandshakeFailed`: TLS handshake failed
- `CertificateExpired`: Certificate expired
- `CertificateUntrusted`: Certificate not trusted

**Handling**:
- Verify TLS version
- Check certificate validity
- Use trusted certificates

## Credential Management

### 1. Access Key/Secret Key
**Storage**: Secure credential storage
**Rotation**: Periodic rotation
**Redaction**: Never log credentials

### 2. Session Tokens
**Purpose**: Temporary credentials
**Expiration**: Time-limited
**Refresh**: Automatic refresh

### 3. Presigned URLs
**Purpose**: Temporary access URLs
**Expiration**: Time-limited
**Redaction**: No URL logging

## Privacy and Redaction

### 1. Endpoint Redaction
**Rule**: No internal endpoints in logs
**Method**: Redact endpoint URLs
**Example**: `https://s3.amazonaws.com` → `https://***.amazonaws.com`

### 2. Credential Redaction
**Rule**: No access keys or secret keys
**Method**: Remove or mask credentials
**Example**: `AKIAIOSFODNN7EXAMPLE` → `***`

### 3. Object Key Redaction
**Rule**: No full object paths
**Method**: Use redacted object keys
**Example**: `/bucket/path/to/movie.mp4` → `movie.mp4`

## Gate Integration

### OBJ-001 Gate Requirements
1. Object storage types defined
2. Endpoint/object identity documented
3. Operations documented
4. Error handling defined
5. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks object storage definitions
- All storage types must have error handling
- All credentials must be redacted

### PMX-001 Gate Integration
- Provider Runtime Matrix includes object storage support
- Each storage type must declare capabilities
- Error handling must be documented

## Test Cases

### Positive Test Cases
1. HEAD object with valid credentials → Successful metadata retrieval
2. GET object with range request → Successful partial download
3. GET object with ETag conditional → Successful conditional download
4. Handle throttling with backoff → Successful retry
5. Handle permission error with credential refresh → Successful access
6. Redact credentials in logs → Privacy compliant

### Negative Test Cases
1. HEAD object with invalid credentials → Graceful failure
2. GET object with invalid range → Graceful failure
3. GET non-existent object → Graceful failure
4. Handle TLS certificate error → Graceful failure
5. Handle bucket not found → Graceful failure
6. Log credentials without redaction → Privacy violation

## Implementation Requirements

### S3 Client
1. Implement S3 API operations
2. Handle authentication
3. Support range requests
4. Handle errors

### Credential Management
1. Secure credential storage
2. Credential rotation
3. Session token handling
4. Presigned URL generation

### Error Handling
1. Error classification
2. Retry logic
3. Backoff implementation
4. Error reporting