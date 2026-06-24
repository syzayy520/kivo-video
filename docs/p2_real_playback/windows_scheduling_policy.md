# P2-014 Windows Scheduling / MMCSS Policy

Updated: 2026-06-25

## Goal

Implement scheduling policy for media playback threads using Windows MMCSS (Multimedia Class Scheduler Service).

## MMCSS Integration

MMCSS ensures media playback threads receive priority scheduling to prevent glitches. The policy should:
1. Register media threads with MMCSS
2. Set appropriate priority levels
3. Handle registration failures gracefully
4. Provide fallback scheduling when MMCSS unavailable

## Scheduling Policy Contract

```cpp
struct SchedulingPolicyConfig {
    ContractMetadata metadata;
    std::string policy_id;
    std::string task_category{"Playback"};
    int priority_level{2};  // 0-10, higher is better
    bool enable_mmcss{true};
    bool fallback_on_failure{true};
};

struct SchedulingPolicyStatus {
    ContractMetadata metadata;
    std::string policy_id;
    bool mmcss_registered{false};
    bool mmcss_available{true};
    int current_priority{0};
    std::string last_error;
};
```

## Fake Scheduling Service

Provides deterministic scheduling policy results for testing:
- `build_default_scheduling_config()` - returns default config
- `build_mmcss_enabled_config()` - config with MMCSS enabled
- `build_mmcss_disabled_config()` - config with MMCSS disabled
- `build_registered_status()` - status showing successful registration
- `build_failed_status()` - status showing registration failure

## Tests

1. Default config values
2. Custom config values
3. MMCSS enabled/disabled config
4. Registered status
5. Failed status
6. Priority level validation