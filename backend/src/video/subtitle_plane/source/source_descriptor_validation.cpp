#include "video/subtitle_plane/source/source_descriptor_validation.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitlePlaneError validate_source_descriptor(
    const KivoSubtitleSourceDescriptor& source) noexcept {
    if (source.kind == KivoSubtitleSourceKind_Unknown) {
        return KivoSubtitlePlaneError_MalformedInput;
    }
    if (source.kind == KivoSubtitleSourceKind_FilePathUtf8 &&
        source.file_path_utf8.utf8 == nullptr &&
        source.file_path_utf8.utf8_bytes > 0) {
        return KivoSubtitlePlaneError_MalformedInput;
    }
    if (source.kind == KivoSubtitleSourceKind_MemoryBytes &&
        source.memory_bytes.data == nullptr &&
        source.memory_bytes.bytes > 0) {
        return KivoSubtitlePlaneError_MalformedInput;
    }
    if ((source.kind == KivoSubtitleSourceKind_Reader ||
         source.kind == KivoSubtitleSourceKind_ProviderByteStream) &&
        (source.reader.open == nullptr ||
         source.reader.read == nullptr ||
         source.reader.close == nullptr)) {
        return KivoSubtitlePlaneError_MalformedInput;
    }
    return KivoSubtitlePlaneError_None;
}

}  // namespace kivo::video::subtitle_plane::runtime
