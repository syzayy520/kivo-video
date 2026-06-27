// chroma_siting_plan.hpp — GATE-019: P5A chroma siting plan
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::color {

enum class ChromaSiting {
    Left,                                   // chroma sited left
    Center,                                 // chroma sited center
    TopLeft,                                // chroma sited top-left
    TopCenter,                              // chroma sited top-center
    BottomLeft,                             // chroma sited bottom-left
    BottomCenter                            // chroma sited bottom-center
};

struct ChromaSitingPlan {
    ChromaSiting horizontal{ChromaSiting::Left};
    ChromaSiting vertical{ChromaSiting::Center};
    std::string source;                     // where siting info came from
    bool operator==(ChromaSitingPlan const&) const = default;
};

}  // namespace kivo::video_plane::color
