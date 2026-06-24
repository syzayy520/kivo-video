#pragma once

#include "kivo/cinema_engine/preference_core/preference_snapshot.hpp"

namespace kivo::cinema_engine {

// Build a default PreferenceSnapshot for testing.
PreferenceSnapshot build_default_preference_snapshot();

// Build a PreferenceSnapshot with custom subtitle language.
PreferenceSnapshot build_preference_snapshot_with_subtitle(
    std::string subtitle_language, bool enabled);

}  // namespace kivo::cinema_engine
