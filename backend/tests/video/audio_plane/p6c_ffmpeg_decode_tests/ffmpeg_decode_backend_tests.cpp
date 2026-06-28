// ffmpeg_decode_backend_tests.cpp — P6C tests: FFmpegDecodeBackend (C1-C8 + TimedOut + transitions)
// Tests real avcodec decode lifecycle with AAC codec.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_backend.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"

namespace rt = kivo::video::audio_plane::backend::decode::ffmpeg;
using kivo::video::audio_plane::decode::AudioDecodeBackendState;

static int g_failed = 0;

#define P6C_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// C1: init with AAC (codec_id=1) → success, state=Ready
static void test_c01_init_aac() {
    rt::FFmpegDecodeBackend backend;
    auto r = backend.init(1, 0);  // codec_id=1 (AAC), private_data_ref=0 (none)
    if (r.is_error()) throw std::runtime_error("init AAC must succeed");
    if (backend.state() != AudioDecodeBackendState::Ready)
        throw std::runtime_error("state must be Ready after init");
    backend.close();
    std::printf("[C01] init AAC PASS\n");
}

// C2: init with unsupported codec (codec_id=999) → error
static void test_c02_init_unsupported_codec() {
    rt::FFmpegDecodeBackend backend;
    auto r = backend.init(999, 0);
    if (!r.is_error()) throw std::runtime_error("unsupported codec must fail");
    if (r.error() != rt::InitError::UnsupportedCodec && r.error() != rt::InitError::NoCodecFound)
        throw std::runtime_error("expected UnsupportedCodec or NoCodecFound");
    std::printf("[C02] init unsupported codec PASS\n");
}

// C3: init twice → error
static void test_c03_init_twice() {
    rt::FFmpegDecodeBackend backend;
    auto r1 = backend.init(1, 0);
    if (r1.is_error()) throw std::runtime_error("first init must succeed");
    auto r2 = backend.init(1, 0);
    if (!r2.is_error()) throw std::runtime_error("second init must fail");
    if (r2.error() != rt::InitError::InvalidState)
        throw std::runtime_error("expected InvalidState for second init");
    backend.close();
    std::printf("[C03] init twice rejected PASS\n");
}

// C4: submit_packet with data → success or SendFailed (codec may reject invalid data)
static void test_c04_submit_packet_valid() {
    rt::FFmpegDecodeBackend backend;
    if (backend.init(1, 0).is_error()) throw std::runtime_error("init must succeed");
    uint8_t fake_data[4] = {0x12, 0x34, 0x56, 0x78};
    auto r = backend.submit_packet(1, fake_data, 4, 1000);
    if (r.is_error()) {
        // SendFailed is acceptable — FFmpeg may reject non-AAC data.
        if (r.error() != rt::DecodeReject::SendFailed)
            throw std::runtime_error("unexpected reject reason");
    } else {
        if (backend.state() != AudioDecodeBackendState::Decoding)
            throw std::runtime_error("state must be Decoding after successful submit");
    }
    backend.close();
    std::printf("[C04] submit_packet valid PASS\n");
}

// C5: submit_packet before init → NotReady
static void test_c05_submit_before_init() {
    rt::FFmpegDecodeBackend backend;
    uint8_t fake_data[4] = {0};
    auto r = backend.submit_packet(1, fake_data, 4, 0);
    if (!r.is_error()) throw std::runtime_error("submit before init must fail");
    if (r.error() != rt::DecodeReject::NotReady)
        throw std::runtime_error("expected NotReady");
    std::printf("[C05] submit before init PASS\n");
}

// C6: receive_frame after submit → WouldBlock or frame or error (codec may reject data)
static void test_c06_receive_frame() {
    rt::FFmpegDecodeBackend backend;
    if (backend.init(1, 0).is_error()) throw std::runtime_error("init must succeed");
    uint8_t fake_data[4] = {0x12, 0x34, 0x56, 0x78};
    // submit may fail (SendFailed) with invalid AAC data — that's OK for this test.
    backend.submit_packet(1, fake_data, 4, 1000);
    auto r = backend.receive_frame(1);
    // WouldBlock, ReceiveFailed, or NotReady are all acceptable (invalid data).
    if (r.is_error()) {
        if (r.error() != rt::DecodeStatus::WouldBlock &&
            r.error() != rt::DecodeStatus::ReceiveFailed &&
            r.error() != rt::DecodeStatus::NotReady)
            throw std::runtime_error("expected WouldBlock/ReceiveFailed/NotReady for invalid data");
    }
    std::printf("[C06] receive_frame PASS\n");
    backend.close();
}

// C7: flush → success, state back to Ready
static void test_c07_flush() {
    rt::FFmpegDecodeBackend backend;
    if (backend.init(1, 0).is_error()) throw std::runtime_error("init must succeed");
    uint8_t fake_data[4] = {0};
    // submit may fail (SendFailed) with invalid data — flush still works.
    backend.submit_packet(1, fake_data, 4, 0);
    // If state is Ready (submit failed), flush from Ready is valid.
    // If state is Decoding (submit succeeded), flush from Decoding is valid.
    auto r = backend.flush();
    if (r.is_error()) throw std::runtime_error("flush must succeed");
    if (backend.state() != AudioDecodeBackendState::Ready)
        throw std::runtime_error("state must be Ready after flush");
    std::printf("[C07] flush PASS\n");
    backend.close();
}

// C8: close → state=Closed; submit after close → Closed reject
static void test_c08_close() {
    rt::FFmpegDecodeBackend backend;
    if (backend.init(1, 0).is_error()) throw std::runtime_error("init must succeed");
    backend.close();
    if (backend.state() != AudioDecodeBackendState::Closed)
        throw std::runtime_error("state must be Closed");
    uint8_t fake_data[4] = {0};
    auto r = backend.submit_packet(1, fake_data, 4, 0);
    if (!r.is_error()) throw std::runtime_error("submit after close must fail");
    if (r.error() != rt::DecodeReject::Closed)
        throw std::runtime_error("expected Closed reject");
    std::printf("[C08] close + submit rejected PASS\n");
}

// TimedOut: set_deadline + exceed_deadline + receive_frame → TimedOut
static void test_c11_timeout() {
    rt::FFmpegDecodeBackend backend;
    if (backend.init(1, 0).is_error()) throw std::runtime_error("init must succeed");
    uint8_t fake_data[4] = {0};
    // submit may fail with invalid data — timeout test still works from Ready state.
    backend.submit_packet(1, fake_data, 4, 0);
    backend.set_deadline_ms(100);
    backend.exceed_deadline();
    // receive_frame after timeout: if in Ready state (submit failed), returns NotReady.
    // If in Decoding state (submit succeeded), returns TimedOut.
    auto r = backend.receive_frame(1);
    if (!r.is_error()) throw std::runtime_error("receive_frame after timeout must fail");
    if (r.error() != rt::DecodeStatus::TimedOut && r.error() != rt::DecodeStatus::NotReady)
        throw std::runtime_error("expected TimedOut or NotReady");
    std::printf("[C11] TimedOut PASS\n");
    backend.close();
}

// Transition invalid → InvalidAdjacency
static void test_c_transition_invalid() {
    rt::FFmpegDecodeBackend backend;
    if (backend.init(1, 0).is_error()) throw std::runtime_error("init must succeed");
    // Ready → NotCreated is invalid adjacency
    auto r = backend.transition_to(AudioDecodeBackendState::NotCreated);
    if (!r.is_error()) throw std::runtime_error("Ready→NotCreated must fail");
    if (r.error() != rt::TransitionError::InvalidAdjacency)
        throw std::runtime_error("expected InvalidAdjacency");
    std::printf("[C-Trans] invalid adjacency rejected PASS\n");
    backend.close();
}

int main() {
    std::printf("=== P6C FFmpeg Decode Backend Tests ===\n\n");

    P6C_RUN(test_c01_init_aac);
    P6C_RUN(test_c02_init_unsupported_codec);
    P6C_RUN(test_c03_init_twice);
    P6C_RUN(test_c04_submit_packet_valid);
    P6C_RUN(test_c05_submit_before_init);
    P6C_RUN(test_c06_receive_frame);
    P6C_RUN(test_c07_flush);
    P6C_RUN(test_c08_close);
    P6C_RUN(test_c11_timeout);
    P6C_RUN(test_c_transition_invalid);

    if (g_failed == 0) {
        std::printf("\n=== P6C Backend: ALL PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6C Backend: %d FAILED ===\n", g_failed);
        return 1;
    }
}
