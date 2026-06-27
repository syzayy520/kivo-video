// success_definition_item.hpp — GATE-037: P5A success definition item
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::success {

struct SuccessDefinitionItem {
    std::string scenario_name;              // scenario name (e.g. "local-mp4-sdr")
    std::string success_criteria;           // what constitutes success
    std::string measurement_method;         // how success is measured
    bool is_required{true};                 // this scenario is required for P5 success
    std::string test_name;                  // associated test name
    bool operator==(SuccessDefinitionItem const&) const = default;
};

}  // namespace kivo::video_plane::success
