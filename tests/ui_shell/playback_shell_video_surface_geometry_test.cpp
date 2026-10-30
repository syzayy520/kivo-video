#include "playback_shell_video_surface_geometry.hpp"

#include <QRectF>
#include <QSize>

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

bool close_to(const double actual, const double expected) {
    return std::abs(actual - expected) < 0.01;
}

void expect_rect(const QRectF& actual, const double x, const double y,
                 const double width, const double height) {
    assert(close_to(actual.x(), x));
    assert(close_to(actual.y(), y));
    assert(close_to(actual.width(), width));
    assert(close_to(actual.height(), height));
}

}  // namespace

int main() {
    using kivo::ui::shell::playback_video_surface_target_rect;

    const QSize portrait_source(360, 640);
    const QSizeF widescreen_surface(1280, 720);

    expect_rect(playback_video_surface_target_rect(portrait_source, widescreen_surface,
                                                   QStringLiteral("fit")),
                437.5, 0.0, 405.0, 720.0);
    expect_rect(playback_video_surface_target_rect(portrait_source, widescreen_surface,
                                                   QStringLiteral("fill")),
                0.0, -777.78, 1280.0, 2275.56);
    expect_rect(playback_video_surface_target_rect(portrait_source, widescreen_surface,
                                                   QStringLiteral("stretch")),
                0.0, 0.0, 1280.0, 720.0);
    expect_rect(playback_video_surface_target_rect(portrait_source, widescreen_surface,
                                                   QStringLiteral("original")),
                460.0, 40.0, 360.0, 640.0);

    expect_rect(playback_video_surface_target_rect(QSize(3840, 2160), widescreen_surface,
                                                   QStringLiteral("original")),
                0.0, 0.0, 1280.0, 720.0);

    std::cout << "playback shell video surface geometry passed\n";
    return 0;
}
