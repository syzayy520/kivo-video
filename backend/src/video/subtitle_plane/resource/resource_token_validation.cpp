#include "video/subtitle_plane/resource/resource_token_validation.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus validate_resource_access_token(
    const KivoSubtitleResourceAccessToken token,
    const KivoSubtitleResourceAccessKind expected_kind) noexcept {
    if (token.kind != expected_kind || token.snapshot_revision.value == 0) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (token.kind == KivoSubtitleResourceAccessKind_FrameLease) {
        return token.lease_id != 0 &&
                   token.lease_generation != 0 &&
                   token.snapshot_lease_id.value == 0 &&
                   token.snapshot_lease_generation == 0
            ? KivoP8ApiStatus_Ok
            : KivoP8ApiStatus_InvalidArgument;
    }
    if (token.kind == KivoSubtitleResourceAccessKind_SnapshotLease) {
        return token.snapshot_lease_id.value != 0 &&
                   token.snapshot_lease_generation != 0 &&
                   token.lease_id == 0 &&
                   token.lease_generation == 0
            ? KivoP8ApiStatus_Ok
            : KivoP8ApiStatus_InvalidArgument;
    }
    return KivoP8ApiStatus_InvalidArgument;
}

}  // namespace kivo::video::subtitle_plane::runtime
