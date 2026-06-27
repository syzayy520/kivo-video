// architecture_violation_schema.hpp — GATE-026: P5A architecture violation schema
#pragma once
#include <string>

namespace kivo::video_plane::architecture {

struct ArchitectureViolation {
    enum class Kind {
        ForbiddenLink,                      // target links forbidden library
        ForbiddenInclude,                   // header includes forbidden platform header
        ForbiddenSymbol,                    // binary exports forbidden symbol
        DependencyCycle,                    // circular dependency detected
        FutureBackendInBaseline,            // future backend in baseline binary
        PublicHeaderThirdPartyType,         // public header exposes raw third-party type
        CoreExposesBackendAbi              // core exposes FFmpeg/D3D11 ABI
    };
    Kind kind;
    std::string description;
    std::string file_path;
    std::string target;
    std::string evidence_id;
};

}  // namespace kivo::video_plane::architecture
