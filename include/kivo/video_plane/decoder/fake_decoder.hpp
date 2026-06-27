// fake_decoder.hpp — P5D D2: Fake decoder produces frames
#pragma once
#include <cstdint>
#include <vector>
#include <optional>
#include "decoder_contract.hpp"
#include "../stamp/core_video_stamp.hpp"

namespace kivo::video_plane::decoder {

struct DecodedFrame {
    CoreVideoStamp stamp;
    uint64_t width{0};
    uint64_t height{0};
    uint64_t pts{0};
    bool is_keyframe{false};
};

class FakeDecoder {
public:
    explicit FakeDecoder(DecoderContract const& contract)
        : contract_(contract) {}

    bool initialize() { initialized_ = true; return true; }

    std::optional<DecodedFrame> decode_next() {
        if (!initialized_) return std::nullopt;
        if (frame_index_ >= 100) return std::nullopt;  // 100 frames
        DecodedFrame f;
        f.stamp = CoreVideoStamp{++next_stamp_};
        f.width = 1920;
        f.height = 1080;
        f.pts = frame_index_ * 33;
        f.is_keyframe = (frame_index_ % 30 == 0);
        ++frame_index_;
        return f;
    }

    void flush() { frame_index_ = 0; }
    void drain() { draining_ = true; }
    bool is_draining() const { return draining_; }
    uint64_t frame_count() const { return frame_index_; }
    DecoderContract const& contract() const { return contract_; }

private:
    DecoderContract contract_;
    bool initialized_{false};
    uint64_t frame_index_{0};
    uint64_t next_stamp_{0};
    bool draining_{false};
};

}  // namespace kivo::video_plane::decoder
