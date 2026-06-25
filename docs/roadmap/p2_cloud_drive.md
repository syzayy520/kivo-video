# P2 Cloud Drive

Updated: 2026-06-26

This document defines the cloud drive system for P2, covering Google Drive and OneDrive integration with OAuth authentication and comprehensive file operations.

## Purpose

The cloud drive system provides access to files stored on cloud drives (Google Drive, OneDrive) with comprehensive OAuth authentication, file operations, and privacy protection.

## Cloud Drive Protocol

### 1. OAuth Profiles

#### Google Drive OAuth Profile
**Protocol**: OAuth 2.0
**Use Cases**: Personal cloud storage, file sharing
**Authentication**: OAuth 2.0 with refresh tokens
**Features**:
- File listing and navigation
- File download and upload
- File sharing and permissions
- Quota management

#### OneDrive OAuth Profile
**Protocol**: OAuth 2.0
**Use Cases**: Personal cloud storage, file sharing
**Authentication**: OAuth 2.0 with refresh tokens
**Features**:
- File listing and navigation
- File download and upload
- File sharing and permissions
- Quota management

### 2. OAuth Authentication

#### OAuth Flow
**Process**: Authorization code flow
**Steps**:
1. Redirect user to authorization endpoint
2. User grants permission
3. Receive authorization code
4. Exchange code for access token
5. Refresh token when expired

#### Token Management
**Access Token Lifetime**: Typically 1 hour
**Refresh Token Lifetime**: Typically 30 days
**Refresh Process**: Automatic using refresh token
**Revocation**: Manual user action

### 3. File Operations

#### Listing and Navigation
**Methods**: List directory contents
**Pagination**: Page token based
**Sorting**: Name, modified date, size
**Filtering**: File type, trashed status

#### File Metadata
**Standard Fields**: ID, name, size, modified date, MIME type
**Cloud-Specific**: Shared status, permissions, version
**User Data**: Starred status, last viewed date
**Sync**: Cross-device synchronization

### 4. Download Operations

#### Download URL Resolution
**Process**: Get temporary download URL
**URL Lifetime**: Typically 1-24 hours
**Parameters**: File ID, export format (for Google Docs)
**Validation**: URL accessibility check

#### Range Requests
**Support**: HTTP Range header
**Use Cases**: Partial content download, seeking
**Limitations**: Cloud-specific chunk sizes
**Retry**: Automatic on failure

### 5. Upload Operations

#### Simple Upload
**Process**: Upload file content directly
**Size Limit**: Typically 5MB
**MIME Type**: Auto-detected or specified
**Overwrite**: Option to overwrite existing

#### Resumable Upload
**Process**: Upload in chunks with resume capability
**Chunk Size**: Configurable (default 1MB)
**Resume**: Automatic on failure
**Progress**: Track upload progress

### 6. Quota and Rate Limits

#### Quota Management
**Storage Quota**: Total storage available
**Used Quota**: Current usage
**File Quota**: Per-file size limit
**Request Quota**: API request limits

#### Rate Limiting
**Requests per Second**: Cloud-specific limits
**Burst Limits**: Temporary higher limits
**Retry Strategy**: Exponential backoff
**Error Handling**: 429 Too Many Requests

### 7. Link Management

#### Link Generation
**Process**: Generate shareable link
**Permissions**: View, comment, edit
**Expiration**: Optional link expiration
**Password Protection**: Optional password

#### Link Refresh
**Process**: Refresh expiring links
**Trigger**: Link expiration within threshold
**Method**: Generate new link
**Validation**: Verify new link works

### 8. Caching Strategy

#### Local Caching
**Metadata Cache**: Cache file metadata
**Content Cache**: Cache downloaded content
**Cache Expiration**: Time-based expiration
**Cache Invalidation**: On file change

#### Cache Policy
**Read Policy**: Cache-first, network fallback
**Write Policy**: Write-through, write-back
**Eviction Policy**: LRU, size-based
**Consistency**: Strong vs eventual

### 9. Error Handling

#### Authentication Errors
**Invalid Token**: Token expired or invalid
**Insufficient Scope**: Missing required permissions
**Rate Limiting**: Too many requests
**Server Error**: Internal server error

#### File Errors
**Not Found**: File doesn't exist
**Permission Denied**: Insufficient permissions
**Quota Exceeded**: Storage quota exceeded
**Conflict**: File conflict or version mismatch

#### Network Errors
**Timeout**: Connection timeout
**DNS**: DNS resolution failed
**SSL**: SSL/TLS handshake failure
**Proxy**: Proxy connection error

### 10. Privacy and Redaction

#### Token Redaction
**Rule**: No access tokens or refresh tokens
**Method**: Remove or mask credentials
**Example**: `access_token=ya29...` → `access_token=***`

#### File Path Redaction
**Rule**: No full file paths
**Method**: Use redacted paths
**Example**: `/drive/folder/file.txt` → `file.txt`

#### Email Redaction
**Rule**: No email addresses
**Method**: Anonymize email addresses
**Example**: `user@example.com` → `***@example.com`

#### Quota Redaction
**Rule**: No exact quota numbers
**Method**: Use percentage or ranges
**Example**: `15.2GB used` → `75% used`

## Gate Integration

### CLD-001 Gate Requirements
1. OAuth profiles defined (Google Drive, OneDrive)
2. OAuth flow documented
3. File operations defined
4. Download/upload operations documented
5. Quota/rate limits defined
6. Link management documented
7. Caching strategy defined
8. Error handling strategies defined
9. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks cloud drive definitions
- All operations must have error handling
- All credentials must be redacted
- Privacy compliance required

### PMX-001 Gate Integration
- Provider Runtime Matrix includes cloud drive support
- Each drive must declare capabilities
- Authentication status must be tracked

## Test Cases

### Positive Test Cases
1. Authenticate with valid OAuth credentials → Successful authentication
2. List directory with valid token → Successful listing
3. Download file with valid ID → Successful download
4. Upload file with valid token → Successful upload
5. Generate shareable link with valid permissions → Successful link
6. Refresh expiring link → Successful refresh

### Negative Test Cases
1. Authenticate with expired token → Graceful failure
2. List directory with insufficient permissions → Graceful failure
3. Download file with invalid ID → Graceful failure
4. Upload file exceeding quota → Graceful failure
5. Generate link without permissions → Graceful failure
6. Refresh link with network error → Graceful failure

## Implementation Requirements

### OAuth Client
1. Implement OAuth flow
2. Handle token refresh
3. Manage token storage
4. Handle token revocation

### File Operations
1. Implement file listing
2. Handle metadata
3. Support pagination
4. Handle sorting/filtering

### Download/Upload
1. Implement download with range support
2. Handle resumable uploads
3. Track progress
4. Handle errors

### Caching
1. Implement metadata caching
2. Handle content caching
3. Manage cache expiration
4. Handle cache invalidation