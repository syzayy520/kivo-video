#pragma once

#include <cstdint>
#include <string>

#include <media/NdkMediaFormat.h>

extern "C" {
#include <libavformat/avformat.h>
}

#include "video/video_plane/backends/demux/demux_source.hpp"

namespace kivo::video::demux {

/// 把 FFmpeg 的轨道描述翻译成 MediaCodec 认识的 `AMediaFormat`。
///
/// 只做格式翻译，不解封装、不解码。单独成文件的原因：
/// 这里是整条 FFmpeg 路径最容易出错的地方（csd 形状、mime 名字、时长单位），
/// 出问题时要能一眼定位，不埋在解封装代码里。
struct BuiltTrackFormat {
    AMediaFormat* format = nullptr;  ///< 成功时调用方接管所有权
    VideoTrackInfo video{};
    AudioTrackInfo audio{};
    bool ok = false;
    std::string message;
};

class CodecConfigBuilder {
public:
    /// @param stream 已选定的轨道，必须属于 ctx
    [[nodiscard]] static BuiltTrackFormat build(AVFormatContext* ctx, AVStream* stream,
                                                TrackKind kind);

    /// Android 的 mime 名字。未知编码返回 nullptr —— 宁可显式失败，
    /// 也不猜一个近似值让解码器去崩。
    [[nodiscard]] static const char* android_mime(AVCodecID codec);
};

}  // namespace kivo::video::demux
