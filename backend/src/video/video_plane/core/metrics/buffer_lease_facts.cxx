#include "video/video_plane/core/metrics/buffer_lease_facts.hpp"

namespace kivo::video::core::metrics {

BufferLeaseFacts make_buffer_lease_facts(std::int64_t received, std::int64_t released) noexcept {
    BufferLeaseFacts facts;
    facts.received = received;
    facts.released = released;
    facts.in_flight = received - released;

    // EOS 缓冲**两边都在**：它占一个输出槽（进 `received`），也被归还（进 `released`），
    // 所以在"归还账"里正好抵消，不需要减 1。
    // 那个 +1 属于**帧账**（`received = rendered + dropped + 1`），不是这里 ——
    // 第一版把两者混了，于是每一轮正确的会话都会报"泄漏 −1"。
    facts.leaked = facts.in_flight;

    if (facts.in_flight < 0) {
        facts.reason = "released more buffers than received; the two counters disagree";
    } else if (facts.leaked != 0) {
        facts.reason =
            "some output buffers were never returned to the codec; a holding slot like this "
            "stalls the decoder after a few dozen frames";
    } else {
        facts.reason = "";
    }
    return facts;
}

}  // namespace kivo::video::core::metrics
