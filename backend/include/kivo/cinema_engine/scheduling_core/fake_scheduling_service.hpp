#pragma once

#include "kivo/cinema_engine/scheduling_core/scheduling_policy_config.hpp"
#include "kivo/cinema_engine/scheduling_core/scheduling_policy_status.hpp"

namespace kivo::cinema_engine {

SchedulingPolicyConfig build_default_scheduling_config(const std::string& policy_id);
SchedulingPolicyConfig build_mmcss_enabled_config(const std::string& policy_id);
SchedulingPolicyConfig build_mmcss_disabled_config(const std::string& policy_id);
SchedulingPolicyStatus build_registered_status(const std::string& policy_id);
SchedulingPolicyStatus build_failed_status(const std::string& policy_id, const std::string& error);
SchedulingPolicyStatus build_unavailable_status(const std::string& policy_id);

}  // namespace kivo::cinema_engine