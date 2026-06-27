// attachment_safety_descriptor.hpp — P5B B6: Attachment Safety Descriptor
#pragma once
#include <cstdint>
#include <string>
#include "../attachment/mime_conflict_policy.hpp"
#include "../attachment/quota_scope.hpp"

namespace kivo::video_plane::input_bridge {

enum class AttachmentSafetyState {
    NotChecked,
    Safe,
    Quarantined,
    Rejected
};

struct AttachmentSafetyDescriptor {
    std::string attachment_id;
    std::string declared_mime;
    std::string sniffed_mime;
    uint64_t byte_size{0};
    attachment::MimeConflictPolicy mime_policy;
    attachment::QuotaScope quota_scope;
    AttachmentSafetyState safety_state{AttachmentSafetyState::NotChecked};

    // Evaluate safety based on MIME + quota
    AttachmentSafetyState evaluate() {
        // Check MIME conflict
        if (!declared_mime.empty() && !sniffed_mime.empty() && declared_mime != sniffed_mime) {
            if (mime_policy.resolution == attachment::MimeConflictResolution::Reject) {
                safety_state = AttachmentSafetyState::Rejected;
                return safety_state;
            }
            if (mime_policy.resolution == attachment::MimeConflictResolution::UseSafest) {
                safety_state = AttachmentSafetyState::Quarantined;
                return safety_state;
            }
        }
        // Check quota
        if (quota_scope.current_bytes + byte_size > quota_scope.max_total_bytes) {
            safety_state = AttachmentSafetyState::Rejected;
            return safety_state;
        }
        safety_state = AttachmentSafetyState::Safe;
        return safety_state;
    }

    bool operator==(AttachmentSafetyDescriptor const&) const = default;
};

}  // namespace kivo::video_plane::input_bridge
