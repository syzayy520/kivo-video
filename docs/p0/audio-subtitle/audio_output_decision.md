# Audio Output Decision

AudioCore owns audio output decisions. UI cannot directly decide passthrough, exclusive mode, shared mode, PCM fallback, or downmix.

## Required Decisions

- selected audio stream
- codec
- channel layout
- sample rate
- optional bit depth
- passthrough requested
- passthrough allowed
- passthrough result
- PCM decode result
- endpoint capability ref
- optional AVR capability ref
- output mode
- sync offset
- optional lipsync policy
- optional fallback reason
- optional user-visible hint

## Rules

- Passthrough fallback must be user-visible.
- Audio endpoint changes must trigger capability update.
- Atmos, DTS, TrueHD, and similar claims come from AudioOutputDecision plus ComplianceGateResult.

