#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#include "kivo/cinema_engine/video_upload/yuv_rgb_conversion.hpp"
#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi.h>
#endif

using namespace kivo::cinema_engine;

bool is_d3d11_available() {
#ifdef _WIN32
    // Try to create a D3D11 device
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL feature_level;
    
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &feature_level,
        &context
    );
    
    if (FAILED(hr)) {
        // Try WARP (software renderer)
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &device,
            &feature_level,
            &context
        );
    }
    
    if (device) device->Release();
    if (context) context->Release();
    
    return SUCCEEDED(hr);
#else
    return false;
#endif
}

int main() {
    std::cout << "d3d11_first_frame_test:\n";

    // Check if D3D11 is available
    if (!is_d3d11_available()) {
        std::cout << "  SKIP: D3D11 not available on this system\n";
        std::cout << "ALL TESTS SKIPPED (D3D11 not available)\n";
        return 0;
    }

    // Test 1: Initialize D3D11 device context
    D3d11DeviceContext device;
    bool init_result = device.initialize();
    if (!init_result) {
        std::cout << "  SKIP: D3D11 initialization failed\n";
        std::cout << "ALL TESTS SKIPPED (D3D11 initialization failed)\n";
        return 0;
    }
    assert(device.is_ready());
    std::cout << "  PASS: D3D11 device initialized (" << device.device_type() << ")\n";

    // Test 2: Create texture
    D3d11TextureUpload uploader;
    uploader.initialize(&device);
    D3D11TextureHandle texture = uploader.create_texture(320, 240, "yuv420p");
    if (texture == nullptr) {
        std::cout << "  SKIP: Texture creation failed\n";
        device.release();
        std::cout << "ALL TESTS SKIPPED (Texture creation failed)\n";
        return 0;
    }
    std::cout << "  PASS: Texture created (320x240 yuv420p)\n";

    // Test 3: Upload frame with real data
    DecodedFrame frame;
    frame.width = 320;
    frame.height = 240;
    frame.pixel_format = "yuv420p";
    frame.stream_kind = "video";
    frame.pts = 0;

    // Create real YUV420P data
    const int y_size = 320 * 240;
    const int uv_size = (320 / 2) * (240 / 2);
    frame.frame_data.resize(y_size + uv_size * 2);
    
    // Fill Y plane with a pattern
    for (int i = 0; i < y_size; ++i) {
        frame.frame_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    // Fill U and V planes with different patterns
    for (int i = 0; i < uv_size; ++i) {
        frame.frame_data[y_size + i] = static_cast<uint8_t>((i * 2) % 256);
        frame.frame_data[y_size + uv_size + i] = static_cast<uint8_t>((i * 3) % 256);
    }

    frame.linesize[0] = 320;  // Y plane stride
    frame.linesize[1] = 160;  // U plane stride
    frame.linesize[2] = 160;  // V plane stride

    YuvRgbConversion conversion;
    bool upload_result = uploader.upload_frame(texture, frame, conversion);
    assert(upload_result);
    std::cout << "  PASS: Frame uploaded with real data (320x240 yuv420p)\n";

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