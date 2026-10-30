#include "playback_shell_video_surface_geometry.hpp"

#include <algorithm>

namespace kivo::ui::shell {
namespace {

QRectF centered_rect(const QRectF& bounds, const QSizeF& size) {
    return QRectF(bounds.center().x() - size.width() / 2.0,
                  bounds.center().y() - size.height() / 2.0,
                  size.width(), size.height());
}

}  // namespace

QRectF playback_video_surface_target_rect(const QSize& source_size,
                                          const QSizeF& surface_size,
                                          const QString& aspect_mode) {
    const QRectF surface_rect(0, 0, surface_size.width(), surface_size.height());
    if (source_size.isEmpty() || surface_size.isEmpty()) {
        return surface_rect;
    }
    if (aspect_mode == QStringLiteral("stretch")) {
        return surface_rect;
    }

    const double source_width = static_cast<double>(source_size.width());
    const double source_height = static_cast<double>(source_size.height());
    const double scale_x = surface_size.width() / source_width;
    const double scale_y = surface_size.height() / source_height;

    if (aspect_mode == QStringLiteral("original") && source_width <= surface_size.width() &&
        source_height <= surface_size.height()) {
        return centered_rect(surface_rect, QSizeF(source_width, source_height));
    }

    const double scale = aspect_mode == QStringLiteral("fill")
                             ? std::max(scale_x, scale_y)
                             : std::min(scale_x, scale_y);
    return centered_rect(surface_rect, QSizeF(source_width * scale, source_height * scale));
}

}  // namespace kivo::ui::shell
