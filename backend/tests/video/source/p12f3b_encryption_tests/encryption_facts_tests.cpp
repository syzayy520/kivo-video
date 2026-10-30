// encryption_facts_tests.cpp — P12-F3B：加密判定的单测
//
// 这一层刻意不碰 `AMediaFormat`：判定规则必须能在桌面上钉住，真机只负责提供
// `crypto-mode` 的值。测的重点不是"能不能识别加密"，而是**三种情况不许混**：
// 声明加密 / 明确声明不加密 / 平台没说。F3 第一版正是因为把第三种当成了第二种，
// 才把一条加密轨播成了"成功"。
#include <cstdio>
#include <cstring>
#include <string>

#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_key_names.hpp"

namespace {

namespace mediacodec = kivo::video::mediacodec;
namespace crypto_key = kivo::video::mediacodec::crypto_key;

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* expr, const char* test, int line) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL %s:%d  %s\n", test, line, expr);
    }
}

#define CHECK(expr) check((expr), #expr, __func__, __LINE__)

/// 造一份"平台声明了 crypto-mode"的事实。
mediacodec::TrackEncryption with_mode(std::int32_t mode, std::int32_t iv = -1) {
    mediacodec::TrackEncryption facts;
    facts.mode_present = true;
    facts.crypto_mode = mode;
    facts.default_iv_size = iv;
    facts.key_declared = true;
    return facts;
}

bool contains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}

// ---- F3B-1 AES-CTR 必须判加密 ------------------------------------------
void t1_aes_ctr_is_encrypted() {
    const auto out = mediacodec::assess_encryption(with_mode(crypto_key::kCryptoModeAesCtr, 8));
    CHECK(out.encrypted);
    CHECK(out.verdict == "encrypted");
    CHECK(contains(out.reason, "crypto-mode=1"));
    CHECK(contains(out.reason, "aes-ctr"));
    CHECK(contains(out.reason, "8 bytes"));
    CHECK(contains(out.reason, "not supported"));
}

// ---- F3B-2 AES-CBC 同样拒绝 --------------------------------------------
void t2_aes_cbc_is_encrypted() {
    const auto out = mediacodec::assess_encryption(with_mode(crypto_key::kCryptoModeAesCbc));
    CHECK(out.encrypted);
    CHECK(contains(out.reason, "aes-cbc"));
}

// ---- F3B-3 未知模式按加密处理（保守）-----------------------------------
void t3_unknown_mode_is_encrypted() {
    const auto out = mediacodec::assess_encryption(with_mode(99));
    CHECK(out.encrypted);
    CHECK(contains(out.reason, "unknown"));
    // 关键：不能因为"不认识"就放行。放行的代价是播一堆垃圾帧还报成功。
    CHECK(contains(out.reason, "crypto-mode=99"));
}

// ---- F3B-4 明确声明不加密 → clear，且不带 reason ------------------------
void t4_explicit_clear_passes() {
    const auto out = mediacodec::assess_encryption(with_mode(crypto_key::kCryptoModeClear));
    CHECK(!out.encrypted);
    CHECK(out.verdict == "clear");
    CHECK(out.reason.empty());
}

// ---- F3B-5 平台没说 → unknown，可播但必须留痕 --------------------------
void t5_absent_mode_is_unknown_not_clear() {
    mediacodec::TrackEncryption facts;  // 全默认：mode_present == false
    const auto out = mediacodec::assess_encryption(facts);
    CHECK(!out.encrypted);          // 明文片源也没有这个键，拒了会全挂
    CHECK(out.verdict == "unknown");
    CHECK(!out.verdict.empty());
    // 这一条是本卡的核心：不能报 "clear"。
    CHECK(out.verdict != "clear");
    CHECK(contains(out.reason, "no crypto-mode"));
    CHECK(contains(out.reason, "cannot prove"));
}

// ---- F3B-6 三种 verdict 互不相同 --------------------------------------
void t6_three_verdicts_are_distinct() {
    const auto clear = mediacodec::assess_encryption(with_mode(crypto_key::kCryptoModeClear));
    const auto encrypted = mediacodec::assess_encryption(with_mode(crypto_key::kCryptoModeAesCtr));
    const auto unknown = mediacodec::assess_encryption(mediacodec::TrackEncryption{});
    CHECK(clear.verdict != encrypted.verdict);
    CHECK(clear.verdict != unknown.verdict);
    CHECK(encrypted.verdict != unknown.verdict);
}

// ---- F3B-7 键名常量不许手滑 -------------------------------------------
void t7_key_names_match_platform() {
    // 这些字符串直接决定判定是否生效；写错不会有编译错误，只会永远判 clear。
    CHECK(std::strcmp(crypto_key::kKeyCryptoMode, "crypto-mode") == 0);
    CHECK(std::strcmp(crypto_key::kKeyCryptoKey, "crypto-key") == 0);
    CHECK(std::strcmp(crypto_key::kKeyCryptoDefaultIvSize, "crypto-default-iv-size") == 0);
    CHECK(crypto_key::kCryptoModeAesCtr == 1);
    CHECK(crypto_key::kCryptoModeAesCbc == 2);
}

// ---- F3B-8 mode 名不许把未知套成已知 -----------------------------------
void t8_crypto_mode_name() {
    CHECK(std::strcmp(crypto_key::crypto_mode_name(0), "clear") == 0);
    CHECK(std::strcmp(crypto_key::crypto_mode_name(1), "aes-ctr") == 0);
    CHECK(std::strcmp(crypto_key::crypto_mode_name(2), "aes-cbc") == 0);
    CHECK(std::strcmp(crypto_key::crypto_mode_name(7), "unknown") == 0);
    CHECK(std::strcmp(crypto_key::crypto_mode_name(-1), "unknown") == 0);
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"F3B-1 AES-CTR 判加密", t1_aes_ctr_is_encrypted},
    {"F3B-2 AES-CBC 判加密", t2_aes_cbc_is_encrypted},
    {"F3B-3 未知模式按加密", t3_unknown_mode_is_encrypted},
    {"F3B-4 明确 clear 放行", t4_explicit_clear_passes},
    {"F3B-5 缺席判 unknown", t5_absent_mode_is_unknown_not_clear},
    {"F3B-6 三态互不相同", t6_three_verdicts_are_distinct},
    {"F3B-7 键名常量", t7_key_names_match_platform},
    {"F3B-8 mode 名", t8_crypto_mode_name},
};

}  // namespace

int main() {
    std::printf("=== P12-F3B track encryption tests ===\n");
    for (const TestCase& test : kCases) {
        const int before = g_failures;
        test.run();
        std::printf("%s %s\n", g_failures == before ? "ok  " : "FAIL", test.name);
    }
    std::printf("%d cases, %d assertions, %d failures\n",
                static_cast<int>(sizeof(kCases) / sizeof(kCases[0])), g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
