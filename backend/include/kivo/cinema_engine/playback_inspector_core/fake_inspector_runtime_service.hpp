#pragma once

#include <string>

#include "kivo/cinema_engine/playback_inspector_core/inspector_runtime.hpp"
#include "kivo/cinema_engine/playback_inspector_core/inspector_read_request.hpp"
#include "kivo/cinema_engine/playback_inspector_core/inspector_read_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

/// Build a default InspectorRuntimeContract with all data sources enabled.
InspectorRuntimeContract build_default_inspector_runtime(const std::string& inspector_id);

/// Build an inspector runtime that reads only decision ledger.
InspectorRuntimeContract build_decision_ledger_only_inspector(const std::string& inspector_id);

/// Build an inspector runtime that reads only telemetry timeline.
InspectorRuntimeContract build_telemetry_only_inspector(const std::string& inspector_id);

/// Build an inspector runtime that does not support redaction.
InspectorRuntimeContract build_no_redaction_inspector(const std::string& inspector_id);

/// Build an inspector runtime that reads nothing (disabled).
InspectorRuntimeContract build_disabled_inspector(const std::string& inspector_id);

/// Build a snapshot read request.
InspectorReadRequest build_snapshot_read_request(const std::string& inspector_id, const std::string& session_id);

/// Build a ledger read request.
InspectorReadRequest build_ledger_read_request(const std::string& inspector_id, const std::string& session_id);

/// Build a telemetry read request.
InspectorReadRequest build_telemetry_read_request(const std::string& inspector_id, const std::string& session_id);

/// Build a successful read result.
InspectorReadResult build_successful_read_result(const std::string& request_id, const std::string& source_type, int64_t record_count);

/// Build an empty read result.
InspectorReadResult build_empty_read_result(const std::string& request_id, const std::string& source_type);

/// Build a redacted read result.
InspectorReadResult build_redacted_read_result(const std::string& request_id, const std::string& source_type);

}  // namespace kivo::cinema_engine