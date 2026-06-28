// p6_schema_version.hpp — P6A version: P6 schema version (§version)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::version {

// P6SchemaVersion
// Semantic version of the P6 schema. Compile-only.
struct P6SchemaVersion {
    uint32_t major{2};
    uint32_t minor{0};
    uint32_t patch{0};
    bool operator==(const P6SchemaVersion&) const = default;
};

}  // namespace kivo::video::audio_plane::version
