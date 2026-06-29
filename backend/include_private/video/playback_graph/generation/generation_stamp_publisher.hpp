#pragma once

#include <atomic>
#include <cstdint>

#include "kivo/video/playback_graph/generation_stamp.hpp"

namespace kivo::video::playback_graph::runtime {

struct GenerationStampReadResult {
    bool stable{false};
    std::uint32_t attempts{0};
    GenerationStamp stamp{};
};

class GenerationStampPublisher {
public:
    void publish_pending_barrier(GenerationTupleId tuple_id, std::uint64_t stream_stamp_id) noexcept;
    void publish_active(GenerationTupleId tuple_id, std::uint64_t stream_stamp_id) noexcept;
    void publish_retired(GenerationTupleId tuple_id, std::uint64_t stream_stamp_id) noexcept;

    [[nodiscard]] GenerationStampReadResult read(std::uint32_t max_attempt_count) const noexcept;
    [[nodiscard]] static bool hot_path_accepts(const GenerationStamp& stamp) noexcept;

    void begin_unstable_write_for_test() noexcept;
    void finish_unstable_write_for_test(GenerationTupleId tuple_id,
                                        std::uint64_t stream_stamp_id,
                                        GenerationStampState state) noexcept;

private:
    static constexpr std::uint64_t kStateShift = 62;
    static constexpr std::uint64_t kStreamMask = (std::uint64_t{1} << kStateShift) - 1;

    void publish(GenerationTupleId tuple_id,
                 std::uint64_t stream_stamp_id,
                 GenerationStampState state) noexcept;
    [[nodiscard]] static std::uint64_t encode_stream_and_state(
        std::uint64_t stream_stamp_id,
        GenerationStampState state) noexcept;
    [[nodiscard]] static GenerationStamp decode(GenerationTupleId tuple_id,
                                                std::uint64_t encoded) noexcept;

    std::atomic<std::uint64_t> sequence_{0};
    std::atomic<std::uint64_t> tuple_id_{0};
    std::atomic<std::uint64_t> stream_and_state_{0};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "P7D GenerationStampPublisher requires lock-free uint64 atomics");

}  // namespace kivo::video::playback_graph::runtime
