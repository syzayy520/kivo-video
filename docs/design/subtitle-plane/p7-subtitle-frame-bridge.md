# P7 Subtitle Frame Bridge

P7 reads real subtitle frame snapshots from P8 through `SubtitleFrameP8Bridge`.

Flow:

P8 `build_frame` + `copy_text_buffer` → P7 `query_subtitle_frame()` → adapter subtitle frame mapper.

The bridge attaches when subtitle track selection enables subtitles and detaches on disable/close.