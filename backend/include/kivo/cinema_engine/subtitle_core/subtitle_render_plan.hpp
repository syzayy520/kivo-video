#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SubtitleRenderPlan {
  ContractMetadata metadata;
  std::string subtitle_render_plan_id;
  std::optional<std::string> session_id;
  std::string subtitle_stream_id;
  std::string subtitle_kind;
  std::string renderer_backend;
  std::string font_resolution_policy;
  std::string embedded_font_policy;
  std::string ass_override_policy;
  std::string hdr_luminance_policy;
  std::string safe_area_policy;
  std::string timing_correction;
  std::string user_offset;
  std::string language_policy;
  std::string forced_subtitle_policy;
  std::string default_subtitle_policy;
  std::optional<std::string> fallback_reason;
  std::optional<std::string> user_visible_hint;
};

}  // namespace kivo::cinema_engine

