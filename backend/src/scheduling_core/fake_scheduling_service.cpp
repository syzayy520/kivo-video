#include "kivo/cinema_engine/scheduling_core/fake_scheduling_service.hpp"

namespace kivo::cinema_engine {

SchedulingPolicyConfig build_default_scheduling_config(const std::string& policy_id) {
    SchedulingPolicyConfig config;
    config.metadata.schema_version = "1.1";
    config.policy_id = policy_id;
    config.task_category = "Playback";
    config.priority_level = 2;
    config.enable_mmcss = true;
    config.fallback_on_failure = true;
    return config;
}

SchedulingPolicyConfig build_mmcss_enabled_config(const std::string& policy_id) {
    SchedulingPolicyConfig config;
    config.metadata.schema_version = "1.1";
    config.policy_id = policy_id;
    config.task_category = "Playback";
    config.priority_level = 5;
    config.enable_mmcss = true;
    config.fallback_on_failure = true;
    return config;
}

SchedulingPolicyConfig build_mmcss_disabled_config(const std::string& policy_id) {
    SchedulingPolicyConfig config;
    config.metadata.schema_version = "1.1";
    config.policy_id = policy_id;
    config.task_category = "Playback";
    config.priority_level = 2;
    config.enable_mmcss = false;
    config.fallback_on_failure = true;
    return config;
}

SchedulingPolicyStatus build_registered_status(const std::string& policy_id) {
    SchedulingPolicyStatus status;
    status.metadata.schema_version = "1.1";
    status.policy_id = policy_id;
    status.mmcss_registered = true;
    status.mmcss_available = true;
    status.current_priority = 5;
    status.last_error = "";
    return status;
}

SchedulingPolicyStatus build_failed_status(const std::string& policy_id, const std::string& error) {
    SchedulingPolicyStatus status;
    status.metadata.schema_version = "1.1";
    status.policy_id = policy_id;
    status.mmcss_registered = false;
    status.mmcss_available = true;
    status.current_priority = 0;
    status.last_error = error;
    return status;
}

SchedulingPolicyStatus build_unavailable_status(const std::string& policy_id) {
    SchedulingPolicyStatus status;
    status.metadata.schema_version = "1.1";
    status.policy_id = policy_id;
    status.mmcss_registered = false;
    status.mmcss_available = false;
    status.current_priority = 0;
    status.last_error = "MMCSS not available";
    return status;
}

}  // namespace kivo::cinema_engine