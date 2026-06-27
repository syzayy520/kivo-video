// fuzz_smoke.hpp — GATE-006: P5A fuzz smoke test descriptor (8 fields)
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::fuzz {

struct FuzzSmoke {
    std::string fuzz_target_name;           // e.g. "demux_parser", "decode_frame"
    std::string corpus_seed_path;           // path to seed corpus
    std::string expected_failure_mode;      // bounded failure mode description
    uint64_t max_input_bytes{0};            // max input size per fuzz run
    uint64_t max_runtime_ms{0};             // timeout per fuzz run
    uint32_t max_iterations{0};             // iteration cap
    bool requires_sandbox{true};            // must run inside sandbox broker
    std::string regression_corpus_path;     // path to regression corpus
    bool operator==(FuzzSmoke const&) const = default;
};

}  // namespace kivo::video_plane::fuzz
