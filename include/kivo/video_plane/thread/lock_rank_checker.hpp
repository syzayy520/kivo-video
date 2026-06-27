// lock_rank_checker.hpp — GATE-025: P5A lock rank checker
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::thread {

struct LockRankChecker {
    std::string lock_name;                  // lock being checked
    uint32_t rank{0};                       // lock rank (lower = acquired first)
    bool rank_violated{false};              // rank order was violated
    std::string violation_description;      // description of violation
    bool operator==(LockRankChecker const&) const = default;
};

}  // namespace kivo::video_plane::thread
