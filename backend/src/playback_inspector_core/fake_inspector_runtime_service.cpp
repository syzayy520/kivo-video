#include "kivo/cinema_engine/playback_inspector_core/fake_inspector_runtime_service.hpp"

namespace kivo::cinema_engine {

InspectorRuntimeContract build_default_inspector_runtime(const std::string& inspector_id) {
    InspectorRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-inspector-runtime";
    contract.inspector_id = inspector_id;
    contract.reads_decision_ledger = true;
    contract.reads_telemetry_timeline = true;
    contract.reads_capability_join = true;
    contract.supports_redaction = true;
    return contract;
}

InspectorRuntimeContract build_decision_ledger_only_inspector(const std::string& inspector_id) {
    InspectorRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-inspector-runtime";
    contract.inspector_id = inspector_id;
    contract.reads_decision_ledger = true;
    contract.reads_telemetry_timeline = false;
    contract.reads_capability_join = false;
    contract.supports_redaction = true;
    return contract;
}

InspectorRuntimeContract build_telemetry_only_inspector(const std::string& inspector_id) {
    InspectorRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-inspector-runtime";
    contract.inspector_id = inspector_id;
    contract.reads_decision_ledger = false;
    contract.reads_telemetry_timeline = true;
    contract.reads_capability_join = false;
    contract.supports_redaction = true;
    return contract;
}

InspectorRuntimeContract build_no_redaction_inspector(const std::string& inspector_id) {
    InspectorRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-inspector-runtime";
    contract.inspector_id = inspector_id;
    contract.reads_decision_ledger = true;
    contract.reads_telemetry_timeline = true;
    contract.reads_capability_join = true;
    contract.supports_redaction = false;
    return contract;
}

InspectorRuntimeContract build_disabled_inspector(const std::string& inspector_id) {
    InspectorRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-inspector-runtime";
    contract.inspector_id = inspector_id;
    contract.reads_decision_ledger = false;
    contract.reads_telemetry_timeline = false;
    contract.reads_capability_join = false;
    contract.supports_redaction = false;
    return contract;
}

}  // namespace kivo::cinema_engine