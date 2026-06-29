#include "video/playback_graph/generation/generation_stamp_publisher.hpp"

namespace kivo::video::playback_graph::runtime {

void GenerationStampPublisher::publish_pending_barrier(GenerationTupleId tuple_id,
                                                       std::uint64_t stream_stamp_id) noexcept {
    publish(tuple_id, stream_stamp_id, GenerationStampState::PendingBarrier);
}

void GenerationStampPublisher::publish_active(GenerationTupleId tuple_id,
                                              std::uint64_t stream_stamp_id) noexcept {
    publish(tuple_id, stream_stamp_id, GenerationStampState::Active);
}

void GenerationStampPublisher::publish_retired(GenerationTupleId tuple_id,
                                               std::uint64_t stream_stamp_id) noexcept {
    publish(tuple_id, stream_stamp_id, GenerationStampState::Retired);
}

GenerationStampReadResult GenerationStampPublisher::read(
    std::uint32_t max_attempt_count) const noexcept {
    GenerationStampReadResult result{};
    for (std::uint32_t attempt = 0; attempt < max_attempt_count; ++attempt) {
        result.attempts = attempt + 1;
        const auto before = sequence_.load(std::memory_order_acquire);
        if ((before & 1U) != 0U) {
            continue;
        }

        const auto tuple = GenerationTupleId{tuple_id_.load(std::memory_order_acquire)};
        const auto encoded = stream_and_state_.load(std::memory_order_acquire);
        const auto after = sequence_.load(std::memory_order_acquire);
        if (before == after && (after & 1U) == 0U) {
            result.stable = true;
            result.stamp = decode(tuple, encoded);
            return result;
        }
    }
    return result;
}

bool GenerationStampPublisher::hot_path_accepts(const GenerationStamp& stamp) noexcept {
    return stamp.state == GenerationStampState::Active;
}

void GenerationStampPublisher::begin_unstable_write_for_test() noexcept {
    sequence_.fetch_add(1, std::memory_order_acq_rel);
}

void GenerationStampPublisher::finish_unstable_write_for_test(GenerationTupleId tuple_id,
                                                              std::uint64_t stream_stamp_id,
                                                              GenerationStampState state) noexcept {
    tuple_id_.store(tuple_id.value, std::memory_order_release);
    stream_and_state_.store(encode_stream_and_state(stream_stamp_id, state),
                            std::memory_order_release);
    sequence_.fetch_add(1, std::memory_order_acq_rel);
}

void GenerationStampPublisher::publish(GenerationTupleId tuple_id,
                                       std::uint64_t stream_stamp_id,
                                       GenerationStampState state) noexcept {
    begin_unstable_write_for_test();
    finish_unstable_write_for_test(tuple_id, stream_stamp_id, state);
}

std::uint64_t GenerationStampPublisher::encode_stream_and_state(
    std::uint64_t stream_stamp_id,
    GenerationStampState state) noexcept {
    auto state_bits = std::uint64_t{0};
    switch (state) {
        case GenerationStampState::PendingBarrier:
            state_bits = 0;
            break;
        case GenerationStampState::Active:
            state_bits = 1;
            break;
        case GenerationStampState::Retired:
            state_bits = 2;
            break;
    }
    return ((state_bits << kStateShift) | (stream_stamp_id & kStreamMask));
}

GenerationStamp GenerationStampPublisher::decode(GenerationTupleId tuple_id,
                                                 std::uint64_t encoded) noexcept {
    GenerationStamp stamp{};
    stamp.tuple_id = tuple_id;
    stamp.stream_stamp_id = encoded & kStreamMask;
    switch (encoded >> kStateShift) {
        case 1:
            stamp.state = GenerationStampState::Active;
            break;
        case 2:
            stamp.state = GenerationStampState::Retired;
            break;
        default:
            stamp.state = GenerationStampState::PendingBarrier;
            break;
    }
    return stamp;
}

}  // namespace kivo::video::playback_graph::runtime
