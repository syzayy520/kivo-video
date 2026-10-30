#pragma once

#include <QRectF>
#include <QSize>
#include <QString>

namespace kivo::ui::shell {

[[nodiscard]] QRectF playback_video_surface_target_rect(const QSize& source_size,
                                                        const QSizeF& surface_size,
                                                        const QString& aspect_mode);

}  // namespace kivo::ui::shell
