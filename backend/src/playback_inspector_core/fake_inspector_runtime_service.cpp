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

// --- Request builders ---

InspectorReadRequest build_snapshot_read_request(const std::string& inspector_id, const std::string& session_id) {
    InspectorReadRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-snapshot-" + session_id;
    r.inspector_id = inspector_id;
    r.source_type = "snapshot";
    r.session_id = session_id;
    r.include_redacted = false;
    return r;
}

InspectorReadRequest build_ledger_read_request(const std::string& inspector_id, const std::string& session_id) {
    InspectorReadRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-ledger-" + session_id;
    r.inspector_id = inspector_id;
    r.source_type = "ledger";
    r.session_id = session_id;
    r.include_redacted = false;
    return r;
}

InspectorReadRequest build_telemetry_read_request(const std::string& inspector_id, const std::string& session_id) {
    InspectorReadRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-telemetry-" + session_id;
    r.inspector_id = inspector_id;
    r.source_type = "telemetry";
    r.session_id = session_id;
    r.include_redacted = false;
    return r;
}

// --- Result builders ---

InspectorReadResult build_successful_read_result(const std::string& request_id, const std::string& source_type, int64_t record_count) {
    InspectorReadResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-ok-" + request_id;
    res.request_id = request_id;
    res.source_type = source_type;
    res.data_payload = "{\"status\":\"ok\",\"source\":\"" + source_type + "\"}";
    res.record_count = record_count;
    res.was_redacted = false;
    return res;
}

InspectorReadResult build_empty_read_result(const std::string& request_id, const std::string& source_type) {
    InspectorReadResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-empty-" + request_id;
    res.request_id = request_id;
    res.source_type = source_type;
    res.data_payload = "{\"status\":\"empty\"}";
    res.record_count = 0;
    res.was_redacted = false;
    return res;
}

InspectorReadResult build_redacted_read_result(const std::string& request_id, const std::string& source_type) {
    InspectorReadResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-redacted-" + request_id;
    res.request_id = request_id;
    res.source_type = source_type;
    res.data_payload = "{\"status\":\"redacted\",\"fields\":[\"token\",\"credential\"]}";
    res.record_count = 5;
    res.was_redacted = true;
    return res;
}

}  // namespace kivo::cinema_engine