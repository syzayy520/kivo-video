# Subtitle Render Plan

SubtitleCore owns subtitle rendering plans. It outputs `SubtitleRenderPlan` and does not mutate playback state.

## Required Decisions

- selected subtitle stream
- subtitle kind
- renderer backend
- font resolution policy
- embedded font policy
- ASS override policy
- HDR luminance policy
- safe area policy
- timing correction
- user offset
- language policy
- forced subtitle policy
- default subtitle policy
- optional fallback reason
- optional user-visible hint

## Rules

- Embedded font fallback must have a reason.
- ASS advanced subtitle fallback must be visible and traceable.
- HDR subtitle luminance must be policy-driven.
- Subtitle render thread must not wait on source or network locks.

