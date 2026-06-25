# P2-PIP-001 Picture-in-Picture Foundation

## Overview

Picture-in-Picture (PiP) foundation defines the architectural contracts for PiP mode in KivoVideo. PiP allows users to watch video in a small floating window while performing other tasks. This foundation establishes the state model, surface management, synchronization, and window management contracts.

## PiP State Model

### State Machine States

```cpp
enum class PipState {
    NotActive,          // PiP mode not active
    Detaching,          // Transitioning from main player to PiP
    Active,             // PiP window active and playing
    Attaching,          // Transitioning from PiP back to main player
    Paused,             // PiP active but paused
    Error               // PiP in error state
};
```

### State Transitions

1. **NotActive → Detaching**: User triggers PiP mode
2. **Detaching → Active**: PiP surface created and playback transferred
3. **Active → Paused**: User pauses in PiP mode
4. **Paused → Active**: User resumes in PiP mode
5. **Active → Attaching**: User returns to main player
6. **Attaching → NotActive**: Playback returned to main player
7. **Any → Error**: PiP operation fails

### State Properties

- **Allowed Transitions**: Each state has defined valid transitions
- **State Persistence**: PiP state survives main window minimization
- **Recovery**: Error state can recover to NotActive
- **Timeout**: Detaching/Attaching states have 5-second timeout

## Surface Management

### Surface Detach Contract

```cpp
struct SurfaceDetachRequest {
    PipSurfaceId surface_id;
    MainWindowHandle main_window;
    PlaybackState transfer_state;
    VideoFormat video_format;
    AudioFormat audio_format;
    bool preserve_playback_position;
};

struct SurfaceDetachResult {
    bool success;
    PipWindowHandle pip_window;
    std::string error_message;
    float detach_duration_ms;
};
```

### Surface Attach Contract

```cpp
struct SurfaceAttachRequest {
    PipSurfaceId surface_id;
    PipWindowHandle pip_window;
    MainWindowHandle main_window;
    bool restore_playback_position;
};

struct SurfaceAttachResult {
    bool success;
    PlaybackState restored_state;
    std::string error_message;
    float attach_duration_ms;
};
```

### Surface Lifecycle

1. **Detach**: Main player → PiP window
2. **Transfer**: Playback state and media content transfer
3. **Render**: Video renders in PiP window
4. **Attach**: PiP window → Main player
5. **Restore**: Playback state restored to main player

## Main Player Synchronization

### Sync Contract

```cpp
struct PipSyncContract {
    bool sync_playback_position;
    bool sync_playback_state;
    bool sync_audio_volume;
    bool sync_subtitle_visibility;
    bool sync_audio_track_selection;
    SyncPrecision precision; // Milliseconds
};
```

### Synchronization Rules

1. **Position Sync**: PiP and main player maintain same playback position
2. **State Sync**: Play/pause state synchronized bidirectionally
3. **Volume Sync**: Audio volume changes propagate to both
4. **Subtitle Sync**: Subtitle visibility synchronized
5. **Track Sync**: Audio track selection synchronized

### Conflict Resolution

- **Main Player Focus**: When main player gains focus, PiP pauses
- **PiP Focus**: When PiP window gains focus, main player pauses
- **User Preference**: User can override sync behavior
- **Performance**: Sync disabled during high CPU usage

## Mini Command Bridge

### Command Interface

```cpp
class PipCommandBridge {
public:
    // Playback control
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(int64_t position_ms) = 0;
    
    // Volume control
    virtual void set_volume(float volume) = 0;
    virtual void mute(bool mute) = 0;
    
    // Subtitle control
    virtual void show_subtitles(bool show) = 0;
    virtual void set_subtitle_track(int track_id) = 0;
    
    // Window control
    virtual void set_window_bounds(int x, int y, int width, int height) = 0;
    virtual void set_opacity(float opacity) = 0;
    virtual void set_always_on_top(bool always_on_top) = 0;
    
    // State query
    virtual PipState get_state() const = 0;
    virtual PlaybackState get_playback_state() const = 0;
    virtual int64_t get_position_ms() const = 0;
    virtual float get_volume() const = 0;
};
```

### Command Bridge Features

1. **Low Latency**: Commands execute within 16ms (60fps)
2. **Thread Safety**: Commands thread-safe from UI and playback threads
3. **Error Handling**: Commands return success/failure with error codes
4. **State Validation**: Commands validate state before execution
5. **Batch Commands**: Support for command batching to reduce IPC overhead

## Window Management

### Window Bounds Contract

```cpp
struct PipWindowBounds {
    int x;                  // Screen X coordinate
    int y;                  // Screen Y coordinate
    int width;              // Window width in pixels
    int height;             // Window height in pixels
    int min_width;          // Minimum width (150px)
    int min_height;         // Minimum height (100px)
    int max_width;          // Maximum width (screen width/2)
    int max_height;         // Maximum height (screen height/2)
    float aspect_ratio;     // Video aspect ratio (16:9, 4:3, etc.)
    bool maintain_aspect;   // Maintain aspect ratio on resize
};
```

### Window Behavior

1. **Default Position**: Bottom-right corner with 20px margin
2. **Snap to Edge**: Window snaps to screen edges
3. **Collision Avoidance**: Window avoids overlapping taskbar
4. **Multi-Monitor**: Window stays on same monitor as main player
5. **DPI Scaling**: Window scales with system DPI settings
6. **Resize**: User can resize with aspect ratio lock

### Window Persistence

- **Position**: Saved to user preferences
- **Size**: Saved to user preferences
- **Opacity**: Saved to user preferences
- **Always on Top**: Saved to user preferences
- **Restore on Launch**: Window restored to last position/size

## Multi-Monitor Support

### Monitor Detection

```cpp
struct MonitorInfo {
    MonitorId id;
    Rect bounds;           // Screen bounds
    Rect work_area;        // Usable area (excluding taskbar)
    float dpi_scale;       // DPI scaling factor
    bool is_primary;       // Primary monitor flag
    std::string device_name; // Monitor device name
};
```

### Multi-Monitor Rules

1. **Primary Monitor**: PiP defaults to primary monitor
2. **Same Monitor**: PiP stays on same monitor as main player
3. **Monitor Switch**: User can drag PiP to other monitors
4. **DPI Handling**: PiP scales correctly across monitors
5. **Resolution Change**: PiP adapts to monitor resolution changes

## DPI Handling

### DPI Scaling

```cpp
struct DpiInfo {
    float system_dpi;      // System DPI (96 = 100%)
    float app_dpi;         // Application DPI
    float scale_factor;    // Scale factor (1.0 = 100%)
    bool per_monitor_dpi;  // Per-monitor DPI aware
    int dpi_awareness;     // DPI awareness level
};
```

### DPI Rules

1. **System DPI**: Use system DPI for default window size
2. **Per-Monitor DPI**: Scale correctly on multi-monitor setups
3. **Text Scaling**: Scale text and UI elements with DPI
4. **Icon Scaling**: Scale icons with DPI
5. **Video Scaling**: Video maintains quality at different DPI

## Restore Behavior

### Restore Contract

```cpp
struct PipRestoreRequest {
    PipWindowHandle pip_window;
    MainWindowHandle main_window;
    bool restore_playback_position;
    bool restore_playback_state;
    bool restore_subtitle_state;
    bool restore_audio_state;
};

struct PipRestoreResult {
    bool success;
    PlaybackState restored_state;
    int64_t restored_position_ms;
    std::string error_message;
    float restore_duration_ms;
};
```

### Restore Rules

1. **Position Restore**: Restore to exact playback position
2. **State Restore**: Restore play/pause state
3. **Subtitle Restore**: Restore subtitle visibility and track
4. **Audio Restore**: Restore volume and audio track
5. **Error Recovery**: If restore fails, return to NotActive state
6. **Timeout**: Restore must complete within 2 seconds

## Error Handling

### Error Types

```cpp
enum class PipError {
    None,
    SurfaceCreationFailed,
    SurfaceDetachFailed,
    SurfaceAttachFailed,
    WindowCreationFailed,
    WindowBoundsInvalid,
    SyncFailed,
    CommandBridgeFailed,
    MonitorNotFound,
    DpiScalingFailed,
    RestoreFailed,
    Timeout,
    Unknown
};
```

### Error Recovery

1. **Automatic Recovery**: Some errors auto-recover to NotActive
2. **User Notification**: Errors shown to user with recovery options
3. **Fallback**: If PiP fails, fallback to main player
4. **Logging**: All errors logged for diagnostics
5. **Retry**: Transient errors can be retried automatically

## Privacy and Redaction

### Redaction Rules

1. **Window Titles**: No sensitive information in PiP window title
2. **File Paths**: Redact file paths in PiP UI
3. **Network URLs**: Redact network URLs in PiP UI
4. **User Data**: No user data in PiP window properties
5. **Diagnostics**: Redact sensitive info in PiP diagnostics

### Privacy Controls

1. **PiP Screenshot**: User can disable PiP screenshots
2. **PiP Recording**: User can disable PiP recording
3. **PiP Sharing**: User can disable PiP sharing
4. **PiP Overlay**: User can disable PiP overlay content
5. **PiP Metadata**: Redact metadata in PiP window

## Implementation Requirements

### Core Components

1. **PipStateManager**: Manages PiP state machine
2. **SurfaceManager**: Handles surface detach/attach
3. **SyncManager**: Manages main player synchronization
4. **CommandBridge**: Mini command bridge for PiP control
5. **WindowManager**: Handles PiP window management
6. **MonitorManager**: Multi-monitor detection and management
7. **DpiManager**: DPI scaling and handling
8. **RestoreManager**: Restore behavior implementation
9. **ErrorHandler**: PiP error handling and recovery
10. **PrivacyManager**: Privacy and redaction enforcement

### Integration Points

1. **Main Player**: Integration with main player state
2. **Playback Engine**: Integration with playback engine
3. **UI Framework**: Integration with UI framework
4. **Window System**: Integration with window system
5. **Monitor System**: Integration with monitor system
6. **DPI System**: Integration with DPI system
7. **Preferences**: Integration with user preferences
8. **Inspector**: Integration with playback inspector
9. **DecisionLedger**: Integration with decision ledger
10. **ErrorMapping**: Integration with error mapping

## Gate Integration

### PFFG-001 Foundation Final Gate Reader

- Reads PiP foundation definitions
- Validates PiP state model completeness
- Validates surface management contracts
- Validates synchronization contracts
- Validates window management contracts
- Validates error handling contracts

### PMX-001 Provider Runtime Matrix

- PiP foundation status: `CONTRACT_PASS`
- PiP implementation status: `NOT_IMPLEMENTED`
- PiP environment requirement: `false`
- PiP runtime verification: Not required for foundation

## Test Cases

### Positive Test Cases

1. PiP state transition NotActive → Detaching → Active
2. PiP state transition Active → Paused → Active
3. PiP state transition Active → Attaching → NotActive
4. Surface detach with successful transfer
5. Surface attach with successful restore
6. Main player synchronization during PiP
7. Command bridge execution in PiP mode
8. Window bounds validation and persistence
9. Multi-monitor PiP window placement
10. DPI scaling across different monitors
11. Restore behavior after PiP exit
12. Error recovery from PiP failure

### Negative Test Cases

1. Invalid state transition rejection
2. Surface detach with invalid window handle
3. Surface attach with invalid surface ID
4. Command bridge timeout handling
5. Window bounds out of screen bounds
6. Monitor disconnection during PiP
7. DPI change during PiP playback
8. Restore timeout exceeded
9. Error state recovery failure
10. Privacy violation in PiP window
11. Redaction failure in diagnostics
12. Conflict with main player focus

## Documentation Files

1. PiP State Model: `docs/p2/pip_state_model.md`
2. Surface Management: `docs/p2/pip_surface_management.md`
3. Synchronization: `docs/p2/pip_synchronization.md`
4. Command Bridge: `docs/p2/pip_command_bridge.md`
5. Window Management: `docs/p2/pip_window_management.md`
6. Multi-Monitor: `docs/p2/pip_multi_monitor.md`
7. DPI Handling: `docs/p2/pip_dpi_handling.md`
8. Restore Behavior: `docs/p2/pip_restore_behavior.md`
9. Error Handling: `docs/p2/pip_error_handling.md`
10. Privacy Rules: `docs/p2/pip_privacy_rules.md`

## Evidence Output

- Evidence file: `artifacts/p2/evidence/PIP-001.json`
- Inspector output: PiP foundation snapshot
- DecisionLedger output: PiP foundation decision
- Schema validation: P2-evidence-v8 schema
- Redline scan: No forbidden terms in implementation