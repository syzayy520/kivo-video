# P2-004 Capability Join / Playback Eligibility

Updated: 2026-06-25

## Goal

Implement the capability join service that determines playback eligibility by joining static device capabilities, session capabilities, and runtime observations into a `CapabilityJoinResult`.

## Design

### Capability Join Process

1. **Static Capability**: Device hardware capabilities (GPU, audio output, display)
2. **Session Capability**: Session-specific settings (user preferences, feature flags)
3. **Runtime Observation**: Current runtime state (memory pressure, thermal, battery)
4. **Join**: Combine all capability sources into a single `CapabilityJoinResult`
5. **Eligibility**: Determine if playback is possible and at what confidence level

### Eligibility Rules

| Rule | Condition | Result |
|------|-----------|--------|
| Video decode capable | GPU supports codec | Required |
| Audio output capable | Audio endpoint available | Required |
| Display capable | Display connected | Required |
| Memory sufficient | Available memory > threshold | Required |
| Thermal safe | Temperature < limit | Warning |
| Battery safe | Battery > threshold (mobile) | Warning |

### Confidence Levels

| Confidence | Meaning |
|------------|---------|
| 1.0 | All capabilities present, no warnings |
| 0.8 | All required capabilities present, some warnings |
| 0.5 | Missing optional capabilities |
| 0.0 | Missing required capabilities |

## Implementation Plan

1. Create `capability_core/playback_eligibility_service.hpp` — Service interface
2. Create `src/capability_core/fake_playback_eligibility_service.cpp` — Fake implementation
3. Create `tests/capability_core/playback_eligibility_test.cpp` — Tests
4. Update `backend/CMakeLists.txt` with new source and test
5. Update governance gate to verify eligibility service

## Verification

P2-004 is verified when:
1. Fake eligibility service can produce CapabilityJoinResult
2. Tests cover all eligibility rules
3. Governance gate passes
4. CTest passes
