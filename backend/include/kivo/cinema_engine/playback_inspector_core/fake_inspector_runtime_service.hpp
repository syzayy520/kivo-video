#pragma once

#include <string>

#include "kivo/cinema_engine/playback_inspector_core/inspector_runtime.hpp"
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

}  // namespace kivo::cinema_engine