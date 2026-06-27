// malformed_corpus_item.hpp — GATE-031: P5A malformed corpus item
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::malformed {

enum class MalformedKind {
    Truncated,                              // file is truncated
    CorruptHeader,                          // header is corrupted
    InvalidCodec,                           // invalid codec identifier
    OversizedPacket,                        // packet exceeds allocation limit
    CircularReference,                      // circular reference detected
    MixedContainers,                        // mixed container formats
    Unknown                                 // unknown malformation
};

struct MalformedCorpusItem {
    MalformedKind kind{MalformedKind::Unknown};
    std::string corpus_path;                // path to malformed corpus file
    std::string expected_behavior;          // expected bounded failure behavior
    std::string regression_id;              // regression test identifier
    bool operator==(MalformedCorpusItem const&) const = default;
};

}  // namespace kivo::video_plane::malformed
