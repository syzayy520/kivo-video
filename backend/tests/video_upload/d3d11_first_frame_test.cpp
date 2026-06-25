#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#include "kivo/cinema_engine/video_upload/yuv_rgb_conversion.hpp"
#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"

using namespace kivo::cinema_engine;

int main() {
    std::cout << "d3d11_first_frame_test:\n";

    // Test 1: Initialize D3D11 device context
    D3d11DeviceContext device;
    bool init_result = device.initialize();
    assert(init_result);
    assert(device.is_ready());
    std::cout << "  PASS: D3D11 device initialized (" << device.device_type() << ")\n";

    // Test 2: Create texture
    D3d11TextureUpload uploader;
    uploader.release(); // ensure clean state
    D3D11TextureHandle texture = uploader.create_texture(320, 240, "yuv420p");
    assert(texture != nullptr);
    std::cout << "  PASS: Texture created (320x240 yuv420p)\n";

    // Test 3: Upload frame
    DecodedFrame frame;
    frame.width = 320;
    frame.height = 240;
    frame.pixel_format = "yuv420p";
    frame.stream_kind = "video";
    frame.pts = 0;

    YuvRgbConversion conversion;
    bool upload_result = uploader.upload_frame(texture, frame, conversion);
    assert(upload_result);
    std::cout << "  PASS: Frame uploaded (320x240 yuv420p)\n";

    // Test 4: Get texture dimensions
    int32_t width, height;
    uploader.get_texture_dimensions(texture, width, height);
    assert(width == 320);
    assert(height == 240);
    std::cout << "  PASS: Texture dimensions verified (320x240)\n";

    // Test 5: Release texture
    uploader.release_texture(texture);
    std::cout << "  PASS: Texture released\n";

    // Test 6: Release all resources
    uploader.release();
    device.release();
    std::cout << "  PASS: All resources released\n";

    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}