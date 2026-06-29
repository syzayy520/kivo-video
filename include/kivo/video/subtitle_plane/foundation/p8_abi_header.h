#ifndef KIVO_VIDEO_SUBTITLE_PLANE_FOUNDATION_P8_ABI_HEADER_H
#define KIVO_VIDEO_SUBTITLE_PLANE_FOUNDATION_P8_ABI_HEADER_H

#include "kivo/video/subtitle_plane/foundation/p8_status.h"

typedef struct KivoP8AbiHeader {
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;
    uint32_t abi_patch;
    uint32_t abi_revision;
    uint32_t flags;
    uint64_t reserved0;
} KivoP8AbiHeader;

#endif
