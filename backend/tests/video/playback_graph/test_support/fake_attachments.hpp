#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::playback_graph::test_support {

struct FakeAttachmentTrace {
    std::uint32_t seed{0};
    std::array<std::uint32_t, 4> commands{};
    std::uint32_t command_count{0};
};

class FakeAttachments {
public:
    [[nodiscard]] FakeAttachmentTrace run_seed(std::uint32_t seed) noexcept {
        FakeAttachmentTrace trace{};
        trace.seed = seed;
        trace.commands = {seed, seed + 1, seed + 2, seed + 3};
        trace.command_count = 4;
        return trace;
    }
};

}  // namespace kivo::video::playback_graph::test_support
