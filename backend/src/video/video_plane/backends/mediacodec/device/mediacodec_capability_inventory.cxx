#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_inventory.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_bit_depth.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_record_parser.hpp"

#include <algorithm>
#include <set>
#include <string_view>

namespace kivo::video::mediacodec {
namespace {

constexpr std::int32_t kHardwareYes = 1;
constexpr std::int32_t kTenBitThreshold = 10;

/// 去重键：同一 (name, mime, profile, level) 只保留一条。
[[nodiscard]] std::string record_key(const CodecInventoryRecord& record) {
    return record.name + '|' + record.mime + '|' + std::to_string(record.profile) + '|' +
           std::to_string(record.level);
}

[[nodiscard]] bool record_less(const CodecInventoryRecord& lhs,
                               const CodecInventoryRecord& rhs) {
    if (lhs.mime != rhs.mime) {
        return lhs.mime < rhs.mime;
    }
    if (lhs.name != rhs.name) {
        return lhs.name < rhs.name;
    }
    return lhs.profile < rhs.profile;
}

}  // namespace

CapabilityInventory build_capability_inventory(
    const std::vector<std::string>& raw_records) {
    CapabilityInventory inventory;
    inventory.raw_record_count = raw_records.size();

    std::set<std::string> seen_keys;
    inventory.records.reserve(raw_records.size());

    for (const std::string& raw : raw_records) {
        CodecInventoryRecord record;
        if (!parse_codec_record(raw, record)) {
            ++inventory.rejected_records;
            continue;
        }
        if (!seen_keys.insert(record_key(record)).second) {
            continue;
        }
        inventory.records.push_back(std::move(record));
    }

    std::sort(inventory.records.begin(), inventory.records.end(), record_less);

    std::set<std::string> mimes;
    for (const CodecInventoryRecord& record : inventory.records) {
        if (record.is_encoder) {
            continue;
        }
        ++inventory.decoder_records;
        mimes.insert(record.mime);
        if (record.is_hardware == kHardwareYes) {
            ++inventory.hardware_decoder_records;
        }
        if (infer_bit_depth(record.mime, record.profile) >= kTenBitThreshold) {
            ++inventory.tenbit_decoder_records;
        }
    }

    inventory.mimes.assign(mimes.begin(), mimes.end());
    return inventory;
}

}  // namespace kivo::video::mediacodec
