#ifndef KIVO_VIDEO_SUBTITLE_PLANE_STREAM_SUBTITLE_EMBEDDED_PACKET_H
#define KIVO_VIDEO_SUBTITLE_PLANE_STREAM_SUBTITLE_EMBEDDED_PACKET_H

#include "kivo/video/subtitle_plane/command/subtitle_command.h"
#include "kivo/video/subtitle_plane/format/subtitle_format.h"
#include "kivo/video/subtitle_plane/generation/subtitle_generation.h"
#include "kivo/video/subtitle_plane/input/subtitle_input_views.h"
#include "kivo/video/subtitle_plane/source/subtitle_source.h"
#include "kivo/video/subtitle_plane/types/subtitle_time.h"

KIVO_P8_EXTERN_C_BEGIN

typedef enum KivoEmbeddedSubtitlePacketFlag {
    KivoEmbeddedSubtitlePacketFlag_None = 0,
    KivoEmbeddedSubtitlePacketFlag_KeyPacket = 1u << 0u,
    KivoEmbeddedSubtitlePacketFlag_Discontinuity = 1u << 1u
} KivoEmbeddedSubtitlePacketFlag;

typedef struct KivoEmbeddedSubtitlePacketSubmitRequest {
    KivoP8AbiHeader header;
    KivoSubtitleCorrelationId correlation_id;
    KivoSubtitleSourceId source_id;
    KivoSubtitlePacketGeneration packet_generation;
    KivoSubtitleFormat format;
    uint32_t flags;
    KivoSubtitleTime start_time;
    KivoSubtitleTime end_time;
    KivoSubtitleInputBytesView packet_bytes;
    uint64_t reserved0;
} KivoEmbeddedSubtitlePacketSubmitRequest;

KIVO_P8_API KivoP8ApiStatus KIVO_P8_CALL kivo_subtitle_plane_submit_embedded_packet_v1(
    KivoSubtitlePlaneHandle plane,
    const KivoEmbeddedSubtitlePacketSubmitRequest* request,
    KivoSubtitleCommandToken* out_token) KIVO_P8_NOEXCEPT;

KIVO_P8_EXTERN_C_END

#endif
