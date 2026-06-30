#pragma once

#include <QVariantMap>

#include "kivo/video/player_runtime_adapter/adapter_snapshot.hpp"

namespace kivo::ui::shell {

[[nodiscard]] QVariantMap map_adapter_snapshot(
    const kivo::video::player_runtime_adapter::AdapterSnapshot& snapshot);

}  // namespace kivo::ui::shell