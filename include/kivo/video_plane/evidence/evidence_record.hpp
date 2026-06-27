#pragma once

#include "kivo/video_plane/evidence/evidence_severity.hpp"
#include "kivo/video_plane/evidence/evidence_loss_policy.hpp"
#include "kivo/video_plane/evidence/evidence_redaction_level.hpp"
#include "kivo/video_plane/stamp/core_video_stamp.hpp"
#include "kivo/video_plane/stamp/full_video_stamp.hpp"
#include <string>
#include <optional>
#include <cstdint>

namespace kivo::video_plane::evidence {

using stamp::CoreVideoStamp;
using stamp::FullVideoStamp;

struct P5EvidenceRecord {
    uint32_t schema_version{0};
    std::string evidence_id;
    std::string evidence_kind;
    EvidenceSeverity severity{EvidenceSeverity::Info};
    std::string component_id;
    std::string pipeline_id;
    std::optional<CoreVideoStamp> core_video_stamp;
    std::optional<FullVideoStamp> full_video_stamp;
    uint64_t monotonic_time{0};
    std::optional<uint64_t> media_time;
    std::optional<uint64_t> thread_id;
    std::optional<std::string> state_before;
    std::optional<std::string> state_after;
    std::optional<std::string> decision;
    std::string reason_code;
    std::string technical_detail_code;
    std::optional<std::string> payload_ref;
    EvidenceRedactionLevel redaction_level{EvidenceRedactionLevel::None};
    uint32_t pii_safety_class{0};
    uint32_t protected_content_safety_class{0};
    EvidenceLossPolicy loss_policy{EvidenceLossPolicy::NonCriticalDroppable};
    std::optional<std::string> correlation_id;
    bool operator==(P5EvidenceRecord const&) const = default;
};

}  // namespace kivo::video_plane::evidence
