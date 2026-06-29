#include "video/subtitle_plane/snapshot/snapshot_blob_writer.hpp"

#include <bit>
#include <cstring>

namespace kivo::video::subtitle_plane::runtime {
namespace {

constexpr uint64_t kSectionCount = 4;

static_assert(std::endian::native == std::endian::little);

}  // namespace

uint64_t snapshot_blob_required_bytes() noexcept {
    return sizeof(KivoSubtitleSnapshotBlobHeader) +
           kSectionCount * sizeof(KivoSubtitleSnapshotBlobSection);
}

KivoP8ApiStatus write_snapshot_blob(
    const PlaneStateRecord& record,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) noexcept {
    if (out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    const uint64_t required = snapshot_blob_required_bytes();
    *out_required_bytes = required;
    if (dst.data == nullptr) return dst.bytes == 0 ? KivoP8ApiStatus_Ok : KivoP8ApiStatus_InvalidArgument;
    if (dst.bytes < required) return KivoP8ApiStatus_InvalidArgument;

    KivoSubtitleSnapshotBlobHeader header{};
    header.magic = KIVO_P8_SNAPSHOT_BLOB_MAGIC;
    header.endian = KIVO_P8_SNAPSHOT_ENDIAN_LITTLE;
    header.header_size = sizeof(KivoSubtitleSnapshotBlobHeader);
    header.abi_major = KIVO_P8_ABI_MAJOR;
    header.blob_revision = KIVO_P8_SNAPSHOT_BLOB_REVISION;
    header.total_bytes = required;
    header.section_count = kSectionCount;
    header.section_table_offset = sizeof(KivoSubtitleSnapshotBlobHeader);
    header.state = record.state;
    header.revision = record.snapshot_revision;
    header.evidence_summary = record.evidence_summary;
    header.metrics = record.metrics;

    KivoSubtitleSnapshotBlobSection sections[kSectionCount]{};
    sections[0].kind = KivoSubtitleSnapshotSection_TrackArray;
    sections[1].kind = KivoSubtitleSnapshotSection_EvidenceSummaryArray;
    sections[1].element_size = sizeof(KivoSubtitleEvidenceSummary);
    sections[1].count = 1;
    sections[1].offset = 0;
    sections[2].kind = KivoSubtitleSnapshotSection_FontAttachmentArray;
    sections[3].kind = KivoSubtitleSnapshotSection_CommandSummaryArray;

    std::memcpy(dst.data, &header, sizeof(header));
    std::memcpy(dst.data + sizeof(header), sections, sizeof(sections));
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
