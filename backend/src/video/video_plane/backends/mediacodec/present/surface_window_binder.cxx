#include "video/video_plane/backends/mediacodec/present/surface_window_binder.hpp"

namespace kivo::video::mediacodec {

SurfaceWindowBinder::SurfaceWindowBinder(ANativeWindow* window) : window_(window) {}

SurfaceWindowBinder::~SurfaceWindowBinder() {
    reset();
}

void SurfaceWindowBinder::reset() {
    if (window_ != nullptr) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

}  // namespace kivo::video::mediacodec
