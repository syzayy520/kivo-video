// symbol_checker_rule.hpp — GATE-026: P5A symbol checker rule
#pragma once
#include <string>

namespace kivo::video_plane::architecture {

struct SymbolCheckerRule {
    std::string check_name;
    std::string target;
    std::string allowed_pattern;
    std::string forbidden_pattern;
};

using PublicExportSymbolScan = SymbolCheckerRule;
using InternalSymbolScan = SymbolCheckerRule;
using ForbiddenAbiTypeScan = SymbolCheckerRule;
using DependencyGraphScan = SymbolCheckerRule;
using BinaryExportSurfaceScan = SymbolCheckerRule;

}  // namespace kivo::video_plane::architecture
