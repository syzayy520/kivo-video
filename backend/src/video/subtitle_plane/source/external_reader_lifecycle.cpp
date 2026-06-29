#include "video/subtitle_plane/source/external_reader_lifecycle.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitlePlaneError probe_external_reader_once(
    const KivoSubtitleSourceDescriptor& source) noexcept {
    if (source.kind != KivoSubtitleSourceKind_Reader &&
        source.kind != KivoSubtitleSourceKind_ProviderByteStream) {
        return KivoSubtitlePlaneError_None;
    }

    KivoSubtitleSourceHandle handle{};
    if (source.reader.open(source.reader.user_data, &source, &handle) != KivoP8CallbackStatus_Ok) {
        return KivoSubtitlePlaneError_SubtitleSourceOpenFailed;
    }

    uint8_t scratch[8]{};
    uint64_t bytes_read = 0;
    const KivoP8CallbackStatus read_status =
        source.reader.read(source.reader.user_data, handle, scratch, sizeof(scratch), &bytes_read);
    if (source.reader.cancel != nullptr) {
        (void)source.reader.cancel(source.reader.user_data, handle);
    }
    source.reader.close(source.reader.user_data, handle);
    return read_status == KivoP8CallbackStatus_Ok
        ? KivoSubtitlePlaneError_None
        : KivoSubtitlePlaneError_SubtitleSourceReadFailed;
}

}  // namespace kivo::video::subtitle_plane::runtime
