# P2-TRK-001 Trakt Foundation

## Overview

Trakt foundation defines the architectural contracts for Trakt integration in KivoVideo. Trakt is a media tracking service that allows users to track their movie and TV show watching habits. This foundation establishes the authentication, identity mapping, tracking, synchronization, and privacy contracts.

## OAuth Profile

### OAuth Configuration

```cpp
struct TraktOAuthProfile {
    std::string client_id;           // Trakt API client ID
    std::string client_secret;       // Trakt API client secret
    std::string redirect_uri;        // OAuth redirect URI
    std::string api_base_url;        // Trakt API base URL
    std::string auth_base_url;       // Trakt authentication base URL
    std::string api_version;         // Trakt API version
    std::vector<std::string> scopes; // OAuth scopes
    bool use_pkce;                   // Use PKCE for OAuth
    int token_expiry_buffer;         // Token expiry buffer in seconds
};
```

### OAuth Scopes

1. **Public**: Basic profile information
2. **History**: Access to watch history
3. **Watchlist**: Access to watchlist
4. **Ratings**: Access to ratings
5. **Comments**: Access to comments
6. **Lists**: Access to custom lists
7. **Recommendations**: Access to recommendations
8. **Discover**: Access to discover features

### OAuth Flow

1. **Authorization**: User authorizes application
2. **Token Exchange**: Exchange authorization code for tokens
3. **Token Refresh**: Refresh expired access tokens
4. **Token Revocation**: Revoke tokens on logout
5. **Token Storage**: Secure token storage

### Token Management

```cpp
struct TraktToken {
    std::string access_token;        // Access token
    std::string refresh_token;       // Refresh token
    std::string token_type;          // Token type (Bearer)
    int expires_in;                  // Token expiry in seconds
    std::string scope;               // Granted scopes
    std::string created_at;          // Token creation timestamp
    std::string username;            // Associated username
};
```

### Token Security

1. **Secure Storage**: Tokens stored in encrypted storage
2. **Token Rotation**: Regular token rotation
3. **Token Revocation**: Immediate token revocation on logout
4. **Token Validation**: Validate tokens before use
5. **Token Expiry**: Handle token expiry gracefully

## Movie/Episode Identity Mapping

### Movie Identity

```cpp
struct TraktMovieIdentity {
    std::string trakt_id;            // Trakt movie ID
    std::string title;               // Movie title
    int year;                        // Release year
    std::string imdb_id;             // IMDB ID
    std::string tmdb_id;             // TMDB ID
    std::string slug;                // Trakt slug
    std::string overview;            // Movie overview
    std::string released;            // Release date
    std::string runtime;             // Runtime in minutes
    std::vector<std::string> genres; // Movie genres
    std::string rating;              // Trakt rating
    std::string votes;               // Trakt votes
};
```

### Episode Identity

```cpp
struct TraktEpisodeIdentity {
    std::string trakt_id;            // Trakt episode ID
    std::string show_trakt_id;       // Trakt show ID
    std::string title;               // Episode title
    int season;                      // Season number
    int episode;                     // Episode number
    std::string first_aired;         // First air date
    std::string overview;            // Episode overview
    std::string runtime;             // Runtime in minutes
    std::string rating;              // Trakt rating
    std::string votes;               // Trakt votes
};
```

### Identity Mapping Rules

1. **IMDB Mapping**: Map by IMDB ID when available
2. **TMDB Mapping**: Map by TMDB ID when IMDB unavailable
3. **Title/Year Mapping**: Map by title and year as fallback
4. **Fuzzy Matching**: Allow fuzzy matching for title variations
5. **Manual Override**: Allow manual identity mapping

### Identity Resolution

1. **Automatic Resolution**: System automatically resolves identity
2. **Confidence Score**: Assign confidence score to matches
3. **User Confirmation**: Low-confidence matches require user confirmation
4. **Manual Match**: User can manually match identities
5. **Match History**: Store match history for future reference

## Manual Match

### Manual Match Interface

```cpp
class TraktManualMatcher {
public:
    // Search for potential matches
    virtual std::vector<TraktSearchResult> search(
        const std::string& title,
        int year,
        MediaType type
    ) = 0;
    
    // Get match details
    virtual TraktMatchDetails get_match_details(
        const std::string& trakt_id
    ) = 0;
    
    // Confirm match
    virtual bool confirm_match(
        const std::string& local_id,
        const std::string& trakt_id
    ) = 0;
    
    // Remove match
    virtual bool remove_match(
        const std::string& local_id
    ) = 0;
    
    // Get match history
    virtual std::vector<TraktMatchHistory> get_match_history() = 0;
};
```

### Match Confidence

1. **High Confidence**: Exact title + year + IMDB/TMDB match
2. **Medium Confidence**: Title + year match, no external ID
3. **Low Confidence**: Title match only
4. **No Match**: No suitable match found
5. **Conflict**: Multiple potential matches

### Match Rules

1. **One-to-One**: One local item maps to one Trakt item
2. **Persistent**: Matches persist across sessions
3. **Override**: User can override automatic matches
4. **Bulk Match**: Support for bulk matching
5. **Export/Import**: Match data exportable/importable

## Watched/Collection Tracking

### Watched Status

```cpp
struct TraktWatchedStatus {
    std::string trakt_id;            // Trakt item ID
    bool watched;                    // Watched status
    std::string watched_at;          // Watch timestamp
    int plays;                       // Number of plays
    std::string last_played_at;      // Last played timestamp
    std::string progress;            // Progress percentage
    bool fully_watched;              // Fully watched flag
};
```

### Collection Status

```cpp
struct TraktCollectionStatus {
    std::string trakt_id;            // Trakt item ID
    bool collected;                  // Collection status
    std::string collected_at;        // Collection timestamp
    std::string media_type;          // Media type (digital, physical, etc.)
    std::string resolution;          // Resolution (HD, 4K, etc.)
    std::string audio_codec;         // Audio codec
    std::string video_codec;         // Video codec
    std::string edition;             // Edition (theatrical, director's cut, etc.)
};
```

### Tracking Rules

1. **Automatic Tracking**: Track automatically on playback
2. **Manual Tracking**: Allow manual status updates
3. **Progress Tracking**: Track viewing progress
4. **Multi-Episode**: Track multi-episode viewing
5. **Rewatch**: Track rewatches separately

### Sync Frequency

1. **Real-time**: Sync on playback events
2. **Periodic**: Sync every 5 minutes
3. **On-demand**: Sync on user request
4. **Background**: Sync in background
5. **Conflict Resolution**: Handle sync conflicts

## Scrobble Start/Pause/Stop

### Scrobble Events

```cpp
struct TraktScrobbleEvent {
    std::string event_type;          // start, pause, stop, scrobble
    std::string trakt_id;            // Trakt item ID
    std::string media_type;          // movie, episode
    int progress;                    // Progress percentage
    int duration;                    // Duration in seconds
    int position;                    // Current position in seconds
    std::string app_version;         // Application version
    std::string app_date;            // Application date
};
```

### Scrobble Rules

1. **Start**: Send start event when playback begins
2. **Pause**: Send pause event when playback pauses
3. **Resume**: Send start event when playback resumes
4. **Stop**: Send stop event when playback stops
5. **Scrobble**: Send scrobble event at 80% completion

### Scrobble Timing

1. **Start Delay**: 30 second delay before start event
2. **Pause Delay**: 30 second delay before pause event
3. **Scrobble Threshold**: 80% completion threshold
4. **Minimum Duration**: 5 minute minimum for scrobble
5. **Maximum Gap**: 30 minute maximum gap between events

### Scrobble Error Handling

1. **Retry**: Retry failed scrobbles
2. **Queue**: Queue scrobbles for later
3. **Conflict**: Handle scrobble conflicts
4. **Offline**: Handle offline scrobbles
5. **Rate Limit**: Handle rate limiting

## Progress/Resume Sync

### Progress Sync

```cpp
struct TraktProgressSync {
    std::string trakt_id;            // Trakt item ID
    int progress;                    // Progress percentage
    int position;                    // Position in seconds
    int duration;                    // Duration in seconds
    std::string last_synced;         // Last sync timestamp
    bool synced;                     // Sync status
};
```

### Resume Sync

```cpp
struct TraktResumeSync {
    std::string trakt_id;            // Trakt item ID
    int resume_position;             // Resume position in seconds
    std::string resume_timestamp;    // Resume timestamp
    bool resume_available;           // Resume available flag
    std::string device;              // Device name
    std::string app_version;         // App version
};
```

### Sync Rules

1. **Bidirectional Sync**: Sync progress both ways
2. **Latest Wins**: Most recent progress wins
3. **Device Tracking**: Track which device last played
4. **Conflict Resolution**: Handle sync conflicts
5. **Offline Support**: Support offline progress tracking

### Sync Frequency

1. **Real-time**: Sync on playback events
2. **Periodic**: Sync every 5 minutes
3. **On-demand**: Sync on user request
4. **Background**: Sync in background
5. **Startup**: Sync on application start

## Offline Queue

### Offline Queue Structure

```cpp
struct TraktOfflineQueue {
    std::queue<TraktScrobbleEvent> scrobble_queue;
    std::queue<TraktProgressSync> progress_queue;
    std::queue<TraktCollectionSync> collection_queue;
    std::queue<TraktWatchlistSync> watchlist_queue;
    std::queue<TraktRatingSync> rating_queue;
    std::string last_synced;         // Last sync timestamp
    bool sync_in_progress;           // Sync in progress flag
};
```

### Queue Rules

1. **Priority**: Scrobble events have highest priority
2. **Order**: Maintain event order within queue
3. **Deduplication**: Remove duplicate events
4. **Expiry**: Expire old queue entries (7 days)
5. **Limit**: Maximum queue size (1000 events)

### Queue Management

1. **Add**: Add events to queue
2. **Remove**: Remove processed events
3. **Clear**: Clear queue on logout
4. **Export**: Export queue for backup
5. **Import**: Import queue from backup

### Sync Strategy

1. **Automatic Sync**: Sync when network available
2. **Manual Sync**: Sync on user request
3. **Background Sync**: Sync in background
4. **Batch Sync**: Batch multiple events
5. **Conflict Resolution**: Handle sync conflicts

## Retry Strategy

### Retry Configuration

```cpp
struct TraktRetryConfig {
    int max_retries;                 // Maximum retry attempts
    int retry_delay_ms;              // Initial retry delay in milliseconds
    int max_retry_delay_ms;          // Maximum retry delay in milliseconds
    float backoff_factor;            // Exponential backoff factor
    bool retry_on_network_error;     // Retry on network errors
    bool retry_on_rate_limit;        // Retry on rate limiting
    bool retry_on_server_error;      // Retry on server errors
    int rate_limit_delay_ms;         // Rate limit retry delay
};
```

### Retry Rules

1. **Exponential Backoff**: Use exponential backoff for retries
2. **Maximum Retries**: Maximum 3 retry attempts
3. **Jitter**: Add jitter to retry delays
4. **Circuit Breaker**: Stop retries after consecutive failures
5. **Fallback**: Use cached data when retries exhausted

### Error Classification

1. **Network Errors**: Retry with backoff
2. **Rate Limiting**: Retry with delay
3. **Server Errors**: Retry with backoff
4. **Client Errors**: Do not retry
5. **Authentication Errors**: Do not retry, refresh token

### Retry Monitoring

1. **Retry Count**: Track retry attempts
2. **Success Rate**: Track retry success rate
3. **Failure Reasons**: Track failure reasons
4. **Performance Impact**: Monitor performance impact
5. **User Notification**: Notify user of persistent failures

## Privacy Rules

### Privacy Contract

```cpp
struct TraktPrivacyContract {
    bool share_watched;              // Share watched status
    bool share_rating;               // Share ratings
    bool share_watchlist;            // Share watchlist
    bool share_collection;           // Share collection
    bool share_comments;             // Share comments
    bool share_recommendations;      // Share recommendations
    bool allow_analytics;            // Allow analytics
    bool allow_crash_reports;        // Allow crash reports
    std::string privacy_level;       // Privacy level (public, friends, private)
};
```

### Privacy Controls

1. **Profile Visibility**: Control profile visibility
2. **Activity Sharing**: Control activity sharing
3. **Data Retention**: Control data retention
4. **Data Export**: Export personal data
5. **Data Deletion**: Delete personal data

### Data Minimization

1. **Collect Minimum**: Collect only necessary data
2. **Purpose Limit**: Use data only for stated purpose
3. **Storage Limit**: Store data only as long as needed
4. **Access Control**: Limit access to personal data
5. **Anonymization**: Anonymize data when possible

### Privacy Settings

1. **Public Profile**: Make profile public or private
2. **Activity Feed**: Control activity feed visibility
3. **Watch History**: Control watch history visibility
4. **Ratings**: Control ratings visibility
5. **Recommendations**: Control recommendations visibility

## Conflict Policy

### Conflict Types

1. **Status Conflict**: Watched status conflict
2. **Progress Conflict**: Progress sync conflict
3. **Collection Conflict**: Collection status conflict
4. **Rating Conflict**: Rating conflict
5. **Watchlist Conflict**: Watchlist conflict

### Conflict Resolution

```cpp
struct TraktConflictResolution {
    std::string conflict_type;       // Conflict type
    std::string resolution_strategy; // Resolution strategy
    bool user_override;              // User can override
    std::string fallback_strategy;   // Fallback strategy
    int resolution_timeout;          // Resolution timeout in seconds
};
```

### Resolution Strategies

1. **Latest Wins**: Most recent change wins
2. **User Choice**: User chooses resolution
3. **Server Wins**: Server state wins
4. **Client Wins**: Client state wins
5. **Merge**: Merge changes when possible

### Conflict Prevention

1. **Optimistic Locking**: Use optimistic locking
2. **Version Tracking**: Track version numbers
3. **Timestamp Comparison**: Use timestamps for ordering
4. **Device Identification**: Identify devices for conflict
5. **User Notification**: Notify user of conflicts

## Error Handling

### Error Types

```cpp
enum class TraktError {
    None,
    AuthenticationFailed,
    TokenExpired,
    TokenRevoked,
    NetworkError,
    RateLimited,
    ServerError,
    NotFound,
    Forbidden,
    Conflict,
    ValidationFailed,
    Offline,
    Timeout,
    Unknown
};
```

### Error Recovery

1. **Authentication Errors**: Refresh token or re-authenticate
2. **Network Errors**: Retry with backoff
3. **Rate Limiting**: Wait and retry
4. **Server Errors**: Retry with backoff
5. **Conflict Errors**: Resolve conflict
6. **Offline Errors**: Queue for later

### Error Reporting

1. **User Notification**: Notify user of errors
2. **Error Logging**: Log errors for debugging
3. **Error Analytics**: Track error patterns
4. **Error Recovery**: Attempt automatic recovery
5. **Fallback**: Use cached data on error

## Implementation Requirements

### Core Components

1. **TraktAuthManager**: OAuth authentication management
2. **TraktIdentityMapper**: Movie/episode identity mapping
3. **TraktManualMatcher**: Manual match interface
4. **TraktWatchedTracker**: Watched status tracking
5. **TraktCollectionTracker**: Collection status tracking
6. **TraktScrobbleManager**: Scrobble event management
7. **TraktProgressSyncManager**: Progress synchronization
8. **TraktOfflineQueue**: Offline queue management
9. **TraktRetryManager**: Retry strategy management
10. **TraktPrivacyManager**: Privacy controls
11. **TraktConflictResolver**: Conflict resolution
12. **TraktErrorHandler**: Error handling and recovery

### Integration Points

1. **Playback Engine**: Integration with playback engine
2. **Media Library**: Integration with media library
3. **User Preferences**: Integration with user preferences
4. **Network Layer**: Integration with network layer
5. **Storage**: Integration with local storage
6. **Inspector**: Integration with playback inspector
7. **DecisionLedger**: Integration with decision ledger
8. **ErrorMapping**: Integration with error mapping

## Gate Integration

### PFFG-001 Foundation Final Gate Reader

- Reads Trakt foundation definitions
- Validates OAuth profile completeness
- Validates identity mapping contracts
- Validates tracking contracts
- Validates synchronization contracts
- Validates privacy contracts

### PMX-001 Provider Runtime Matrix

- Trakt foundation status: `CONTRACT_PASS`
- Trakt implementation status: `NOT_IMPLEMENTED`
- Trakt environment requirement: `true` (requires network)
- Trakt runtime verification: Not required for foundation

## Test Cases

### Positive Test Cases

1. OAuth authentication flow
2. Token refresh and revocation
3. Movie identity mapping by IMDB ID
4. Episode identity mapping by TMDB ID
5. Manual match search and confirmation
6. Watched status tracking
7. Collection status tracking
8. Scrobble start/pause/stop events
9. Progress synchronization
10. Resume synchronization
11. Offline queue management
12. Retry strategy execution
13. Privacy settings application
14. Conflict resolution
15. Error handling and recovery

### Negative Test Cases

1. Invalid OAuth credentials
2. Token expiration handling
3. Network connectivity loss
4. Rate limiting handling
5. Server error handling
6. Identity mapping conflict
7. Manual match rejection
8. Offline queue overflow
9. Retry limit exceeded
10. Privacy violation detection
11. Conflict resolution failure
12. Error state recovery failure

## Documentation Files

1. OAuth Profile: `docs/p2/trakt_oauth_profile.md`
2. Identity Mapping: `docs/p2/trakt_identity_mapping.md`
3. Manual Match: `docs/p2/trakt_manual_match.md`
4. Watched/Collection: `docs/p2/trakt_watched_collection.md`
5. Scrobble: `docs/p2/trakt_scrobble.md`
6. Progress Sync: `docs/p2/trakt_progress_sync.md`
7. Offline Queue: `docs/p2/trakt_offline_queue.md`
8. Retry Strategy: `docs/p2/trakt_retry_strategy.md`
9. Privacy Rules: `docs/p2/trakt_privacy_rules.md`
10. Conflict Policy: `docs/p2/trakt_conflict_policy.md`

## Evidence Output

- Evidence file: `artifacts/p2/evidence/TRK-001.json`
- Inspector output: Trakt foundation snapshot
- DecisionLedger output: Trakt foundation decision
- Schema validation: P2-evidence-v8 schema
- Redline scan: No forbidden terms in implementation