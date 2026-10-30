#include "video/video_plane/backends/mediacodec/device/mediacodec_device_identity.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_system_property_reader.hpp"

namespace kivo::video::mediacodec {

DeviceIdentity read_device_identity() {
    DeviceIdentity identity;
    identity.manufacturer = read_system_property("ro.product.manufacturer");
    identity.model = read_system_property("ro.product.model");
    identity.device = read_system_property("ro.product.device");
    identity.soc_model = read_system_property("ro.soc.model");
    identity.board_platform = read_system_property("ro.board.platform");
    identity.sdk_int = read_system_property("ro.build.version.sdk");
    identity.release = read_system_property("ro.build.version.release");
    identity.abi = read_system_property("ro.product.cpu.abi");
    identity.vulkan = read_system_property("ro.hardware.vulkan");
    return identity;
}

}  // namespace kivo::video::mediacodec
