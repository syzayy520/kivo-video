#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/subtitle_core/subtitle_render_plan.hpp"

namespace kivo::cinema_engine {

SubtitleRenderPlan build_fake_subtitle_render_plan(const ContractMetadata& metadata,
                                                   std::string subtitle_stream_id,
                                                   std::string subtitle_kind);

}  // namespace kivo::cinema_engine

