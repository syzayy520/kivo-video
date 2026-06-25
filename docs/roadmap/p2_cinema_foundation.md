# P2-CIN-001 Cinema Foundation

## Overview

Cinema foundation defines the architectural contracts for Cinema mode in KivoVideo. Cinema mode provides an immersive viewing experience by hiding UI elements and maximizing video display area. This foundation establishes the state model, conflict policies, window management, and user experience contracts.

## Cinema Mode State Model

### State Machine States

```cpp
enum class CinemaState {
    Normal,             // Normal mode with UI visible
    Fullscreen,         // Fullscreen mode (UI hidden, video fills screen)
    Cinema,             // Cinema mode (UI hidden, video with safe areas)
    PiP,                // Picture-in-Picture mode (small floating window)
    Cast,               // Cast mode (video on external device)
    Transitioning       // Transitioning between states
};
```

### State Transitions

1. **Normal → Fullscreen**: User enters fullscreen mode
2. **Normal → Cinema**: User enters cinema mode
3. **Normal → PiP**: User enters PiP mode
4. **Normal → Cast**: User enters cast mode
5. **Fullscreen → Normal**: User exits fullscreen mode
6. **Cinema → Normal**: User exits cinema mode
7. **PiP → Normal**: User exits PiP mode
8. **Cast → Normal**: User exits cast mode
9. **Any → Transitioning**: State transition in progress
10. **Transitioning → Any**: Transition completed

### State Properties

- **UI Visibility**: UI hidden in Fullscreen/Cinema/PiP/Cast modes
- **Video Scaling**: Video scales to fill available area
- **Audio Routing**: Audio routes to appropriate output device
- **Input Handling**: Input events handled differently per state
- **Performance**: Different performance profiles per state

## Conflict Policy

### Conflict Resolution Matrix

| Current State | Requested State | Resolution |
|---------------|-----------------|------------|
| Normal | Fullscreen | Allow |
| Normal | Cinema | Allow |
| Normal | PiP | Allow |
| Normal | Cast | Allow |
| Fullscreen | Cinema | Transition to Cinema |
| Fullscreen | PiP | Transition to PiP |
| Fullscreen | Cast | Transition to Cast |
| Cinema | Fullscreen | Transition to Fullscreen |
| Cinema | PiP | Transition to PiP |
| Cinema | Cast | Transition to Cast |
| PiP | Fullscreen | Transition to Fullscreen |
| PiP | Cinema | Transition to Cinema |
| PiP | Cast | Transition to Cast |
| Cast | Fullscreen | Transition to Fullscreen |
| Cast | Cinema | Transition to Cinema |
| Cast | PiP | Transition to PiP |

### Conflict Rules

1. **Priority Order**: Fullscreen > Cinema > PiP > Cast > Normal
2. **User Preference**: User can override priority order
3. **Hardware Limitation**: Some transitions blocked by hardware
4. **Performance**: High-resolution video may block PiP transition
5. **Network**: Cast mode requires network connection
6. **Battery**: PiP mode may be blocked on low battery

### Conflict Resolution Types

1. **Automatic**: System automatically resolves conflicts
2. **User Choice**: User chooses which mode to keep
3. **Fail Safe**: Default to Normal mode on conflict
4. **Priority Based**: Higher priority mode wins
5. **Resource Based**: Mode with more resources wins

## Enter/Exit Behavior

### Enter Cinema Mode

```cpp
struct EnterCinemaRequest {
    CinemaState target_state;  // Fullscreen, Cinema, PiP, Cast
    bool preserve_playback;
    bool preserve_audio;
    bool preserve_subtitles;
    bool preserve_position;
    int monitor_id;           // Target monitor
    bool hide_cursor;
    bool hide_osd;
    bool hide_subtitles;
};

struct EnterCinemaResult {
    bool success;
    CinemaState actual_state;
    std::string error_message;
    float transition_duration_ms;
};
```

### Exit Cinema Mode

```cpp
struct ExitCinemaRequest {
    CinemaState from_state;
    bool restore_playback;
    bool restore_audio;
    bool restore_subtitles;
    bool restore_position;
    bool restore_cursor;
    bool restore_osd;
};

struct ExitCinemaResult {
    bool success;
    CinemaState actual_state;
    std::string error_message;
    float transition_duration_ms;
};
```

### Enter/Exit Rules

1. **Animation**: Smooth transition animations between states
2. **Input Block**: Block input during transition
3. **Audio Fade**: Fade audio during transition
4. **Video Scaling**: Scale video during transition
5. **State Persistence**: Save state before transition
6. **Error Recovery**: Recover to previous state on failure

## Chrome/Cursor/OSD/Subtitle Safe Areas

### Safe Area Definitions

```cpp
struct SafeArea {
    int top;                 // Top safe area in pixels
    int bottom;              // Bottom safe area in pixels
    int left;                // Left safe area in pixels
    int right;               // Right safe area in pixels
    int title_safe;          // Title safe area (80% of screen)
    int action_safe;         // Action safe area (90% of screen)
};

struct CinemaSafeAreas {
    SafeArea video_safe;     // Video content safe area
    SafeArea subtitle_safe;  // Subtitle safe area
    SafeArea osd_safe;       // OSD safe area
    SafeArea cursor_safe;    // Cursor safe area
    SafeArea control_safe;   // Control overlay safe area
};
```

### Chrome Hiding Rules

1. **Title Bar**: Hidden in Fullscreen/Cinema modes
2. **Menu Bar**: Hidden in Fullscreen/Cinema modes
3. **Status Bar**: Hidden in Fullscreen/Cinema modes
4. **Toolbars**: Hidden in Fullscreen/Cinema modes
5. **Scrollbars**: Hidden in Fullscreen/Cinema modes

### Cursor Handling

1. **Auto Hide**: Cursor hides after 3 seconds of inactivity
2. **Show on Movement**: Cursor shows on mouse movement
3. **Custom Cursor**: Cinema mode uses custom cursor
4. **Cursor Size**: Cursor scales with DPI
5. **Cursor Opacity**: Cursor fades out instead of hiding

### OSD Handling

1. **Play/Pause**: Show OSD on play/pause
2. **Volume**: Show OSD on volume change
3. **Seek**: Show OSD on seek
4. **Time**: Show OSD on time display
5. **Status**: Show OSD on status change

### Subtitle Safe Area

1. **Position**: Subtitles positioned in safe area
2. **Margin**: 10% margin from screen edges
3. **Scaling**: Subtitles scale with screen size
4. **Background**: Optional subtitle background
5. **Animation**: Subtitle fade in/out animation

## Playback Continuity

### Continuity Contract

```cpp
struct PlaybackContinuity {
    bool maintain_position;        // Maintain playback position across state changes
    bool maintain_state;           // Maintain play/pause state across state changes
    bool maintain_volume;          // Maintain volume across state changes
    bool maintain_subtitles;       // Maintain subtitle visibility across state changes
    bool maintain_audio_track;     // Maintain audio track across state changes
    bool maintain_video_track;     // Maintain video track across state changes
    ContinuityPrecision precision; // Position precision in milliseconds
};
```

### Continuity Rules

1. **Position Continuity**: Playback position preserved across state changes
2. **State Continuity**: Play/pause state preserved across state changes
3. **Audio Continuity**: Volume and audio track preserved
4. **Subtitle Continuity**: Subtitle visibility preserved
5. **Video Continuity**: Video track preserved
6. **Network Continuity**: Network connection preserved in Cast mode

### Discontinuity Handling

1. **Forced Discontinuity**: Some state changes force discontinuity
2. **User Choice**: User can choose to maintain or reset continuity
3. **Performance**: High-resolution video may cause discontinuity
4. **Network**: Network issues may cause discontinuity in Cast mode
5. **Error**: Errors may cause discontinuity

## Multi-Monitor Support

### Multi-Monitor Cinema

```cpp
struct MultiMonitorCinema {
    int primary_monitor;          // Primary monitor for Cinema mode
    bool extend_to_secondary;     // Extend Cinema to secondary monitor
    bool separate_controls;       // Separate controls on secondary monitor
    bool separate_subtitles;      // Separate subtitles on secondary monitor
    MonitorArrangement arrangement; // Monitor arrangement
};
```

### Monitor Rules

1. **Primary Monitor**: Cinema mode defaults to primary monitor
2. **Secondary Monitor**: Optional extended Cinema on secondary monitor
3. **Monitor Switching**: User can switch Cinema to different monitor
4. **Monitor Arrangement**: Support for horizontal/vertical/diagonal arrangements
5. **DPI Scaling**: Cinema scales correctly across monitors

### Multi-Monitor Features

1. **Extended Cinema**: Video on primary, controls on secondary
2. **Picture-in-Picture**: Video on primary, PiP on secondary
3. **Cast Display**: Video on external display, controls on primary
4. **Monitor Profiles**: Save monitor configurations per user
5. **Auto Switch**: Auto-switch to Cinema monitor when entering Cinema

## DPI Handling

### Cinema DPI Scaling

```cpp
struct CinemaDpi {
    float video_scale_factor;     // Video scaling factor
    float ui_scale_factor;        // UI scaling factor
    float subtitle_scale_factor;  // Subtitle scaling factor
    float osd_scale_factor;       // OSD scaling factor
    bool per_monitor_dpi;         // Per-monitor DPI aware
    bool dpi_aware_video;         // DPI-aware video rendering
};
```

### DPI Rules

1. **Video Scaling**: Video maintains aspect ratio with DPI
2. **UI Scaling**: UI elements scale with DPI
3. **Subtitle Scaling**: Subtitles scale with DPI
4. **OSD Scaling**: OSD scales with DPI
5. **Cursor Scaling**: Cursor scales with DPI
6. **Safe Area Scaling**: Safe areas scale with DPI

### DPI Transitions

1. **Monitor Change**: DPI changes when moving Cinema to different monitor
2. **System Change**: DPI changes when system DPI changes
3. **Application Change**: DPI changes when application DPI changes
4. **Manual Change**: User manually changes DPI settings
5. **Auto Adapt**: Cinema adapts to DPI changes automatically

## Sleep Inhibit

### Sleep Inhibit Contract

```cpp
struct SleepInhibit {
    bool inhibit_sleep;           // Inhibit system sleep during Cinema
    bool inhibit_screen_save;     // Inhibit screen saver during Cinema
    bool inhibit_idle;            // Inhibit idle timeout during Cinema
    bool inhibit_power_save;      // Inhibit power save during Cinema
    InhibitReason reason;         // Reason for inhibit
    int timeout_seconds;          // Inhibit timeout in seconds
};
```

### Sleep Rules

1. **Active Playback**: Inhibit sleep during active playback
2. **Paused Playback**: Inhibit sleep for 5 minutes after pause
3. **Cinema Mode**: Inhibit sleep during Cinema mode
4. **Cast Mode**: Inhibit sleep during Cast mode
5. **PiP Mode**: Inhibit sleep during PiP mode
6. **User Preference**: User can override sleep inhibit

### Power Management

1. **Battery Mode**: Different sleep rules on battery
2. **Plugged In**: Different sleep rules when plugged in
3. **Power Profile**: Respect system power profile
4. **User Override**: User can override power management
5. **Auto Resume**: Auto-resume sleep inhibit when needed

## Error Handling

### Cinema Error Types

```cpp
enum class CinemaError {
    None,
    StateTransitionFailed,
    FullscreenFailed,
    CinemaModeFailed,
    PiPFailed,
    CastFailed,
    SafeAreaCalculationFailed,
    MonitorNotFound,
    DpiScalingFailed,
    SleepInhibitFailed,
    RestoreFailed,
    Timeout,
    Unknown
};
```

### Error Recovery

1. **Automatic Recovery**: Some errors auto-recover to Normal mode
2. **User Notification**: Errors shown to user with recovery options
3. **Fallback**: If Cinema fails, fallback to Normal mode
4. **Logging**: All errors logged for diagnostics
5. **Retry**: Transient errors can be retried automatically

## Privacy and Redaction

### Cinema Privacy Rules

1. **Window Titles**: No sensitive information in Cinema window title
2. **File Paths**: Redact file paths in Cinema UI
3. **Network URLs**: Redact network URLs in Cinema UI
4. **User Data**: No user data in Cinema window properties
5. **Diagnostics**: Redact sensitive info in Cinema diagnostics

### Cinema Redaction Controls

1. **Cinema Screenshot**: User can disable Cinema screenshots
2. **Cinema Recording**: User can disable Cinema recording
3. **Cinema Sharing**: User can disable Cinema sharing
4. **Cinema Overlay**: User can disable Cinema overlay content
5. **Cinema Metadata**: Redact metadata in Cinema window

## Implementation Requirements

### Core Components

1. **CinemaStateManager**: Manages Cinema state machine
2. **ConflictResolver**: Handles mode conflicts
3. **TransitionManager**: Manages state transitions
4. **SafeAreaManager**: Handles safe area calculations
5. **ChromeManager**: Manages UI element visibility
6. **CursorManager**: Handles cursor behavior
7. **OsdManager**: Manages OSD display
8. **SubtitleManager**: Handles subtitle positioning
9. **ContinuityManager**: Manages playback continuity
10. **MonitorManager**: Multi-monitor support
11. **DpiManager**: DPI scaling and handling
12. **SleepManager**: Sleep inhibit management
13. **ErrorHandler**: Cinema error handling and recovery
14. **PrivacyManager**: Privacy and redaction enforcement

### Integration Points

1. **Main Player**: Integration with main player state
2. **Playback Engine**: Integration with playback engine
3. **UI Framework**: Integration with UI framework
4. **Window System**: Integration with window system
5. **Monitor System**: Integration with monitor system
6. **DPI System**: Integration with DPI system
7. **Power Management**: Integration with power management
8. **Preferences**: Integration with user preferences
9. **Inspector**: Integration with playback inspector
10. **DecisionLedger**: Integration with decision ledger
11. **ErrorMapping**: Integration with error mapping

## Gate Integration

### PFFG-001 Foundation Final Gate Reader

- Reads Cinema foundation definitions
- Validates Cinema state model completeness
- Validates conflict policy completeness
- Validates safe area calculations
- Validates sleep inhibit contracts
- Validates error handling contracts

### PMX-001 Provider Runtime Matrix

- Cinema foundation status: `CONTRACT_PASS`
- Cinema implementation status: `NOT_IMPLEMENTED`
- Cinema environment requirement: `false`
- Cinema runtime verification: Not required for foundation

## Test Cases

### Positive Test Cases

1. Cinema state transition Normal → Fullscreen
2. Cinema state transition Normal → Cinema
3. Cinema state transition Normal → PiP
4. Cinema state transition Normal → Cast
5. Cinema state transition Fullscreen → Normal
6. Cinema state transition Cinema → Normal
7. Cinema state transition PiP → Normal
8. Cinema state transition Cast → Normal
9. Conflict resolution between Cinema and PiP
10. Safe area calculation for different screen sizes
11. DPI scaling across different monitors
12. Sleep inhibit during Cinema playback
13. Playback continuity across state changes
14. Multi-monitor Cinema support
15. Chrome hiding in Cinema mode
16. Cursor auto-hide in Cinema mode
17. OSD display in Cinema mode
18. Subtitle positioning in Cinema mode
19. Error recovery from Cinema failure
20. Privacy redaction in Cinema mode

### Negative Test Cases

1. Invalid state transition rejection
2. Conflict resolution with equal priority modes
3. Safe area calculation with invalid screen dimensions
4. DPI scaling with unsupported DPI values
5. Sleep inhibit failure on battery
6. Continuity break during state transition
7. Monitor disconnection during Cinema
8. Cast mode without network connection
9. PiP mode with insufficient resources
10. Fullscreen mode on unsupported monitor
11. Cinema mode with invalid safe areas
12. Error state recovery failure

## Documentation Files

1. Cinema State Model: `docs/p2/cinema_state_model.md`
2. Conflict Policy: `docs/p2/cinema_conflict_policy.md`
3. Enter/Exit Behavior: `docs/p2/cinema_enter_exit.md`
4. Safe Areas: `docs/p2/cinema_safe_areas.md`
5. Playback Continuity: `docs/p2/cinema_playback_continuity.md`
6. Multi-Monitor: `docs/p2/cinema_multi_monitor.md`
7. DPI Handling: `docs/p2/cinema_dpi_handling.md`
8. Sleep Inhibit: `docs/p2/cinema_sleep_inhibit.md`
9. Error Handling: `docs/p2/cinema_error_handling.md`
10. Privacy Rules: `docs/p2/cinema_privacy_rules.md`

## Evidence Output

- Evidence file: `artifacts/p2/evidence/CIN-001.json`
- Inspector output: Cinema foundation snapshot
- DecisionLedger output: Cinema foundation decision
- Schema validation: P2-evidence-v8 schema
- Redline scan: No forbidden terms in implementation