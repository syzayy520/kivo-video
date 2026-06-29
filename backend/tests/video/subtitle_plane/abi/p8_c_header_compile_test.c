#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

_Static_assert(KIVO_P8_ABI_MAJOR == 1u, "P8 ABI major mismatch");
_Static_assert(KIVO_P8_ABI_MINOR == 5u, "P8 ABI minor mismatch");
_Static_assert(KIVO_P8_ABI_PATCH == 5u, "P8 ABI patch mismatch");
_Static_assert(KIVO_P8_ABI_REVISION == 155u, "P8 ABI revision mismatch");
_Static_assert(KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE > 0u, "P8 prefix missing");
_Static_assert(offsetof(KivoSubtitlePlaneApiV1, header) == 0u, "API header offset");

int main(void) {
    KivoSubtitlePlaneApiV1 api;
    api.header.struct_size = sizeof(api);
    return api.header.struct_size == sizeof(api) ? 0 : 1;
}
