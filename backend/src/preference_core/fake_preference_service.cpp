#include "kivo/cinema_engine/preference_core/fake_preference_service.hpp"

namespace kivo::cinema_engine {

PreferenceSnapshot build_default_preference_snapshot() {
  PreferenceSnapshot snapshot;
  snapshot.metadata.schema_version = "1.1";
  snapshot.metadata.producer_version = "kivo-cinema-engine-p2";
  return snapshot;
}

PreferenceSnapshot build_preference_snapshot_with_subtitle(
    std::string subtitle_language, bool enabled) {
  PreferenceSnapshot snapshot;
  snapshot.metadata.schema_version = "1.1";
  snapshot.metadata.producer_version = "kivo-cinema-engine-p2";
  snapshot.subtitle.language = std::move(subtitle_language);
  snapshot.subtitle.enabled = enabled;
  return snapshot;
}

}  // namespace kivo::cinema_engine
