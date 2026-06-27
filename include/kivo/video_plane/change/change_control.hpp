// change_control.hpp — GATE-038: P5A change control
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::change {

enum class ChangeImpact {
    None,                                   // no impact on existing contracts
    Additive,                               // additive change only
    Breaking,                               // breaking change
    Deprecated                              // deprecation (non-breaking)
};

struct ChangeControl {
    std::string change_id;                  // unique change identifier
    std::string description;                // change description
    ChangeImpact impact{ChangeImpact::None};
    bool requires_approval{true};           // change requires approval
    std::string approved_by;                // who approved the change
    uint64_t approved_timestamp{0};
    bool operator==(ChangeControl const&) const = default;
};

}  // namespace kivo::video_plane::change
