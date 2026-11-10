#pragma once

#include <cstddef>
#include <cstdint>

namespace kivo::video::core::decode {

/// 一次读包的结果。
enum class SampleReadStatus : std::int32_t {
    kOk = 0,      ///< 读到一包，view 有效
    kEndOfStream, ///< 文件读完，不是错误
    kFailed,      ///< 读失败
    kTryAgain,    ///< 异步读取尚未就绪；没有消费或丢弃样本
};

/// 从解封装器借来的样本视图。生命周期只到下一次 read_next_sample 调用。
///
/// 用视图而非拷贝：解码路径上每帧多一次 memcpy 会吃掉可观带宽，
/// 而 AMediaCodec 的输入缓冲本来就要再拷一次，没必要拷两遍。
///
/// 标志位是**中立语义**，不是平台常量。这不是洁癖，是真实撞车：
///   AMediaExtractor: SYNC=1, ENCRYPTED=2
///   AMediaCodec    : KEY_FRAME=1, CODEC_CONFIG=2, END_OF_STREAM=4, PARTIAL_FRAME=8
/// 把提取器的 ENCRYPTED(2) 直接透传给解码器，等于告诉它「这是配置包，别输出帧」，
/// 表现是画面突然丢帧却毫无报错。必须由两侧各自翻译。
struct SampleView {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::int64_t presentation_time_us = 0;
    bool is_sync = false;      ///< 关键帧
    bool is_encrypted = false; ///< 加密样本，当前不支持，必须显式拒绝
};

}  // namespace kivo::video::core::decode
