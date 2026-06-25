# FFmpeg Adapter Boundary Contract

## Purpose

The FFmpeg Adapter Boundary ensures that all FFmpeg library types and API calls are isolated behind a single translation unit (`third_party_adapter/ffmpeg_adapter/`), preventing FFmpeg dependencies from leaking into business core headers.

## Boundary Rules

### 1. Type Isolation
- `AVFormatContext`, `AVCodecContext`, `AVPacket`, `AVFrame` must **never** appear in `include/kivo/cinema_engine/` headers (except in forward declarations within `ffmpeg_adapter.hpp`)
- Business code uses `void*` opaque pointers to hold FFmpeg contexts

### 2. Header Isolation
- FFmpeg headers (`<libavformat/avformat.h>`, `<libavcodec/avcodec.h>`, etc.) are **only** included in `ffmpeg_adapter.cpp`
- Business code includes only `ffmpeg_adapter.hpp`

### 3. API Isolation
- All FFmpeg API calls go through `FfmpegAdapter` static methods
- Business code never calls `av_*` functions directly

## FfmpegAdapter API

### File Operations
```cpp
static AVFormatContext* open_file(const std::string& path);
static void free_format_context(AVFormatContext* format_context);
```

### Probe Operations
```cpp
static bool probe_container(AVFormatContext* format_context, 
                           std::string& container_format,
                           double& duration,
                           int64_t& bitrate_estimate);

static bool enumerate_streams(AVFormatContext* format_context,
                             std::vector<int>& video_stream_indices,
                             std::vector<int>& audio_stream_indices,
                             std::vector<int>& subtitle_stream_indices);
```

### Decode Operations
```cpp
static AVCodecContext* create_codec_context(AVFormatContext* format_context, int stream_index);
static bool decode_packet(AVCodecContext* codec_context, AVPacket* packet, AVFrame* frame);
static void free_codec_context(AVCodecContext* codec_context);
static void free_packet(AVPacket* packet);
static void free_frame(AVFrame* frame);
```

### Utility Operations
```cpp
static std::string map_error(int ffmpeg_error);
static std::string get_codec_name(int codec_id);
static std::string get_pixel_format_name(int pixel_format);
static std::string get_sample_format_name(int sample_format);
static std::string get_channel_layout_description(uint64_t channel_layout);
```

## Conditional Compilation

When `KIVO_ENABLE_FFMPEG` is **not** defined:
- All methods return safe defaults (nullptr, false, empty strings)
- No FFmpeg headers are included
- No FFmpeg symbols are referenced

When `KIVO_ENABLE_FFMPEG` **is** defined:
- Full FFmpeg implementations are compiled
- FFmpeg headers are included via `extern "C"` block
- All FFmpeg API calls are made through adapter methods

## Verification

To verify no FFmpeg tokens leak into business code:

```powershell
# Search for forbidden tokens in business headers
rg "AVFormatContext|AVCodecContext|AVPacket|AVFrame" include/kivo/cinema_engine/ --glob "!**/third_party_adapter/**"
# Should return 0 matches

# Search for FFmpeg header includes in business code
rg "#include.*libav" include/kivo/cinema_engine/ --glob "!**/third_party_adapter/**"
# Should return 0 matches
```

## Migration Guide

When adding new FFmpeg functionality:

1. **Add adapter method** in `ffmpeg_adapter.hpp` (declaration) and `ffmpeg_adapter.cpp` (implementation)
2. **Guard with `#ifdef KIVO_ENABLE_FFMPEG`** in the .cpp file
3. **Provide stub** in the `#else` branch
4. **Use `void*`** in business code to hold the context
5. **Never include FFmpeg headers** in business code
