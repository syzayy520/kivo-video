#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_probe.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_inventory.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_json_renderer.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_device_identity.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_wall_clock.hpp"

#include <fstream>
#include <string_view>

namespace kivo::video::mediacodec {
namespace {

constexpr std::int32_t kCodeOk = 0;
constexpr std::int32_t kCodeEmptyInventory = 1;
constexpr std::int32_t kCodeEmptyOutputPath = 2;
constexpr std::int32_t kCodeAllRejected = 3;
constexpr std::int32_t kCodeOpenFailed = 4;
constexpr std::int32_t kCodeWriteFailed = 5;

/// 落盘结果。刻意区分「打不开」与「写不进」：
/// 前者是路径/权限问题，后者是磁盘或文件系统问题，排查方向完全不同。
enum class WriteOutcome {
    kOk,
    kOpenFailed,
    kWriteFailed,
};

[[nodiscard]] WriteOutcome write_file(const std::string& path, std::string_view content) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return WriteOutcome::kOpenFailed;
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    if (!file) {
        return WriteOutcome::kWriteFailed;
    }
    return WriteOutcome::kOk;
}

void copy_counts(const CapabilityInventory& inventory, CapabilityProbeOutcome& outcome) {
    outcome.total_records = inventory.raw_record_count;
    outcome.decoder_records = inventory.decoder_records;
    outcome.hardware_decoder_records = inventory.hardware_decoder_records;
    outcome.decoder_10bit_records = inventory.tenbit_decoder_records;
}

}  // namespace

CapabilityProbeOutcome probe_capabilities(const std::vector<std::string>& raw_records,
                                          const std::string& output_path) {
    CapabilityProbeOutcome outcome;

    if (raw_records.empty()) {
        outcome.code = kCodeEmptyInventory;
        outcome.message =
            "inventory is empty: platform MediaCodecList returned no video codec";
        return outcome;
    }
    if (output_path.empty()) {
        outcome.code = kCodeEmptyOutputPath;
        outcome.message = "output_path is empty";
        return outcome;
    }

    const CapabilityInventory inventory = build_capability_inventory(raw_records);
    copy_counts(inventory, outcome);

    if (inventory.records.empty()) {
        outcome.code = kCodeAllRejected;
        outcome.message =
            "all " + std::to_string(inventory.rejected_records) + " records failed to parse";
        return outcome;
    }

    const std::string json =
        render_capability_json(read_device_identity(), inventory, now_unix_millis());

    switch (write_file(output_path, json)) {
        case WriteOutcome::kOpenFailed:
            outcome.code = kCodeOpenFailed;
            outcome.message = "cannot open evidence path for writing: " + output_path;
            return outcome;
        case WriteOutcome::kWriteFailed:
            outcome.code = kCodeWriteFailed;
            outcome.message = "write failed after open (disk full?): " + output_path;
            return outcome;
        case WriteOutcome::kOk:
            break;
    }

    outcome.code = kCodeOk;
    outcome.message = "ok";
    return outcome;
}

}  // namespace kivo::video::mediacodec
