// quota_scope.hpp — GATE-021: P5A attachment quota scope
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::attachment {

struct QuotaScope {
    uint64_t max_total_bytes{0};            // max total attachment bytes
    uint64_t current_bytes{0};              // current attachment bytes
    uint32_t max_attachment_count{0};       // max number of attachments
    uint32_t current_count{0};              // current attachment count
    std::string scope_id;                   // quota scope identifier
    bool operator==(QuotaScope const&) const = default;
};

}  // namespace kivo::video_plane::attachment
