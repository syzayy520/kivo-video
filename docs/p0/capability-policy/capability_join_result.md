# Capability Join Result

CapabilityCore freezes capability as layers and exposes only the joined result to planning.

## Layers

- StaticCapabilitySnapshot
- DeviceCapabilitySnapshot
- SessionCapabilitySnapshot
- RuntimeCapabilityObservation

## Join Result

`CapabilityJoinResult` contains:

- refs to the four layers
- conflicts
- missing capabilities
- confidence
- expiry
- trace id

## Rule

PlaybackPlan depends on `CapabilityJoinResult`, never on raw capability layers.

