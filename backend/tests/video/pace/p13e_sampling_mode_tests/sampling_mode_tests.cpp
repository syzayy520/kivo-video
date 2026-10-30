// sampling_mode_tests.cpp — P13-E tests: 分位数的采样口径词汇
//
// 这一组守的是三句话：
//   ① **三个线格式名字逐字不变** —— 它们写在 JSON 里、被 Python 校验器读、
//      被状态卡的表格引用。改一个字节就是换了一份 schema，必须是一次显式决定，
//      不能是某次"顺手改得更好读"。
//   ② **只有抽稀口径可以跨帧率比较** —— 一个 `comparable_across_frame_rates`
//      写反，30fps 与 60fps 的 p99 就会重新被摆进同一张表，而这张表是 P13-D
//      花了一整卡才拆干净的东西。
//   ③ **两个口径不许同名** —— 同名意味着"哪一份都行"，那正是本卡要消灭的歧义。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "video/video_plane/core/pace/sampling_mode.hpp"

namespace pace = kivo::video::core::pace;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                        \
    do {                                                                   \
        ++g_checked;                                                       \
        if (!(expr)) {                                                     \
            ++g_failed;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);  \
        }                                                                  \
    } while (false)

// ---- E1/E2 线格式名字逐字钉死 ----------------------------------------------
void t01_names_are_wire_format() {
    const auto decimated = pace::sampling_mode_name(pace::SamplingMode::kMediaTimeDecimated);
    const auto per_frame = pace::sampling_mode_name(pace::SamplingMode::kPerFrame);
    const auto measured = pace::sampling_mode_name(pace::SamplingMode::kNotMeasured);

    // 逐字比对，不用前缀/包含判断 —— 那种断言允许"加个后缀"悄悄溜过去。
    CHECK(std::strcmp(decimated, "media_time_decimated") == 0);
    CHECK(std::strcmp(per_frame, "per_frame") == 0);
    CHECK(std::strcmp(measured, "not_measured") == 0);
}

// ---- E3 三个名字互不相同、均非空 --------------------------------------------
//
// 这是最省事也最容易被"优化"掉的一条：把 per_frame 和 media_time_decimated
// 合并成一个 "lateness"，代码会短一截，然后 30/60 的 p99 又混回一张表。
void t02_names_are_distinct_and_non_empty() {
    const char* names[] = {
        pace::sampling_mode_name(pace::SamplingMode::kMediaTimeDecimated),
        pace::sampling_mode_name(pace::SamplingMode::kPerFrame),
        pace::sampling_mode_name(pace::SamplingMode::kNotMeasured),
    };
    for (const char* name : names) {
        CHECK(name != nullptr);
        CHECK(name != nullptr && name[0] != '\0');
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            CHECK(std::strcmp(names[i], names[j]) != 0);
        }
    }
}

// ---- E4 只有抽稀口径可跨帧率比较 --------------------------------------------
void t03_only_decimated_is_comparable() {
    CHECK(pace::comparable_across_frame_rates(pace::SamplingMode::kMediaTimeDecimated));
    CHECK(!pace::comparable_across_frame_rates(pace::SamplingMode::kPerFrame));
    // 量不出来的一轮当然更不能进对比表 —— 如果这条为真，
    // "没测"就会被当成一个可比的数参与中位数。
    CHECK(!pace::comparable_across_frame_rates(pace::SamplingMode::kNotMeasured));
}

// ---- E5 未登记的枚举值不许伪装成合法口径 ------------------------------------
//
// 枚举被加了新值而名字表没更新时，返回值必须一眼看得出不对。
// 返回 "unknown" 这种"看起来像个正常口径"的字符串，会让一份坏证据长得很健康。
void t04_unregistered_mode_is_obviously_wrong() {
    const auto bogus = static_cast<pace::SamplingMode>(200);
    const char* name = pace::sampling_mode_name(bogus);
    CHECK(name != nullptr);
    CHECK(std::strstr(name, "unregistered") != nullptr);
}

}  // namespace

int main() {
    struct Case {
        const char* name;
        void (*fn)();
    };
    const Case cases[] = {
        {"01 三个线格式名字逐字不变", t01_names_are_wire_format},
        {"02 三个名字互不相同且非空", t02_names_are_distinct_and_non_empty},
        {"03 只有抽稀口径可跨帧率比较", t03_only_decimated_is_comparable},
        {"04 未登记模式明显非法", t04_unregistered_mode_is_obviously_wrong},
    };

    for (const Case& item : cases) {
        const int before = g_failed;
        item.fn();
        std::printf("%s %s\n", (g_failed == before) ? "PASS" : "FAIL", item.name);
    }
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
