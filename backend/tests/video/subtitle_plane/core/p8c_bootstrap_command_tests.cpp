#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace {

KivoSubtitlePlaneApiV1 request_api(uint32_t revision = KIVO_P8_ABI_REVISION) {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    const KivoP8ApiStatus status = kivo_subtitle_plane_get_api_v1(revision, &api);
    if (status != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

KivoP8CallbackStatus KIVO_P8_CALL cancel_now(void*, uint64_t, uint32_t* out_cancelled) KIVO_P8_NOEXCEPT {
    *out_cancelled = 1;
    return KivoP8CallbackStatus_Ok;
}

}  // namespace

int main() {
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, nullptr) != KivoP8ApiStatus_NullPointer) {
        return 1;
    }

    KivoSubtitlePlaneApiV1 too_small{};
    too_small.header.struct_size = 1;
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &too_small) != KivoP8ApiStatus_ApiTableTooSmall) {
        return 2;
    }

    KivoSubtitlePlaneApiV1 wrong_revision{};
    wrong_revision.header.struct_size = sizeof(wrong_revision);
    if (kivo_subtitle_plane_get_api_v1(1, &wrong_revision) != KivoP8ApiStatus_UnsupportedAbiVersion) {
        return 3;
    }

    KivoSubtitlePlaneApiV1 api = request_api();
    if (api.header.struct_size == 0 || api.header.abi_revision != KIVO_P8_ABI_REVISION) {
        return 4;
    }
    if (api.create_plane == nullptr || api.query_command == nullptr || api.build_frame == nullptr) {
        return 5;
    }

    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(nullptr, &plane) != KivoP8ApiStatus_Ok || plane.value == 0) {
        return 6;
    }

    KivoSubtitleCommandLifecycleSnapshot rejected{};
    if (api.query_command(plane, KivoSubtitleCommandId{}, &rejected) != KivoP8ApiStatus_Ok ||
        rejected.state != KivoSubtitleCommandLifecycleState_Failed) {
        return 7;
    }

    KivoSubtitleDiscoveryBeginRequest discovery{};
    discovery.correlation_id.value = 77;
    KivoSubtitleCommandToken token{};
    if (api.begin_discovery(plane, &discovery, &token) != KivoP8ApiStatus_Ok ||
        token.command_id.value == 0) {
        return 8;
    }

    KivoSubtitleCommandLifecycleSnapshot accepted{};
    if (api.query_command(plane, token.command_id, &accepted) != KivoP8ApiStatus_Ok ||
        accepted.state != KivoSubtitleCommandLifecycleState_Completed ||
        accepted.correlation_id.value != 77) {
        return 9;
    }

    KivoSubtitleFrameBuildRequest frame_request{};
    frame_request.playback_revision.value = 0;
    frame_request.cancellation.is_cancelled = cancel_now;
    KivoSubtitleFrameLease lease{};
    if (api.build_frame(plane, &frame_request, &lease) != KivoP8ApiStatus_NotReady) {
        return 10;
    }

    KivoSubtitlePlaneConfig bad_config{};
    bad_config.caps.max_packet_queue_bytes = UINT64_MAX;
    KivoSubtitlePlaneHandle bad_plane{};
    if (api.create_plane(&bad_config, &bad_plane) != KivoP8ApiStatus_InvalidArgument) {
        return 11;
    }

    if (api.close_plane(plane) != KivoP8ApiStatus_Ok || api.destroy_plane(plane) != KivoP8ApiStatus_Ok) {
        return 12;
    }

    return 0;
}
