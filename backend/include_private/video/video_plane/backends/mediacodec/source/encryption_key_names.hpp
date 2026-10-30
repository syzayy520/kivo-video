#pragma once

#include <cstdint>

namespace kivo::video::mediacodec::crypto_key {

/// 轨道格式里的加密键（P12-F3B）。
///
/// **为什么集中在一处**：这些字符串是平台与容器之间的契约，写错不会有编译错误，
/// 只会让判定静默变成"永远不加密" —— 和 E1 那批颜色常量同一个坑。
/// 实测值来自 K40 / Android 13 上 `AMediaFormat_toString()` 的输出，见
/// `docs/evidence/p12f3b/`。

/// `crypto-mode`（int32）。**主判据**。
/// API 21 起存在；实测三份 CENC 样本（非分片/带 pssh/分片）全部给出。
constexpr const char* kKeyCryptoMode = "crypto-mode";

/// `crypto-key`（data）。**API 28 才有 `AMediaFormat_getBuffer`**，而 minSdk = 26，
/// 所以它只作佐证、不作主判据 —— 直接读会在 26/27 上加载期解析失败。
constexpr const char* kKeyCryptoKey = "crypto-key";

/// `crypto-default-iv-size`（int32）。作佐证：加密轨几乎必然带 IV 长度。
constexpr const char* kKeyCryptoDefaultIvSize = "crypto-default-iv-size";

/// `crypto-mode` 的已知取值。来自 Stagefright：0 表示不加密，1 = AES-CTR，2 = AES-CBC。
constexpr std::int32_t kCryptoModeClear = 0;
constexpr std::int32_t kCryptoModeAesCtr = 1;
constexpr std::int32_t kCryptoModeAesCbc = 2;

/// 给证据用的可读名。**未知值必须原样回报**，不能硬套成 clear/ctr ——
/// 那会让一个我们没见过的模式看起来像已知的。
[[nodiscard]] const char* crypto_mode_name(std::int32_t mode) noexcept;

}  // namespace kivo::video::mediacodec::crypto_key
