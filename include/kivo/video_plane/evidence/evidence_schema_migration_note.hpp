#pragma once

#include <string>
#include <cstdint>

namespace kivo::video_plane::evidence {

struct P5EvidenceSchemaMigrationNote {
    uint32_t from_version{0};
    uint32_t to_version{0};
    std::string breaking_change_description;
    std::string migration_instructions;
    bool operator==(P5EvidenceSchemaMigrationNote const&) const = default;
};

}  // namespace kivo::video_plane::evidence
