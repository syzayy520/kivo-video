// contract_runtime_checker.hpp — GATE-025: P5A contract runtime checker
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::thread {

struct ContractRuntimeChecker {
    std::string contract_name;              // contract being checked
    bool is_violated{false};                // contract is violated
    std::string violation_description;      // description of violation
    uint64_t check_timestamp{0};            // when check was performed
    bool operator==(ContractRuntimeChecker const&) const = default;
};

}  // namespace kivo::video_plane::thread
