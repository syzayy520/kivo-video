# P1 Audio And Subtitle Services

These services provide dedicated P1 generation paths for subtitle and audio contracts.

## Services

- SubtitleCore: `build_fake_subtitle_render_plan`.
- AudioCore: `build_fake_audio_output_decision`.

## Rules

- Subtitle embedded font fallback carries a reason-capable policy.
- HDR subtitle luminance is policy-driven.
- Audio passthrough is selected only when requested and not blocked by compliance.
- Passthrough fallback produces a user-visible hint and fallback reason.

