#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// FeatureFlagClosurePolicy ensures closure does not rely on experimental flags.
/// Experimental flag paths need separate evidence.
/// Closure must pass with all experimental flags disabled.
/// schema_version: p3.execution.feature_flag.v1
struct FeatureFlagClosurePolicy
{
    std::string schema_version{"p3.execution.feature_flag.v1"};
    std::string flag_id{};
    bool is_experimental{false};
    bool default_on{false};
    bool is_closure_path{false};
    bool has_separate_evidence{false};

    bool is_valid_for_closure() const noexcept
    {
        if (!is_experimental) return true;
        if (default_on) return false;
        if (is_closure_path && !has_separate_evidence) return false;
        return true;
    }
};

} // namespace kivo::p3::execution
