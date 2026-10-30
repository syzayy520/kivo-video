#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_inventory.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_device_identity.hpp"

namespace kivo::video::mediacodec {

/// 把设备身份与能力清单渲染成 JSON 文本（schema: kivo.p12a.capabilities/1）。
///
/// 纯函数：给定相同入参必然产出逐字节相同的结果，不读时钟、不读系统属性、
/// 不写文件。时间戳由调用方传入。
[[nodiscard]] std::string render_capability_json(const DeviceIdentity& identity,
                                                 const CapabilityInventory& inventory,
                                                 std::int64_t generated_at_unix_ms);

/// 本渲染器产出的 schema 标识。测试与下游解析器共用，避免字面量散落。
[[nodiscard]] const char* capability_schema_id();

}  // namespace kivo::video::mediacodec
