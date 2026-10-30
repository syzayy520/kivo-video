#pragma once

#include <cstdint>

namespace kivo::video::core::metrics {

/// 输出缓冲的归属账（P14-A / G4）。
///
/// **必须先说清一件事**：Android 的输出缓冲池**属于 codec**，不属于我们。
/// 平台不提供"池里 allocated 了多少"的读数（`ACodec` 的统计在 dumpsys 里，
/// 不是稳定 API）。所以本结构只出版**我们自己的契约**：
/// 从解码器拿到的每一个缓冲，是否都还回去了。
///
/// 把"我们看不到平台的池"写成 `pool_owner = "codec"`，而不是假装没这回事 ——
/// 一份 G4 证据如果不说这句，读者会以为 `leaked = 0` 覆盖了平台池。
struct BufferLeaseFacts {
    /// 池的拥有者。恒为 `"codec"`（当前平台），出版它是为了让这句话有出处。
    const char* pool_owner = "codec";

    std::int64_t received = 0;  ///< 从解码器取走的输出缓冲数（含末尾 EOS）
    std::int64_t released = 0;  ///< 已经交回平台的缓冲数

    /// 在途 = 取了但没还。**stop 之后必须是 0**，否则有槽永远回不来。
    std::int64_t in_flight = 0;

    /// 泄漏 = 在途。**0 才是"没有泄漏"。**
    ///
    /// 为什么不减掉末尾 EOS：EOS 也占一个输出槽（进 `received`），也同样被归还
    /// （进 `released`），在归还账里正好抵消。那个 +1 属于**帧账**
    /// （`received = rendered + dropped + 1`），不是这个账 ——
    /// 两者混起来会让每一轮正确的会话都报"泄漏 −1"。
    std::int64_t leaked = 0;

    /// `in_flight != 0` 时非空。空串 = 契约成立。
    const char* reason = "";
};

/// 由会话计数算账。**纯函数**。
[[nodiscard]] BufferLeaseFacts make_buffer_lease_facts(std::int64_t received,
                                                      std::int64_t released) noexcept;

}  // namespace kivo::video::core::metrics
