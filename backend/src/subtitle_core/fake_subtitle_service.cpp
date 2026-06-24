#include "kivo/cinema_engine/subtitle_core/fake_subtitle_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

SubtitleRenderPlan build_fake_subtitle_render_plan(const ContractMetadata& metadata,
                                                   std::string subtitle_stream_id,
                                                   std::string subtitle_kind) {
  SubtitleRenderPlan plan;
  plan.metadata = metadata;
  plan.subtitle_render_plan_id = "subtitle.fake.service.0001";
  plan.session_id = metadata.session_id;
  plan.subtitle_stream_id = std::move(subtitle_stream_id);
  plan.subtitle_kind = std::move(subtitle_kind);
  plan.renderer_backend = "libass-adapter";
  plan.font_resolution_policy = "embedded-first";
  plan.embedded_font_policy = "fallback-with-reason";
  plan.ass_override_policy = "respect-script";
  plan.hdr_luminance_policy = "tone-map-subtitle-luma";
  plan.safe_area_policy = "content-safe-area";
  plan.timing_correction = "none";
  plan.user_offset = "0ms";
  plan.language_policy = "user-preferred";
  plan.forced_subtitle_policy = "auto-forced";
  plan.default_subtitle_policy = "prefer-matched-audio";
  return plan;
}

}  // namespace kivo::cinema_engine

