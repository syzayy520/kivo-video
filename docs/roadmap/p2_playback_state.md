# P2 Playback State

Updated: 2026-06-26

This document defines the playback state system for P2, covering playback sessions, progress tracking, and state synchronization.

## Purpose

The playback state system manages playback sessions, tracks progress, and synchronizes state across devices, providing a seamless playback experience with resume capabilities.

## Playback State System

### 1. Playback Session Identity

#### Session Identity
**Fields**: Session ID, user ID, device ID, media ID
**Uniqueness**: Globally unique session identifier
**Lifetime**: Created on playback start, destroyed on session end
**Persistence**: Optional persistence across app restarts

#### Session Lifecycle
**States**: Created → Active → Paused → Ended → Archived
**Transitions**: Based on user actions and system events
**Timeout**: Automatic timeout after inactivity
**Cleanup**: Resource cleanup on session end

### 2. Progress Tracking

#### Progress Snapshot
**Fields**: Current position, duration, playback speed
**Update Frequency**: Every second or on significant change
**Storage**: Local storage with optional cloud sync
**Accuracy**: Millisecond precision

#### Progress Calculation
**Current Position**: Time since playback start
**Duration**: Total media duration
**Percentage**: Current position / duration
**Remaining**: Duration - current position

### 3. Resume Point Management

#### Resume Point
**Fields**: Position, timestamp, device ID
**Creation**: Automatic on pause or stop
**Storage**: Persistent storage
**Retrieval**: On playback start

#### Resume Logic
**New Session**: Check for existing resume point
**Resume Prompt**: Ask user whether to resume
**Skip Option**: Start from beginning
**Multiple Points**: Support multiple resume points per media

### 4. Recent Playback

#### Recent List
**Fields**: Media ID, last played timestamp, progress
**Capacity**: Last 50 items
**Sorting**: By last played timestamp
**Persistence**: Persistent storage

#### Recent Management
**Add**: On playback start
**Update**: On progress update
**Remove**: On manual removal
**Purge**: Automatic purge when list full

### 5. Playback Queue

#### Queue Structure
**Fields**: Media list, current index, shuffle state
**Order**: Sequential or shuffled
**Navigation**: Next, previous, jump to index
**Persistence**: Optional persistence

#### Queue Operations
**Add**: Add media to queue
**Remove**: Remove media from queue
**Reorder**: Change media order
**Clear**: Clear entire queue

### 6. Repeat and Shuffle Modes

#### Repeat Modes
**Off**: Play once, then stop
**All**: Repeat entire queue
**One**: Repeat current media
**State**: Persistent across sessions

#### Shuffle Modes
**Off**: Sequential playback
**On**: Random order
**Seed**: Random seed for reproducibility
**State**: Persistent across sessions

### 7. Watched Threshold

#### Threshold Definition
**Default**: 90% of media duration
**Configurable**: User-adjustable threshold
**Minimum**: 10 seconds or 5%
**Maximum**: 95% of media duration

#### Watched Logic
**Automatic**: Mark as watched when threshold reached
**Manual**: Allow manual watched/unwatched marking
**Override**: User can override automatic marking
**Sync**: Sync watched status across devices

### 8. Local History

#### History Structure
**Fields**: Media ID, timestamps, durations, devices
**Capacity**: Last 1000 items
**Granularity**: Per-play session
**Persistence**: Persistent storage

#### History Management
**Record**: On each playback session
**Query**: By media, date, duration
**Export**: Export history data
**Delete**: Delete history entries

### 9. Non-Blocking Sync

#### Sync Process
**Upload**: Local changes to cloud
**Download**: Cloud changes to local
**Conflict**: Resolve conflicts automatically
**Queue**: Queue changes when offline

#### Sync Strategy
**Real-time**: Immediate sync on change
**Batch**: Periodic batch sync
**Background**: Background sync
**Manual**: User-triggered sync

### 10. State Persistence

#### Local Persistence
**Storage**: Local database or files
**Encryption**: Optional encryption
**Backup**: Automatic backup
**Restore**: Restore from backup

#### Cloud Persistence
**Storage**: Cloud storage service
**Authentication**: Required for sync
**Conflict Resolution**: Last-write-wins
**Privacy**: Encrypted in transit and at rest

### 11. Error Handling

#### Sync Errors
**Network**: Handle network failures
**Conflict**: Resolve sync conflicts
**Quota**: Handle storage quota exceeded
**Auth**: Handle authentication failures

#### State Errors
**Corruption**: Detect and recover from corruption
**Loss**: Recover from data loss
**Timeout**: Handle sync timeouts
**Partial**: Handle partial sync

### 12. Privacy and Redaction

#### Playback Data Redaction
**Rule**: No detailed playback patterns
**Method**: Anonymize playback data
**Example**: `watched_at=2026-06-26T12:00:00Z` → `watched_date=2026-06-26`

#### Device ID Redaction
**Rule**: No specific device identifiers
**Method**: Use generic device references
**Example**: `device_id=abc123` → `device_id=***`

#### User Data Redaction
**Rule**: No personal viewing data
**Method**: Anonymize user data
**Example**: `user_id=12345` → `user_id=***`

#### History Redaction
**Rule**: No detailed viewing history
**Method**: Aggregate or anonymize
**Example**: `viewing_history=[...]` → `recent_count=5`

## Gate Integration

### PST-001 Gate Requirements
1. Playback session identity defined
2. Progress tracking documented
3. Resume point management defined
4. Recent playback system documented
5. Playback queue system defined
6. Repeat/shuffle modes defined
7. Watched threshold documented
8. Local history system defined
9. Non-blocking sync documented
10. State persistence documented
11. Error handling strategies defined
12. Privacy/redaction rules enforced

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks playback state definitions
- All operations must have error handling
- All credentials must be redacted
- Privacy compliance required

### PMX-001 Gate Integration
- Provider Runtime Matrix includes playback state support
- Each provider must declare state capabilities
- State sync status must be tracked

## Test Cases

### Positive Test Cases
1. Create playback session with valid media → Successful creation
2. Track progress with millisecond precision → Successful tracking
3. Save resume point on pause → Successful save
4. Add media to queue → Successful addition
5. Switch repeat mode → Successful switch
6. Sync state across devices → Successful sync

### Negative Test Cases
1. Create session with invalid media → Graceful failure
2. Track progress with network error → Graceful handling
3. Save resume point with storage full → Graceful failure
4. Add media to full queue → Graceful handling
5. Sync with authentication failure → Graceful retry
6. Recover from corrupted state → Graceful recovery

## Implementation Requirements

### Session Manager
1. Create and manage sessions
2. Handle session lifecycle
3. Persist session state
4. Clean up resources

### Progress Tracker
1. Track playback progress
2. Calculate progress metrics
3. Update progress frequently
4. Persist progress data

### Queue Manager
1. Manage playback queue
2. Handle queue operations
3. Support repeat/shuffle
4. Persist queue state

### Sync Manager
1. Sync state across devices
2. Handle conflicts
3. Queue offline changes
4. Manage sync errors