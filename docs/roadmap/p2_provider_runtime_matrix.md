# P2-PMX-001 Provider Runtime Matrix Generation

## Overview

Provider Runtime Matrix defines the comprehensive status tracking for all media providers in KivoVideo. This matrix tracks contract status, runtime verification status, environment requirements, and evidence references for each provider. The matrix ensures honest and accurate representation of provider capabilities.

## Provider Runtime Matrix Schema

### Matrix Structure

```cpp
struct ProviderRuntimeMatrix {
    std::string schema_version;           // Matrix schema version
    std::string generation_timestamp;     // Matrix generation timestamp
    std::string generator_version;        // Generator version
    std::string project_version;          // Project version
    std::vector<ProviderEntry> providers; // Provider entries
    MatrixMetadata metadata;              // Matrix metadata
};
```

### Provider Entry

```cpp
struct ProviderEntry {
    std::string provider_id;              // Unique provider identifier
    std::string provider_name;            // Human-readable provider name
    ProviderKind provider_kind;           // Provider category
    std::string provider_version;         // Provider version
    ContractStatus contract_status;       // Contract status
    RuntimeStatus runtime_status;         // Runtime verification status
    EnvironmentKind environment_kind;     // Required environment
    bool environment_required;            // Environment required flag
    bool environment_available;           // Environment available flag
    std::string blocked_reason;           // Reason if blocked
    std::string runtime_verified_timestamp; // Runtime verification timestamp
    std::string evidence_file;            // Evidence file path
    ProviderCapability capabilities;      // Provider capabilities
    ProviderLimitations limitations;      // Provider limitations
    ProviderDependencies dependencies;    // Provider dependencies
};
```

### Provider Kinds

1. **local_file**: Local file playback
2. **smb**: SMB network share
3. **nfs**: NFS network share
4. **ftp**: FTP server
5. **ftps**: FTPS secure server
6. **sftp**: SFTP secure server
7. **http_file**: HTTP/HTTPS direct media
8. **https_file**: HTTPS direct media
9. **dlna_upnp**: DLNA/UPnP device
10. **s3_object**: S3-compatible object storage
11. **minio_object**: MinIO object storage
12. **webdav**: WebDAV server
13. **alist**: AList server
14. **emby**: Emby media server
15. **plex**: Plex media server
16. **jellyfin**: Jellyfin media server
17. **google_drive**: Google Drive cloud storage
18. **onedrive**: OneDrive cloud storage
19. **iptv_hls**: IPTV HLS live streaming
20. **airplay_cast**: AirPlay/Cast streaming
21. **trakt_api**: Trakt API integration

### Contract Status

```cpp
enum class ContractStatus {
    CONTRACT_PASS,           // Contract definitions complete
    NOT_IMPLEMENTED,         // Contract not implemented
    FAIL,                    // Contract validation failed
    PARTIAL_IMPLEMENTATION   // Partial implementation
};
```

### Runtime Status

```cpp
enum class RuntimeStatus {
    RUNTIME_PASS,            // Runtime verified with real environment
    CONTRACT_PASS,           // Only contract verified
    BLOCKED_ENV,             // Blocked due to missing environment
    FAIL,                    // Runtime verification failed
    NOT_IMPLEMENTED,         // Runtime not implemented
    SKIPPED_TEST_ONLY        // Skipped for test-only providers
};
```

### Environment Kind

```cpp
enum class EnvironmentKind {
    TEST_HARNESS,            // Test environment only
    LOCAL_HARDWARE,          // Local hardware required
    NETWORK_LOCAL,           // Local network required
    NETWORK_INTERNET,        // Internet required
    EXTERNAL_SERVICE,        // External service required
    HYBRID                   // Combination of environments
};
```

## Provider List (HONEST STATUS - 2026-06-26)

### Local Playback Providers

1. **local_file**: Local file playback
   - Environment: Local hardware (GPU + audio endpoint)
   - Contract: CONTRACT_PASS (real FFmpeg probe/demux/decode proven)
   - Runtime: CONTRACT_PASS (D3D11/WASAPI contracts defined, runtime verification hardware-dependent)
   - RUNTIME_PASS: NO — requires real GPU + audio endpoint + test execution
   - Evidence: Real FFmpeg pipeline proven; D3D11/WASAPI require hardware proof

2. **disc_bdmv**: Disc image/BDMV playback
   - Environment: Local hardware + disc drive
   - Contract: NOT_IMPLEMENTED
   - Runtime: NOT_IMPLEMENTED
   - RUNTIME_PASS: NO

### Network File Providers

3. **smb**: SMB network share
   - Environment: Local network
   - Runtime: SMB client, authentication
   - Status: NOT_IMPLEMENTED

4. **nfs**: NFS network share
   - Environment: Local network
   - Runtime: NFS client, mounting
   - Status: NOT_IMPLEMENTED

5. **ftp**: FTP server
   - Environment: Network
   - Runtime: FTP client, authentication
   - Status: NOT_IMPLEMENTED

6. **ftps**: FTPS secure server
   - Environment: Network
   - Runtime: FTPS client, TLS
   - Status: NOT_IMPLEMENTED

7. **sftp**: SFTP secure server
   - Environment: Network
   - Runtime: SFTP client, SSH
   - Status: NOT_IMPLEMENTED

8. **http_file**: HTTP/HTTPS direct media
   - Environment: Internet
   - Runtime: HTTP client, range requests
   - Status: NOT_IMPLEMENTED

9. **https_file**: HTTPS direct media
   - Environment: Internet
   - Runtime: HTTPS client, TLS
   - Status: NOT_IMPLEMENTED

10. **dlna_upnp**: DLNA/UPnP device
    - Environment: Local network
    - Runtime: DLNA/UPnP discovery, streaming
    - Status: NOT_IMPLEMENTED

### Object Storage Providers

11. **s3_object**: S3-compatible object storage
    - Environment: Internet
    - Runtime: S3 client, authentication
    - Status: NOT_IMPLEMENTED

12. **minio_object**: MinIO object storage
    - Environment: Local network or internet
    - Runtime: MinIO client, authentication
    - Status: NOT_IMPLEMENTED

### WebDAV/AList Providers

13. **webdav**: WebDAV server
    - Environment: Network
    - Runtime: WebDAV client, PROPFIND/GET
    - Status: NOT_IMPLEMENTED

14. **alist**: AList server
    - Environment: Network
    - Runtime: AList API, direct links
    - Status: NOT_IMPLEMENTED

### Media Server Providers

15. **emby**: Emby media server
    - Environment: Network
    - Runtime: Emby API, session management
    - Status: NOT_IMPLEMENTED

16. **plex**: Plex media server
    - Environment: Network
    - Runtime: Plex API, session management
    - Status: NOT_IMPLEMENTED

17. **jellyfin**: Jellyfin media server
    - Environment: Network
    - Runtime: Jellyfin API, session management
    - Status: NOT_IMPLEMENTED

### Cloud Storage Providers

18. **google_drive**: Google Drive cloud storage
    - Environment: Internet
    - Runtime: Google Drive API, OAuth
    - Status: NOT_IMPLEMENTED

19. **onedrive**: OneDrive cloud storage
    - Environment: Internet
    - Runtime: OneDrive API, OAuth
    - Status: NOT_IMPLEMENTED

### Live Streaming Providers

20. **iptv_hls**: IPTV HLS live streaming
    - Environment: Internet
    - Runtime: HLS client, m3u8 parsing
    - Status: NOT_IMPLEMENTED

### Streaming Protocol Providers

21. **airplay_cast**: AirPlay/Cast streaming
    - Environment: Local network
    - Runtime: Cast protocol, device discovery
    - Status: NOT_IMPLEMENTED

### Integration Providers

22. **trakt_api**: Trakt API integration
    - Environment: Internet
    - Runtime: Trakt API, OAuth
    - Status: NOT_IMPLEMENTED

## Matrix Generation Rules

### Generation Process

1. **Discovery**: Discover all available providers
2. **Contract Check**: Verify contract definitions
3. **Environment Check**: Check environment availability
4. **Runtime Verification**: Verify runtime capabilities
5. **Evidence Collection**: Collect evidence for each provider
6. **Status Assignment**: Assign honest status to each provider
7. **Matrix Assembly**: Assemble provider runtime matrix
8. **Validation**: Validate matrix completeness and accuracy

### Status Assignment Rules

1. **Honest Status**: Assign status based on actual verification
2. **No Default PASS**: Never default to RUNTIME_PASS
3. **Environment Check**: BLOCKED_ENV if environment unavailable
4. **Evidence Required**: Evidence required for RUNTIME_PASS
5. **Contract Minimum**: CONTRACT_PASS minimum for defined providers

### Evidence Requirements

1. **Evidence File**: Each provider must have evidence file
2. **Schema Validation**: Evidence must validate against schema
3. **Timestamp**: Evidence must have verification timestamp
4. **Environment**: Evidence must specify environment kind
5. **Status Justification**: Evidence must justify status assignment

## Matrix Metadata

### Matrix Metadata Structure

```cpp
struct MatrixMetadata {
    std::string generation_command;       // Command that generated matrix
    std::string generator_version;        // Generator version
    std::string project_version;          // Project version
    std::string branch;                   // Git branch
    std::string commit_hash;              // Git commit hash
    std::string build_configuration;      // Build configuration
    std::string runtime_environment;      // Runtime environment description
    std::string verification_timestamp;   // Verification timestamp
    int total_providers;                  // Total provider count
    int runtime_pass_count;              // RUNTIME_PASS count
    int contract_pass_count;             // CONTRACT_PASS count
    int blocked_env_count;               // BLOCKED_ENV count
    int fail_count;                      // FAIL count
    int not_implemented_count;           // NOT_IMPLEMENTED count
};
```

### Matrix Statistics

1. **Provider Counts**: Count of providers by status
2. **Environment Counts**: Count of providers by environment
3. **Capability Counts**: Count of providers by capability
4. **Coverage Metrics**: Coverage metrics for provider categories
5. **Verification Metrics**: Metrics for verification completeness

## Matrix Validation

### Validation Rules

1. **Completeness**: All providers must be listed
2. **Consistency**: Status must be consistent with evidence
3. **Accuracy**: Status must accurately reflect verification
4. **Timeliness**: Timestamps must be recent
5. **Evidence**: All RUNTIME_PASS must have evidence

### Validation Checks

1. **Schema Validation**: Validate matrix against schema
2. **Evidence Validation**: Validate evidence files exist
3. **Status Validation**: Validate status assignments
4. **Environment Validation**: Validate environment requirements
5. **Capability Validation**: Validate capability claims

### Validation Errors

1. **Missing Provider**: Provider not listed in matrix
2. **Missing Evidence**: RUNTIME_PASS without evidence
3. **Status Conflict**: Status inconsistent with evidence
4. **Environment Conflict**: Environment requirement mismatch
5. **Capability Conflict**: Capability claim not supported

## Matrix Usage

### Matrix Consumers

1. **PFFG-001**: Foundation Final Gate Reader
2. **PMX-002**: Matrix Honesty Gate
3. **APRV-001**: All Providers Runtime Verification Gate
4. **Inspector**: Playback Inspector
5. **DecisionLedger**: Decision Ledger

### Matrix Queries

1. **Provider Status**: Get status for specific provider
2. **Category Status**: Get status for provider category
3. **Environment Status**: Get status for environment type
4. **Capability Search**: Search providers by capability
5. **Blocked Providers**: Get list of blocked providers

### Matrix Updates

1. **Real-time Updates**: Update matrix on verification changes
2. **Periodic Updates**: Update matrix periodically
3. **Event-driven Updates**: Update matrix on events
4. **Manual Updates**: Allow manual status updates
5. **Version Control**: Version control matrix changes

## Implementation Requirements

### Core Components

1. **ProviderDiscovery**: Discover available providers
2. **ContractVerifier**: Verify contract definitions
3. **EnvironmentChecker**: Check environment availability
4. **RuntimeVerifier**: Verify runtime capabilities
5. **EvidenceCollector**: Collect evidence for providers
6. **StatusAssigner**: Assign honest status to providers
7. **MatrixAssembler**: Assemble provider runtime matrix
8. **MatrixValidator**: Validate matrix completeness
9. **MatrixGenerator**: Generate matrix from verification data
10. **MatrixPersistence**: Store and retrieve matrix

### Integration Points

1. **Provider Registry**: Integration with provider registry
2. **Contract Registry**: Integration with contract registry
3. **Environment Detection**: Integration with environment detection
4. **Evidence Storage**: Integration with evidence storage
5. **Inspector**: Integration with playback inspector
6. **DecisionLedger**: Integration with decision ledger
7. **ErrorMapping**: Integration with error mapping

## Gate Integration

### PMX-002 Matrix Honesty Gate

- Validates matrix honesty
- No default RUNTIME_PASS
- Evidence required for RUNTIME_PASS
- Environment requirements accurate
- Status assignments justified

### PFFG-001 Foundation Final Gate Reader

- Reads provider runtime matrix
- Validates provider completeness
- Validates provider statuses
- Validates evidence references
- Validates matrix metadata

### APRV-001 All Providers Runtime Verification Gate

- Requires all providers RUNTIME_PASS
- Requires all evidence validated
- Requires no BLOCKED_ENV
- Requires no CONTRACT_PASS-only providers
- Requires matrix completeness

## Test Cases

### Positive Test Cases

1. Generate matrix with all 21 providers
2. Validate matrix schema compliance
3. Validate provider status assignments
4. Validate evidence file references
5. Validate environment requirements
6. Validate matrix metadata
7. Validate matrix statistics
8. Validate matrix completeness
9. Validate matrix accuracy
10. Validate matrix timeliness

### Negative Test Cases

1. Missing provider in matrix
2. RUNTIME_PASS without evidence
3. Status conflict with evidence
4. Environment requirement mismatch
5. Capability claim not supported
6. Invalid evidence file reference
7. Missing matrix metadata
8. Invalid matrix statistics
9. Incomplete matrix coverage
10. Outdated matrix timestamps

## Documentation Files

1. Matrix Schema: `docs/p2/provider_runtime_matrix_schema.md`
2. Provider List: `docs/p2/provider_list.md`
3. Generation Rules: `docs/p2/matrix_generation_rules.md`
4. Validation Rules: `docs/p2/matrix_validation_rules.md`
5. Usage Guidelines: `docs/p2/matrix_usage_guidelines.md`

## Evidence Output

- Evidence file: `artifacts/p2/evidence/PMX-001.json`
- Matrix file: `artifacts/p2/provider_runtime_matrix.json`
- Inspector output: Matrix generation snapshot
- DecisionLedger output: Matrix generation decision
- Schema validation: P2-evidence-v8 schema
- Redline scan: No forbidden terms in matrix