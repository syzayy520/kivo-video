#include "video/subtitle_plane/source/embedded_packet_ingest.hpp"

#include <algorithm>
#include <cstring>

#include "video/subtitle_plane/command/command_lifecycle.hpp"
#include "video/subtitle_plane/format/format_track_classification.hpp"

namespace kivo::video::subtitle_plane::runtime {
namespace {

[[nodiscard]] bool valid_time_range(const KivoSubtitleTime start,
                                    const KivoSubtitleTime end) noexcept {
    return start.microseconds != KIVO_SUBTITLE_TIME_INVALID &&
           end.microseconds != KIVO_SUBTITLE_TIME_INVALID &&
           end.microseconds > start.microseconds;
}

[[nodiscard]] bool valid_payload(const KivoSubtitleInputBytesView bytes) noexcept {
    return bytes.bytes == 0 || bytes.data != nullptr;
}

void clear_active_cue(PlaneStateRecord& record) noexcept {
    record.active_text_cue = false;
    record.active_bitmap_packet = false;
    record.active_text_bytes = 0;
    record.active_text[0] = 0;
}

void clear_text_cues(PlaneStateRecord& record) noexcept {
    record.text_cue_count = 0;
    record.next_text_cue_slot = 0;
    for (std::size_t index = 0; index < PlaneStateRecord::kMaxTextCues; ++index) {
        record.text_cue_valid[index] = false;
        record.text_cue_bytes[index] = 0;
        record.text_cue_text[index][0] = 0;
        record.text_cue_start_times[index] = {};
        record.text_cue_end_times[index] = {};
        record.text_cue_formats[index] = KivoSubtitleFormat_Unknown;
    }
}

void append_text_cue(PlaneStateRecord& record,
                     const KivoSubtitleTime start,
                     const KivoSubtitleTime end,
                     const KivoSubtitleFormat format,
                     const KivoSubtitleInputBytesView bytes) noexcept {
    const std::size_t slot = static_cast<std::size_t>(
        record.next_text_cue_slot % PlaneStateRecord::kMaxTextCues);
    const uint64_t copy_bytes = std::min<uint64_t>(
        bytes.bytes, static_cast<uint64_t>(PlaneStateRecord::kMaxTextCueBytes - 1u));
    if (copy_bytes > 0) {
        std::memcpy(record.text_cue_text[slot], bytes.data,
                    static_cast<std::size_t>(copy_bytes));
    }
    record.text_cue_text[slot][copy_bytes] = 0;
    record.text_cue_bytes[slot] = copy_bytes;
    record.text_cue_start_times[slot] = start;
    record.text_cue_end_times[slot] = end;
    record.text_cue_formats[slot] = format;
    record.text_cue_valid[slot] = copy_bytes > 0;
    record.next_text_cue_slot = (record.next_text_cue_slot + 1u) %
                                PlaneStateRecord::kMaxTextCues;
    if (record.text_cue_count < PlaneStateRecord::kMaxTextCues) {
        ++record.text_cue_count;
    }
}

void store_text_cue(PlaneStateRecord& record,
                    const KivoSubtitleTime start,
                    const KivoSubtitleTime end,
                    const KivoSubtitleFormat format,
                    const KivoSubtitleInputBytesView bytes) noexcept {
    clear_active_cue(record);
    const uint64_t copy_bytes = std::min<uint64_t>(
        bytes.bytes, static_cast<uint64_t>(sizeof(record.active_text) - 1u));
    if (copy_bytes > 0) {
        std::memcpy(record.active_text, bytes.data, static_cast<std::size_t>(copy_bytes));
    }
    record.active_text[copy_bytes] = 0;
    record.active_text_bytes = copy_bytes;
    record.active_start_time = start;
    record.active_end_time = end;
    record.active_format = format;
    record.active_text_cue = copy_bytes > 0;
    append_text_cue(record, start, end, format, bytes);
    record.timeline = KivoSubtitleTimelineAvailability_Available;
    record.state = KivoSubtitlePlaneState_Ready;
    record.metrics.cue_count += record.active_text_cue ? 1u : 0u;
    record.metrics.visible_cue_count = record.active_text_cue ? 1u : 0u;
    record.metrics.text_pool_bytes = copy_bytes;
}

void store_bitmap_packet(PlaneStateRecord& record,
                         const KivoSubtitleTime start,
                         const KivoSubtitleTime end,
                         const KivoSubtitleFormat format) noexcept {
    clear_active_cue(record);
    record.active_start_time = start;
    record.active_end_time = end;
    record.active_format = format;
    record.active_bitmap_packet = true;
    record.timeline = KivoSubtitleTimelineAvailability_Available;
    record.state = KivoSubtitlePlaneState_Ready;
    record.metrics.unsupported_feature_count += 1u;
    record.metrics.visible_cue_count = 0;
}

}  // namespace

void store_memory_text_source(PlaneStateRecord& record,
                              const KivoSubtitleInputBytesView bytes) noexcept {
    clear_text_cues(record);
    KivoSubtitleTime start{};
    KivoSubtitleTime end{};
    start.microseconds = 0;
    end.microseconds = 5'000'000;
    store_text_cue(record, start, end, KivoSubtitleFormat_Srt, bytes);
    record.metrics.packet_queue_depth = bytes.bytes > 0 ? 1u : 0u;
    record.metrics.packet_queue_bytes = bytes.bytes;
    record.metrics.peak_packet_queue_bytes = bytes.bytes;
}

KivoP8ApiStatus ingest_embedded_packet(
    PlaneStateRecord& record,
    const KivoEmbeddedSubtitlePacketSubmitRequest& request,
    KivoSubtitleCommandToken* const out_token) noexcept {
    if (out_token == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    *out_token = KivoSubtitleCommandToken{};

    if (!valid_payload(request.packet_bytes) ||
        !valid_time_range(request.start_time, request.end_time) ||
        request.format == KivoSubtitleFormat_Unknown ||
        request.format == KivoSubtitleFormat_Unsupported) {
        *out_token =
            reject_command(request.correlation_id, KivoSubtitlePlaneError_InvalidArgument);
        return KivoP8ApiStatus_InvalidArgument;
    }

    record.metrics.packet_queue_depth += 1u;
    record.metrics.packet_queue_bytes += request.packet_bytes.bytes;
    record.metrics.peak_packet_queue_bytes =
        std::max(record.metrics.peak_packet_queue_bytes, record.metrics.packet_queue_bytes);

    if (subtitle_format_is_bitmap(request.format)) {
        store_bitmap_packet(record, request.start_time, request.end_time, request.format);
        *out_token = accept_command(record, request.correlation_id);
        return KivoP8ApiStatus_Ok;
    }

    if (!subtitle_format_is_text(request.format)) {
        record.metrics.packet_drop_count += 1u;
        *out_token = reject_command(request.correlation_id,
                                    KivoSubtitlePlaneError_UnsupportedSubtitleFormat);
        return KivoP8ApiStatus_UnsupportedOperation;
    }

    store_text_cue(record,
                   request.start_time,
                   request.end_time,
                   request.format,
                   request.packet_bytes);
    *out_token = accept_command(record, request.correlation_id);
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
