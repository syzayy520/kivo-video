// array_texture_resolver.hpp -- P5G G2: array texture / slice resolver
#pragma once
#include <cstdint>
#include <vector>

namespace kivo::video_plane::gpu_hw {

struct ArrayTextureSlice {
    uint32_t array_index{0};
    uint32_t mip_level{0};
    bool is_valid{false};
};

class FakeArrayTextureResolver {
public:
    void configure(uint32_t array_size, uint32_t mip_levels) {
        array_size_ = array_size;
        mip_levels_ = mip_levels;
    }

    ArrayTextureSlice resolve(uint32_t array_index, uint32_t mip_level) const {
        ArrayTextureSlice s;
        s.array_index = array_index;
        s.mip_level = mip_level;
        s.is_valid = array_index < array_size_ && mip_level < mip_levels_;
        return s;
    }

    uint32_t array_size() const { return array_size_; }
    uint32_t mip_levels() const { return mip_levels_; }

private:
    uint32_t array_size_{0};
    uint32_t mip_levels_{0};
};

}  // namespace kivo::video_plane::gpu_hw
