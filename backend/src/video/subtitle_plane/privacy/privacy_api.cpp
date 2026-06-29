#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/privacy/redacted_identity.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_compute_redacted_source_identity(
    const KivoSubtitlePrivacyPolicy* const privacy,
    const KivoSubtitleSourceDescriptor* const source,
    KivoRedactedSourceIdentity* const out_identity) KIVO_P8_NOEXCEPT {
    if (source == nullptr) return KivoP8ApiStatus_NullPointer;
    return compute_redacted_identity(privacy, *source, out_identity);
}

}  // namespace kivo::video::subtitle_plane::runtime
