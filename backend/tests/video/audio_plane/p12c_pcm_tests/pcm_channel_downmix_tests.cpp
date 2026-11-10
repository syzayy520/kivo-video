// pcm_channel_downmix_tests.cpp — 252：5.1 → 立体声下混矩阵
//
// 这一组测试守的是"人声还在不在"。
// 252 的根因是：真高清音轨的人声几乎全部在 FC（中置声道）——
// 实测 Shelter 的 TrueHD 5.1(side) 轨：FC 占总能量 70.7%，FL/FR 各只有 12%/11%。
// 交给平台做 6→2 时平台把 FC 压到 −3 dB 计入，人声被明显压掉（用户反馈"人声太小"）。
// 我们改成自己按 AC3 标准矩阵折，于是矩阵本身就是判据：
// **FC 必须以 0.7071 的权重同时进入 L 和 R**，否则人声就没了。
//
// 数值错了不会崩、不会报错，只会"听着人声小一点"——是最难归因的一类缺陷，
// 因此这里逐条断言系数与能量，不靠"能播就行"。
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "video/video_plane/core/audio/pcm_channel_downmix.hpp"

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

void expect_near(float actual, float expected, float tolerance, const char* what) {
    ++g_checked;
    if (std::fabs(actual - expected) > tolerance) {
        ++g_failed;
        std::printf("  [FAIL] %s: 实际 %.6f，期望 %.6f\n", what, static_cast<double>(actual),
                    static_cast<double>(expected));
    }
}

/// 造一帧 5.1：FL FR FC LFE BL BR。
std::vector<float> make_frame(float fl, float fr, float fc, float lfe, float bl, float br) {
    return {fl, fr, fc, lfe, bl, br};
}

/// 对一段 5.1 做下混，返回立体声帧。
std::vector<float> downmix(const std::vector<std::vector<float>>& frames) {
    std::vector<float> source;
    source.reserve(frames.size() * pcm::kSourceChannels5Point1);
    for (const auto& frame : frames) {
        source.insert(source.end(), frame.begin(), frame.end());
    }
    std::vector<float> stereo(frames.size() * pcm::kStereoChannels, 0.0F);
    const std::int32_t written = pcm::downmix_5point1_to_stereo(
        source.data(), static_cast<std::int32_t>(frames.size()), stereo.data(),
        static_cast<std::int32_t>(frames.size()));
    expect(written == static_cast<std::int32_t>(frames.size()), "下混写出的帧数与输入一致");
    return stereo;
}

/// 只在 FC 有信号（中置独占）时的左右输出。
void test_center_only_is_kept_in_both_channels() {
    const float k = pcm::kDownmixCenterWeight;
    const auto stereo = downmix({make_frame(0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F)});
    expect_near(stereo[0], k, 1e-6F, "纯中置信号进左声道，权重为 0.7071");
    expect_near(stereo[1], k, 1e-6F, "纯中置信号进右声道，权重为 0.7071（人声必须在两边都在）");

    // 关键性质：FC 单独存在时，左右各得 0.7071 ⇒ 两声道**相等**。
    // 若有人把 FC 只送一边，人声就会偏到一侧 —— 那是"人声小且偏"的典型故障。
    expect_near(stereo[0], stereo[1], 1e-6F, "纯中置信号左右完全相等（人声居中）");
}

void test_front_channels_pass_through() {
    const auto stereo = downmix({make_frame(0.5F, -0.25F, 0.0F, 0.0F, 0.0F, 0.0F)});
    expect_near(stereo[0], 0.5F, 1e-6F, "FL 原样进入左声道");
    expect_near(stereo[1], -0.25F, 1e-6F, "FR 原样进入右声道");
}

void test_surround_weight() {
    const float k = pcm::kDownmixSurroundWeight;
    const auto stereo = downmix({make_frame(0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F)});
    expect_near(stereo[0], k, 1e-6F, "左环绕进左声道，权重为 0.7071");
    expect_near(stereo[1], 0.0F, 1e-6F, "左环绕不进右声道");

    const auto stereo2 = downmix({make_frame(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F)});
    expect_near(stereo2[1], k, 1e-6F, "右环绕进右声道，权重为 0.7071");
    expect_near(stereo2[0], 0.0F, 1e-6F, "右环绕不进左声道");
}

void test_lfe_is_dropped() {
    // LFE 不进立体声：标准做法，低频效果该由独立声道承担。
    // 若把 LFE 折进来，低频会糊在人声上 —— 与"人声太小"是相反方向的错误，
    // 但同样要守住：这里断言它**确实被丢弃**。
    const auto stereo = downmix({make_frame(0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F)});
    expect_near(stereo[0], 0.0F, 1e-6F, "LFE 被丢弃，不进左声道");
    expect_near(stereo[1], 0.0F, 1e-6F, "LFE 被丢弃，不进右声道");
}

/// 全 6 声道满幅时不得越界（下游是浮点，但越界意味着矩阵配错）。
void test_no_overshoot_on_full_scale() {
    const auto stereo = downmix({make_frame(1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F)});
    const float expected = 1.0F + pcm::kDownmixCenterWeight + pcm::kDownmixSurroundWeight;
    expect_near(stereo[0], expected, 1e-6F, "全满幅左声道 = FL + 0.7071·FC + 0.7071·BL");
    expect_near(stereo[1], expected, 1e-6F, "全满幅右声道 = FR + 0.7071·FC + 0.7071·BR");
}

/// 目标容量不足时只写能写下的部分，**不静默丢弃**。
void test_truncates_to_capacity() {
    const std::vector<float> source(12, 0.5F);  // 2 帧
    std::vector<float> stereo(2, 0.0F);         // 只够 1 帧（2 声道）
    const std::int32_t written = pcm::downmix_5point1_to_stereo(source.data(), 2, stereo.data(), 1);
    expect(written == 1, "目标容量不足时只写能写下的帧数");
}

/// 空输入与空指针必须安全返回 0（实时线程上的越界写入会直接崩）。
void test_degenerate_inputs() {
    std::vector<float> stereo(4, 0.0F);
    expect(pcm::downmix_5point1_to_stereo(nullptr, 1, stereo.data(), 2) == 0, "空源指针返回 0");
    expect(pcm::downmix_5point1_to_stereo(stereo.data(), 0, stereo.data(), 2) == 0, "零帧返回 0");
    expect(pcm::downmix_5point1_to_stereo(stereo.data(), 1, nullptr, 2) == 0, "空目标指针返回 0");
    expect(pcm::downmix_5point1_to_stereo(stereo.data(), 1, stereo.data(), 0) == 0, "零容量返回 0");
}

/// 回归：多帧连续处理时帧边界不错位（第 2 帧不能读到第 1 帧的数据）。
void test_frame_boundaries() {
    std::vector<float> source;
    // 帧0：只有 FL=0.1；帧1：只有 FR=0.2
    source.insert(source.end(), {0.1F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F});
    source.insert(source.end(), {0.0F, 0.2F, 0.0F, 0.0F, 0.0F, 0.0F});
    std::vector<float> stereo(4, 0.0F);
    const std::int32_t written = pcm::downmix_5point1_to_stereo(source.data(), 2, stereo.data(), 2);
    expect(written == 2, "两帧都写出了");
    expect_near(stereo[0], 0.1F, 1e-6F, "第 0 帧左声道是 0.1");
    expect_near(stereo[1], 0.0F, 1e-6F, "第 0 帧右声道是 0");
    expect_near(stereo[2], 0.0F, 1e-6F, "第 1 帧左声道是 0（边界没错位）");
    expect_near(stereo[3], 0.2F, 1e-6F, "第 1 帧右声道是 0.2");
}

}  // namespace

int main() {
    std::printf("pcm_channel_downmix_tests\n");

    test_center_only_is_kept_in_both_channels();
    test_front_channels_pass_through();
    test_surround_weight();
    test_lfe_is_dropped();
    test_no_overshoot_on_full_scale();
    test_truncates_to_capacity();
    test_degenerate_inputs();
    test_frame_boundaries();

    std::printf("%d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
