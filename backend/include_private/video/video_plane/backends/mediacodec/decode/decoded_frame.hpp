#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 一张已解出、但**还没交出去**的帧。
///
/// 为什么把"收"和"呈现"拆成两步：`AMediaCodec_releaseOutputBuffer(codec, index, render)`
/// 的第三个参数是唯一能表达"呈现"还是"丢弃"的开关。原先 `receive()` 内部直接
/// `render=true`，会话层就再也没有机会按节拍丢帧 —— 而那正是 P12-C5 要的东西。
///
/// `index` 是不透明句柄，除了交回 `release_frame()` 之外不许解读。
struct DecodedFrame {
    std::uint64_t index = 0;
    std::int64_t presentation_time_us = 0;

    /// 该帧带 `AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM`。
    /// **带了 EOS 的帧仍然是数据帧**，一样要 release，不能因为它是最后一帧就跳过。
    bool end_of_stream = false;
};

}  // namespace kivo::video::mediacodec
