#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace {

struct ObserverProbe {
    KivoSubtitlePlaneApiV1* api = nullptr;
    KivoSubtitlePlaneHandle plane{};
    KivoSubtitleSubscriptionToken token{};
    int calls = 0;
    KivoP8ApiStatus mutation_status = KivoP8ApiStatus_Ok;
    KivoP8ApiStatus unsubscribe_status = KivoP8ApiStatus_Ok;
};

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

void KIVO_P8_CALL on_event(void* user_data, const KivoSubtitleEvent* event) KIVO_P8_NOEXCEPT {
    auto* probe = static_cast<ObserverProbe*>(user_data);
    probe->calls += event != nullptr ? 1 : 0;
    KivoSubtitleCommandToken token{};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_EmbeddedTrack;
    probe->mutation_status =
        probe->api->set_source(probe->plane, &source, KivoSubtitleCorrelationId{44}, &token);
    probe->unsubscribe_status = probe->api->unsubscribe_events(probe->plane, probe->token);
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();
    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(nullptr, &plane) != KivoP8ApiStatus_Ok) return 1;

    KivoSubtitleSubscriptionToken token{};
    if (api.subscribe_events(plane, nullptr, &token) != KivoP8ApiStatus_InvalidArgument) return 2;
    if (token.value != 0) return 3;

    ObserverProbe probe{};
    probe.api = &api;
    probe.plane = plane;
    KivoSubtitleObserverCallbacks callbacks{};
    callbacks.user_data = &probe;
    callbacks.on_event = on_event;
    if (api.subscribe_events(plane, &callbacks, &probe.token) != KivoP8ApiStatus_Ok) return 4;
    if (probe.token.value == 0) return 5;

    KivoSubtitleSubscriptionToken duplicate{};
    if (api.subscribe_events(plane, &callbacks, &duplicate) != KivoP8ApiStatus_Failed) return 6;

    KivoEmbeddedSubtitleStreamEndRequest stream_end{};
    stream_end.packet_generation.packet_generation = 0;
    KivoSubtitleCommandToken command{};
    if (api.end_embedded_stream(plane, &stream_end, &command) != KivoP8ApiStatus_Ok) return 7;
    if (probe.calls != 1) return 8;
    if (probe.mutation_status != KivoP8ApiStatus_UnsupportedOperation) return 9;
    if (probe.unsubscribe_status != KivoP8ApiStatus_Ok) return 10;

    stream_end.packet_generation.packet_generation = 1;
    if (api.end_embedded_stream(plane, &stream_end, &command) != KivoP8ApiStatus_Ok) return 11;
    if (probe.calls != 1) return 12;

    KivoSubtitleMetricsSnapshot metrics{};
    if (api.get_metrics(plane, &metrics) != KivoP8ApiStatus_Ok) return 13;
    if (metrics.observer_queue_overflow_count != 1) return 14;
    if (metrics.evidence_overflow_count != 0) return 15;

    api.destroy_plane(plane);
    return 0;
}
