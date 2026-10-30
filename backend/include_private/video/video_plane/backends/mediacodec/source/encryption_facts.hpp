#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// 一次解封装的加密事实（P12-F3B）。
///
/// 全是整数与 bool，**不含 `AMediaFormat`** —— 取值在 `source/mediaformat_encryption` 里做，
/// 判定在这里做。这样判定就能在桌面上被单测，而不用构造一个平台格式。
///
/// 数值约定：拿不到写 -1，**未知与"等于 0"必须分开**。
/// `crypto-mode = 0` 是"明确声明不加密"，`-1` 是"平台没说" —— 两者的处理不一样。
struct TrackEncryption {
    /// `crypto-mode` 存在且读到了。
    bool mode_present = false;
    /// `crypto-mode` 的值；`mode_present == false` 时保持 -1。
    std::int32_t crypto_mode = -1;
    /// `crypto-default-iv-size`；拿不到 -1。
    std::int32_t default_iv_size = -1;
    /// 轨道格式里出现过 `crypto-key` 键。
    ///
    /// **不能直接读它的值**（`AMediaFormat_getBuffer` 是 API 28，minSdk = 26），
    /// 这里只记"这个键在不在" —— 由 `AMediaFormat_getString` 之外的方式拿不到，
    /// 所以实现里用 `crypto-mode` 的存在性作主判据，本字段恒由 `crypto-mode` 反推。
    bool key_declared = false;
};

/// 加密判定。`encrypted == true` 表示**必须拒绝**，不是"可能有问题"。
struct EncryptionAssessment {
    bool encrypted = false;
    /// 证据字段用的简短结论：`"clear"` / `"encrypted"` / `"unknown"`。
    std::string verdict;
    /// `encrypted == true` 或 `verdict == "unknown"` 时非空。
    std::string reason;
};

/// 按平台声明的 `crypto-mode` 判定这条轨道能不能播。
///
/// 判据来源见 `docs/design/backend-core/51-p12f3b-design.md` §二：
/// K40 上 `AMediaExtractor_getSampleFlags()` 的 `ENCRYPTED(2)` 位**恒不设**，
/// 真正可靠的信号是轨道格式里的 `crypto-mode`。
///
/// 三态而不是两态：`crypto-mode` 缺席时判 `"unknown"` 而不是 `"clear"` ——
/// 把"平台没说"当"没加密"正是 F3 第一版踩的坑。
[[nodiscard]] EncryptionAssessment assess_encryption(const TrackEncryption& facts);

}  // namespace kivo::video::mediacodec
