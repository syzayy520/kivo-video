#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"

#include "video/video_plane/backends/mediacodec/source/encryption_key_names.hpp"

namespace kivo::video::mediacodec {

const char* crypto_key::crypto_mode_name(std::int32_t mode) noexcept {
    switch (mode) {
        case crypto_key::kCryptoModeClear:
            return "clear";
        case crypto_key::kCryptoModeAesCtr:
            return "aes-ctr";
        case crypto_key::kCryptoModeAesCbc:
            return "aes-cbc";
        default:
            return "unknown";
    }
}

EncryptionAssessment assess_encryption(const TrackEncryption& facts) {
    EncryptionAssessment out;

    if (!facts.mode_present) {
        // 平台没声明 crypto-mode。**不能当成 clear**。
        //
        // 这一步刻意不返回 pass：明文片源在 K40 上同样没有这个键（实测
        // `probe_seek_av_1080p30.mp4` 的 dump 里就没有 crypto-mode），
        // 所以"缺席"其实等价于明文。但把它写成 `"clear"` 会让一个
        // 未来平台改键名的场景静默变成"可播"。
        //
        // 折中：判定为 `"unknown"` 并带 reason，**但 `encrypted` 保持 false** ——
        // 上层据此继续播放（否则所有明文片源都会被拒），同时证据里留下痕迹。
        out.verdict = "unknown";
        out.reason =
            "track format carries no crypto-mode key, so the platform did not declare whether this "
            "track is encrypted; playback continues, but this run cannot prove the track was clear";
        return out;
    }

    if (facts.crypto_mode == crypto_key::kCryptoModeClear) {
        out.verdict = "clear";
        return out;
    }

    out.encrypted = true;
    out.verdict = "encrypted";
    out.reason = "track declares crypto-mode=" + std::to_string(facts.crypto_mode) + " (" +
                 crypto_key::crypto_mode_name(facts.crypto_mode) +
                 "); sample-level encrypted tracks are not supported — no key/DRM session is set up";
    if (facts.default_iv_size > 0) {
        out.reason += ", default IV size " + std::to_string(facts.default_iv_size) + " bytes";
    }
    return out;
}

}  // namespace kivo::video::mediacodec
