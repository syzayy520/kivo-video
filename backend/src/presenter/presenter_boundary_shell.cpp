#include "kivo/cinema_engine/presenter/presenter_boundary_shell.hpp"

#include <iostream>
#include <chrono>

namespace kivo::cinema_engine {

bool PresenterBoundaryShell::initialize() {
    initialized_ = true;
    std::cout << "[PresenterBoundaryShell] Initialized\n";
    return true;
}

bool PresenterBoundaryShell::present(D3D11TextureHandle texture, int32_t width, int32_t height) {
    if (!initialized_ || !texture) return false;

    if (!has_first_frame_) {
        first_frame_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        has_first_frame_ = true;
        std::cout << "[PresenterBoundaryShell] First frame presented: " << width << "x" << height << "\n";
    }

    frames_presented_++;
    return true;
}

int64_t PresenterBoundaryShell::first_frame_time_us() const {
    return first_frame_time_us_;
}

int64_t PresenterBoundaryShell::frames_presented() const {
    return frames_presented_;
}

bool PresenterBoundaryShell::has_first_frame() const {
    return has_first_frame_;
}

std::string PresenterBoundaryShell::status() const {
    if (!initialized_) return "not_initialized";
    if (!has_first_frame_) return "waiting_for_first_frame";
    return "presenting";
}

void PresenterBoundaryShell::release() {
    initialized_ = false;
    first_frame_time_us_ = 0;
    frames_presented_ = 0;
    has_first_frame_ = false;
    std::cout << "[PresenterBoundaryShell] Released\n";
}

bool PresenterBoundaryShell::is_ready() const {
    return initialized_;
}

}  // namespace kivo::cinema_engine