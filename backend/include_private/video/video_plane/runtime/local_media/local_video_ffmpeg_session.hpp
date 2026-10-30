#pragma once

#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "video/source_runtime/local_media/local_media_source_session.hpp"
#endif

namespace kivo::video::video_plane::runtime::local_media {

#ifdef KIVO_ENABLE_FFMPEG
struct LocalVideoRuntime::FfmpegSession {
    kivo::video::source_runtime::local_media::LocalMediaSourceSession source{};
    kivo::cinema_engine::FfmpegFormatHandle format_handle{};
    kivo::cinema_engine::RealSoftwareDecodeRuntime decoder{};
};
#else
struct LocalVideoRuntime::FfmpegSession {};
#endif

}  // namespace kivo::video::video_plane::runtime::local_media
