#pragma once

#include <cstddef>
#include <vector>

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph::runtime {

enum class CriticalProofKind {
    Close,
    Recovery,
    Fatal
};

struct CriticalProof {
    CriticalProofKind kind{CriticalProofKind::Close};
    PlaybackCommandId command_id{};
    PlaybackGraphError error{PlaybackGraphError::None};
};

class CriticalProofRing {
public:
    CriticalProofRing(std::size_t capacity, std::size_t reserved_critical_slots);

    void set_persistent_sink_available(bool available) noexcept;
    [[nodiscard]] bool push(CriticalProof proof) noexcept;
    [[nodiscard]] bool contains(CriticalProofKind kind) const noexcept;
    [[nodiscard]] std::uint32_t sink_failure_count() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::size_t capacity_{0};
    std::size_t reserved_critical_slots_{0};
    bool persistent_sink_available_{true};
    std::vector<CriticalProof> proofs_{};
    std::uint32_t sink_failure_count_{0};
};

}  // namespace kivo::video::playback_graph::runtime
