// pcm_format_converter_tests.cpp — P12-C3 tests: convert_to_float (P1-P9)
//
// 这一组测试守的是"音频数据进设备之前最后一次形状改变"。
// 转换错了不会崩、不会报错，只会听起来不对——是最难归因的一类缺陷：
//   定点→浮点比例错 → 音量不对或削波；
//   交错顺序错 → 左右声道互换；
//   半帧被放过 → 之后所有样本错位一个声道，整段变噪声。
// 因此这里逐条断言，不靠"能播就行"。
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "video/video_plane/core/audio/pcm_format_converter.hpp"

namespace pcm = kivo::video::core::audio;

namespace {

int g_failed = 0;
int g_checked = 0;

void expect(bool condition, const char* what) {
    ++g_checked;
    if (!condition) {
        ++g_failed;
        std::printf("  [FAIL] %s\n", what);
    }
}

void expect_eq(std::int64_t actual, std::int64_t expected, const char* what) {
    ++g_checked;
    if (actual != expected) {
        ++g_failed;
        std::printf("  [FAIL] %s: 实际 %lld，期望 %lld\n", what,
                    static_cast<long long>(actual), static_cast<long long>(expected));
    }
}

void expect_near(float actual, float expected, const char* what) {
    ++g_checked;
    if (std::fabs(actual - expected) > 1e-6F) {
        ++g_failed;
        std::printf("  [FAIL] %s: 实际 %.8f，期望 %.8f\n", what, static_cast<double>(actual),
                    static_cast<double>(expected));
    }
}

/// 把 int16 序列打包成字节流（模拟解码器给出的缓冲）。
std::vector<std::uint8_t> pack_s16(const std::vector<std::int16_t>& samples) {
    std::vector<std::uint8_t> bytes(samples.size() * sizeof(std::int16_t));
    std::memcpy(bytes.data(), samples.data(), bytes.size());
    return bytes;
}

std::vector<std::uint8_t> pack_s32(const std::vector<std::int32_t>& samples) {
    std::vector<std::uint8_t> bytes(samples.size() * sizeof(std::int32_t));
    std::memcpy(bytes.data(), samples.data(), bytes.size());
    return bytes;
}

pcm::PcmFormat format_of(std::int32_t channels, pcm::PcmEncoding encoding) {
    pcm::PcmFormat format;
    format.sample_rate = 48000;
    format.channel_count = channels;
    format.encoding = encoding;
    return format;
}

/// P1：16bit 定点 → float，比例是 1/32768 而不是 1/32767。
///
/// 为什么必须断言这一条：用 1/32767 时 +32767 会变成 1.0，
/// 与 -1.0 不对称，混音时正半周先削波。这个差异肉眼不可见，只有断言能守住。
void p1_s16_scale() {
    const std::vector<std::int16_t> samples = {0, 32767, -32768, 16384, -16384};
    const auto bytes = pack_s16(samples);
    const auto format = format_of(1, pcm::PcmEncoding::kInt16);

    std::vector<float> out(samples.size(), 0.0F);
    const auto result = pcm::convert_to_float(format, bytes.data(), bytes.size(), out.data(),
                                              static_cast<std::int32_t>(out.size()));

    expect(result.ok(), "P1 转换应成功");
    expect_eq(result.frames_written, 5, "P1 应转换 5 帧");
    expect_near(out[0], 0.0F, "P1 零点");
    expect_near(out[1], 32767.0F / 32768.0F, "P1 正满幅不得夹到 1.0");
    expect_near(out[2], -1.0F, "P1 负满幅");
    expect_near(out[3], 0.5F, "P1 半幅");
    expect_near(out[4], -0.5F, "P1 负半幅");
}

/// P2：32bit 定点 → float，比例 1/2^31。
void p2_s32_scale() {
    const std::vector<std::int32_t> samples = {0, 2147483647, -2147483647 - 1, 1073741824};
    const auto bytes = pack_s32(samples);
    const auto format = format_of(1, pcm::PcmEncoding::kInt32);

    std::vector<float> out(samples.size(), 0.0F);
    const auto result = pcm::convert_to_float(format, bytes.data(), bytes.size(), out.data(),
                                              static_cast<std::int32_t>(out.size()));

    expect(result.ok(), "P2 转换应成功");
    expect_near(out[0], 0.0F, "P2 零点");
    expect_near(out[1], 2147483647.0F / 2147483648.0F, "P2 正满幅");
    expect_near(out[2], -1.0F, "P2 负满幅");
    expect_near(out[3], 0.5F, "P2 半幅");
}

/// P3：源本身就是 float32 时逐位原样拷贝，不做任何乘法。
void p3_float_passthrough() {
    const std::vector<float> samples = {0.0F, 0.25F, -0.75F, 1.0F, -1.0F};
    std::vector<std::uint8_t> bytes(samples.size() * sizeof(float));
    std::memcpy(bytes.data(), samples.data(), bytes.size());
    const auto format = format_of(1, pcm::PcmEncoding::kFloat32);

    std::vector<float> out(samples.size(), 0.0F);
    const auto result = pcm::convert_to_float(format, bytes.data(), bytes.size(), out.data(),
                                              static_cast<std::int32_t>(out.size()));

    expect(result.ok(), "P3 转换应成功");
    for (std::size_t i = 0; i < samples.size(); ++i) {
        expect(out[i] == samples[i], "P3 逐位相等（不得引入任何运算误差）");
    }
}

/// P4：半帧必须被拒。
/// 放过半帧的后果不是"最后少一个样本"，而是**之后所有样本错位一个声道**。
void p4_partial_frame_rejected() {
    const std::vector<std::uint8_t> bytes = {0x00, 0x01, 0x02};  // 3 字节，立体声 16bit 帧长 4
    const auto format = format_of(2, pcm::PcmEncoding::kInt16);

    std::vector<float> out(8, 0.0F);
    const auto result = pcm::convert_to_float(format, bytes.data(), bytes.size(), out.data(), 8);

    expect(!result.ok(), "P4 半帧必须失败");
    expect(result.code == pcm::PcmConversionOutcome::Code::kPartialFrame, "P4 失败原因应为半帧");
    expect_eq(result.frames_written, 0, "P4 失败时不得写出任何帧");
}

/// P5：目标容量不足必须整块失败，不允许"能转多少转多少"。
void p5_output_too_small() {
    const std::vector<std::int16_t> samples = {1, 2, 3, 4, 5, 6};
    const auto bytes = pack_s16(samples);
    const auto format = format_of(1, pcm::PcmEncoding::kInt16);

    std::vector<float> out(4, 0.0F);
    const auto result = pcm::convert_to_float(format, bytes.data(), bytes.size(), out.data(), 4);

    expect(!result.ok(), "P5 容量不足必须失败");
    expect(result.code == pcm::PcmConversionOutcome::Code::kOutputTooSmall, "P5 失败原因应为容量不足");
    expect_near(out[0], 0.0F, "P5 失败时不得写入目标缓冲");
}

/// P6：格式不完整（缺采样率/声道/编码）必须被拒，不许"按默认值猜"。
void p6_invalid_format_rejected() {
    const std::vector<std::uint8_t> bytes = {0x00, 0x01, 0x02, 0x03};

    pcm::PcmFormat unknown_encoding;
    unknown_encoding.sample_rate = 48000;
    unknown_encoding.channel_count = 2;
    unknown_encoding.encoding = pcm::PcmEncoding::kUnknown;

    std::vector<float> out(8, 0.0F);
    const auto with_unknown = pcm::convert_to_float(unknown_encoding, bytes.data(), bytes.size(),
                                                    out.data(), 8);
    expect(with_unknown.code == pcm::PcmConversionOutcome::Code::kInvalidFormat,
           "P6 未知编码应报格式无效");

    pcm::PcmFormat zero_channels = unknown_encoding;
    zero_channels.encoding = pcm::PcmEncoding::kInt16;
    zero_channels.channel_count = 0;
    const auto with_zero = pcm::convert_to_float(zero_channels, bytes.data(), bytes.size(),
                                                 out.data(), 8);
    expect(with_zero.code == pcm::PcmConversionOutcome::Code::kInvalidFormat,
           "P6 声道数为 0 应报格式无效");
}

/// P7：立体声交错顺序必须原样保留（左不进右）。
void p7_stereo_interleaving() {
    // 左 0.5 / 右 -0.5 交替，共 3 帧
    const std::vector<std::int16_t> samples = {16384, -16384, 16384, -16384, 16384, -16384};
    const auto bytes = pack_s16(samples);
    const auto format = format_of(2, pcm::PcmEncoding::kInt16);

    std::vector<float> out(samples.size(), 0.0F);
    const auto result = pcm::convert_to_float(format, bytes.data(), bytes.size(), out.data(),
                                              static_cast<std::int32_t>(out.size()));

    expect(result.ok(), "P7 转换应成功");
    expect_eq(result.frames_written, 3, "P7 立体声 6 样本应为 3 帧");
    for (int frame = 0; frame < 3; ++frame) {
        expect_near(out[static_cast<std::size_t>(frame) * 2], 0.5F, "P7 左声道位置");
        expect_near(out[static_cast<std::size_t>(frame) * 2 + 1], -0.5F, "P7 右声道位置");
    }
}

/// P8：源地址非对齐也要正确。
/// 解码器给出的缓冲不保证按样本宽度对齐；直接 reinterpret_cast 取 int32/float
/// 在 ARM 上可能直接异常，这里用偏移 1 字节的数据把这条锁死。
void p8_unaligned_source() {
    const std::vector<std::int16_t> samples = {16384, -16384, 8192, -8192};
    const auto packed = pack_s16(samples);

    std::vector<std::uint8_t> shifted(packed.size() + 1, 0);
    std::memcpy(shifted.data() + 1, packed.data(), packed.size());

    const auto format = format_of(2, pcm::PcmEncoding::kInt16);
    std::vector<float> out(samples.size(), 0.0F);
    const auto result = pcm::convert_to_float(format, shifted.data() + 1, packed.size(), out.data(),
                                              static_cast<std::int32_t>(out.size()));

    expect(result.ok(), "P8 非对齐源应成功");
    expect_near(out[0], 0.5F, "P8 非对齐左声道");
    expect_near(out[1], -0.5F, "P8 非对齐右声道");
}

/// P9：PcmFormat 的帧长计算 + 零字节输入。
void p9_format_math_and_empty() {
    pcm::PcmFormat stereo_s16;
    stereo_s16.sample_rate = 48000;
    stereo_s16.channel_count = 2;
    stereo_s16.encoding = pcm::PcmEncoding::kInt16;

    expect(stereo_s16.is_valid(), "P9 立体声 16bit 应有效");
    expect_eq(stereo_s16.bytes_per_sample(), 2, "P9 16bit 每样本 2 字节");
    expect_eq(stereo_s16.frame_size_bytes(), 4, "P9 立体声 16bit 帧长 4 字节");

    pcm::PcmFormat broken;
    expect(!broken.is_valid(), "P9 默认构造应无效");
    expect_eq(broken.frame_size_bytes(), 0, "P9 无效格式帧长为 0");

    std::vector<float> out(4, 0.0F);
    const auto empty = pcm::convert_to_float(stereo_s16, nullptr, 0, out.data(), 4);
    expect(empty.ok(), "P9 零字节输入应成功（不是错误）");
    expect_eq(empty.frames_written, 0, "P9 零字节应写 0 帧");
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"P1 16bit 定点比例与满幅对称", p1_s16_scale},
    {"P2 32bit 定点比例", p2_s32_scale},
    {"P3 float32 原样透传", p3_float_passthrough},
    {"P4 半帧被拒", p4_partial_frame_rejected},
    {"P5 目标容量不足被拒", p5_output_too_small},
    {"P6 无效格式被拒", p6_invalid_format_rejected},
    {"P7 立体声交错保持", p7_stereo_interleaving},
    {"P8 非对齐源地址", p8_unaligned_source},
    {"P9 帧长数学与零字节输入", p9_format_math_and_empty},
};

}  // namespace

int main() {
    std::printf("=== P12-C3 PcmFormatConverter tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
