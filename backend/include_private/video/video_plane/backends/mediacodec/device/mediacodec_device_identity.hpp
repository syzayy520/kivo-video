#pragma once

#include <string>

namespace kivo::video::mediacodec {

/// 设备身份指纹，写入证据文件头，用于把产物和具体机器绑定。
///
/// 全部字段来自只读系统属性。缺失一律填 "unknown"，不留空串 ——
/// 空串在 JSON 里容易被误读为「该属性存在但为空」。
struct DeviceIdentity {
    std::string manufacturer;
    std::string model;
    std::string device;
    std::string soc_model;
    std::string board_platform;
    std::string sdk_int;
    std::string release;
    std::string abi;
    std::string vulkan;
};

/// 采集本机身份。每条属性读取失败时对应字段为 "unknown"。
[[nodiscard]] DeviceIdentity read_device_identity();

}  // namespace kivo::video::mediacodec
