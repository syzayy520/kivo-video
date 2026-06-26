#pragma once
#include <string>
#include <vector>
namespace kivo::video {
struct EvidenceJsonValidator {
    std::vector<std::string> errors;
    bool valid{true};
    void add_error(const std::string& e) { errors.push_back(e); valid=false; }
};
}  // namespace kivo::video
