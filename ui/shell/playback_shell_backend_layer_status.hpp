#pragma once

#include <QJsonObject>
#include <QVariantMap>

namespace kivo::ui::shell {

[[nodiscard]] QJsonObject build_backend_layer_status(const QVariantMap& playback_query);

}  // namespace kivo::ui::shell
