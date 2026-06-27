// adapter_topology.hpp -- P5G G9: adapter topology
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::gpu_hw {

struct AdapterInfo {
    uint64_t luid{0};
    std::string vendor_name;
    uint32_t device_id{0};
    bool is_primary{false};
    bool supports_hw_decode{false};
};

class FakeAdapterTopology {
public:
    void add_adapter(uint64_t luid, const std::string& vendor, uint32_t device_id,
                     bool is_primary, bool supports_hw_decode) {
        adapters_.push_back({luid, vendor, device_id, is_primary, supports_hw_decode});
    }

    AdapterInfo const* find_adapter(uint64_t luid) const {
        for (auto const& a : adapters_) {
            if (a.luid == luid) return &a;
        }
        return nullptr;
    }

    AdapterInfo const* primary_adapter() const {
        for (auto const& a : adapters_) {
            if (a.is_primary) return &a;
        }
        return nullptr;
    }

    size_t adapter_count() const { return adapters_.size(); }
    std::vector<AdapterInfo> const& adapters() const { return adapters_; }

    bool is_single_gpu() const { return adapters_.size() == 1; }
    bool is_multi_gpu() const { return adapters_.size() > 1; }

private:
    std::vector<AdapterInfo> adapters_;
};

}  // namespace kivo::video_plane::gpu_hw
