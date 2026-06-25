#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/presenter/presenter_boundary_shell.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"

using namespace kivo::cinema_engine;

int main() {
    std::cout << "presenter_boundary_first_frame_test:\n";

    // Test 1: Initialize presenter boundary
    PresenterBoundaryShell presenter;
    bool init_result = presenter.initialize();
    assert(init_result);
    assert(presenter.is_ready());
    std::cout << "  PASS: Presenter boundary initialized\n";

    // Test 2: No first frame yet
    assert(!presenter.has_first_frame());
    assert(presenter.first_frame_time_us() == 0);
    assert(presenter.frames_presented() == 0);
    std::cout << "  PASS: No first frame initially\n";

    // Test 3: Create texture for presentation
    D3d11TextureUpload uploader;
    D3D11TextureHandle texture = uploader.create_texture(320, 240, "yuv420p");
    assert(texture != nullptr);
    std::cout << "  PASS: Texture created for presentation\n";

    // Test 4: Present first frame
    bool present_result = presenter.present(texture, 320, 240);
    assert(present_result);
    assert(presenter.has_first_frame());
    assert(presenter.first_frame_time_us() > 0);
    assert(presenter.frames_presented() == 1);
    std::cout << "  PASS: First frame presented\n";

    // Test 5: Present second frame
    bool present_result2 = presenter.present(texture, 320, 240);
    assert(present_result2);
    assert(presenter.frames_presented() == 2);
    std::cout << "  PASS: Second frame presented\n";

    // Test 6: Status check
    std::string status = presenter.status();
    assert(status == "presenting");
    std::cout << "  PASS: Status is 'presenting'\n";

    // Test 7: Release
    presenter.release();
    assert(!presenter.is_ready());
    assert(!presenter.has_first_frame());
    assert(presenter.frames_presented() == 0);
    std::cout << "  PASS: Presenter released\n";

    // Cleanup
    uploader.release_texture(texture);

    std::cout << "ALL 7 TESTS PASSED\n";
    return 0;
}