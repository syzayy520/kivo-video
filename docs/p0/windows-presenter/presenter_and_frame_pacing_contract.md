# Presenter And Frame Pacing Contract

VideoPresenterContract records how frames should be presented under the current display and graph conditions.

## Required Decisions

- presenter backend
- vsync source
- refresh rate
- content frame rate
- frame pacing mode
- clock master
- late frame policy
- drop/duplicate policy
- jitter budget
- zero-copy state
- optional copy-back reason
- expected pacing quality
- optional fallback reason

## Rule

24p and 23.976 content must have a frame pacing decision. Zero-copy to copy-back fallback must have a reason.

