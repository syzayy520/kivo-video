// budget_pressure_level.hpp — GATE-024: P5A budget pressure level
#pragma once

namespace kivo::video_plane::gpu {

enum class BudgetPressureLevel {
    Relaxed,                                // plenty of memory available
    Moderate,                               // memory usage moderate
    High,                                   // memory usage high
    Critical                                // memory usage critical
};

}  // namespace kivo::video_plane::gpu
