#include "video/video_plane/backends/mediacodec/source/mediaformat_encryption.hpp"

#include "video/video_plane/backends/mediacodec/source/encryption_key_names.hpp"

#include <media/NdkMediaFormat.h>

namespace kivo::video::mediacodec {

TrackEncryption read_track_encryption(const AMediaFormat* format) {
    TrackEncryption out;
    if (format == nullptr) {
        return out;
    }

    AMediaFormat* writable = const_cast<AMediaFormat*>(format);
    std::int32_t mode = 0;
    if (AMediaFormat_getInt32(writable, crypto_key::kKeyCryptoMode, &mode)) {
        out.mode_present = true;
        out.crypto_mode = mode;
    }

    std::int32_t iv_size = 0;
    if (AMediaFormat_getInt32(writable, crypto_key::kKeyCryptoDefaultIvSize, &iv_size)) {
        out.default_iv_size = iv_size;
    }

    // `crypto-key` 只做"在不在"的旁证。注意这里**不是**在读它的值：
    // 读 data 需要 `AMediaFormat_getBuffer`（API 28），minSdk = 26 不能直接调。
    // 实测 `crypto-mode` 与 `crypto-key` 总是同时出现，所以用前者代表后者的存在性，
    // 并在字段名上写清楚这是"声明"，不是"取值"。
    out.key_declared = out.mode_present;

    return out;
}

}  // namespace kivo::video::mediacodec
