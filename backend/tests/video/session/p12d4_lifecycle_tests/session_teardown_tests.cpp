// session_teardown_tests.cpp — P12-D4 tests: 会话收尾顺序
//
// 这张卡的判据是"释放顺序唯一性有单测守护"。守护的对象不是代码里的先后语句，
// 而是 `session_teardown_plan.hpp` 里那份 constexpr 计划表 —— 因为
// `session_stop()` 与 `session_destroy()` 都只是它的消费者。
//
// 六条用例分两类：
//   t27–t30 盯**计划表本身**（顺序对不对、是不是同一份、停是否全在放之前）；
//   t31–t32 盯**执行器**（它有没有自己的顺序、重复收尾会不会覆盖已取到的数据）。
//
// t31 是这套测试的核心：只测数据不测执行器，就会出现"表好看但代码另写一套"。
#include <cstdint>
#include <cstdio>
#include <vector>

#include "video/video_plane/backends/mediacodec/session/session_teardown_plan.hpp"

namespace session = kivo::video::mediacodec;

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

/// 只记账的钩子：它不碰任何真实资源，只把"被调用过哪些步、各几次"记下来。
///
/// 这个结构体的存在本身就是设计的一部分：它证明执行器只依赖七个函数名，
/// 不依赖 `SessionHandle` 的任何细节 —— 所以顺序可以在桌面上被完整走一遍。
struct RecordingHooks {
    std::vector<session::SessionTeardownStep> calls;
    /// 模拟"取到的音频位置"。`-1` 表示还没取过（与 `SessionHandle` 同语义）。
    std::int64_t audio_position_at_video_stop_ns = -1;
    /// 采样时音频是否还在跑。真实实现里停了流就取不到位置，这里显式建模。
    bool audio_running = true;
    int sample_attempts = 0;

    void stop_video() { calls.push_back(session::SessionTeardownStep::kStopVideo); }
    void stop_audio() {
        calls.push_back(session::SessionTeardownStep::kStopAudio);
        audio_running = false;
    }
    void reset_video() { calls.push_back(session::SessionTeardownStep::kResetVideo); }
    void reset_audio() { calls.push_back(session::SessionTeardownStep::kResetAudio); }
    void reset_window() { calls.push_back(session::SessionTeardownStep::kResetWindow); }
    void reset_clock() { calls.push_back(session::SessionTeardownStep::kResetClock); }

    void sample_audio_position() {
        calls.push_back(session::SessionTeardownStep::kSampleAudioPosition);
        ++sample_attempts;
        // 与 JNI 侧 `SessionTeardownHooks` 完全相同的三条守卫：
        // 没音频就跳过、已经取过就不覆盖、音频停了取不到。
        if (!audio_running) {
            return;
        }
        if (audio_position_at_video_stop_ns >= 0) {
            return;
        }
        audio_position_at_video_stop_ns = 1'234'567;
    }
};

// ---- t27 销毁计划逐项等于预期七步 ----------------------------------------
void t27_destroy_plan_is_exact() {
    const auto plan = session::session_destroy_plan();
    CHECK(plan.size() == 7);
    CHECK(plan[0] == session::SessionTeardownStep::kStopVideo);
    CHECK(plan[1] == session::SessionTeardownStep::kSampleAudioPosition);
    CHECK(plan[2] == session::SessionTeardownStep::kStopAudio);
    CHECK(plan[3] == session::SessionTeardownStep::kResetVideo);
    CHECK(plan[4] == session::SessionTeardownStep::kResetAudio);
    CHECK(plan[5] == session::SessionTeardownStep::kResetWindow);
    CHECK(plan[6] == session::SessionTeardownStep::kResetClock);
}

// ---- t28 停止计划是销毁计划的严格前缀 ------------------------------------
//
// "严格前缀"同时守住三件事：
//   ① 停止不是另一份列表（否则两份可以各自演化，"唯一"就成了口号）；
//   ② `kSessionStopPlanSize` 与数组没脱钩（改了数组忘了常量就会红）；
//   ③ 停止做的事销毁一定也做（不会有"销毁漏了停音频"这种分支）。
void t28_stop_plan_is_strict_prefix() {
    const auto stop_plan = session::session_stop_plan();
    const auto destroy_plan = session::session_destroy_plan();

    CHECK(stop_plan.size() == session::kSessionStopPlanSize);
    CHECK(session::kSessionStopPlanSize == 3);
    CHECK(stop_plan.size() < destroy_plan.size());  // 严格：不是"相等也算前缀"
    CHECK(stop_plan.data() == destroy_plan.data());  // 同一块内存，不是拷贝出来的副本

    for (std::size_t i = 0; i < stop_plan.size(); ++i) {
        CHECK(stop_plan[i] == destroy_plan[i]);
    }
}

// ---- t29 所有 reset 都排在所有 stop 之后 ---------------------------------
//
// 这条是同一条硬约束的静态版本：**先停后放**。
// 反过来的顺序（先放资源、再停线程）会让线程摸到已释放对象，
// 而它恰恰是最容易被"整理代码"整理坏的。
void t29_all_resets_after_all_stops() {
    bool seen_reset = false;
    bool order_ok = true;
    for (const auto step : session::session_destroy_plan()) {
        const bool is_reset = step == session::SessionTeardownStep::kResetVideo ||
                              step == session::SessionTeardownStep::kResetAudio ||
                              step == session::SessionTeardownStep::kResetWindow ||
                              step == session::SessionTeardownStep::kResetClock;
        const bool is_stop = step == session::SessionTeardownStep::kStopVideo ||
                             step == session::SessionTeardownStep::kStopAudio;
        if (is_reset) {
            seen_reset = true;
        }
        if (is_stop && seen_reset) {
            order_ok = false;
        }
    }
    CHECK(order_ok);
}

// ---- t30 采样夹在"停视频"与"停音频"之间 ----------------------------------
//
// C8 那个覆盖 bug 的形状：停完音频再取位置，读到的是停流后的回退档。
// 位置错了不会有任何异常，只会让 `kivo.p12c.sync/1` 的 `timeline` 静默变错 ——
// 所以必须钉死在这个位置上。
void t30_sample_between_video_and_audio_stop() {
    const auto plan = session::session_destroy_plan();
    auto index_of = [&plan](session::SessionTeardownStep target) -> int {
        for (std::size_t i = 0; i < plan.size(); ++i) {
            if (plan[i] == target) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    const int stop_video = index_of(session::SessionTeardownStep::kStopVideo);
    const int sample = index_of(session::SessionTeardownStep::kSampleAudioPosition);
    const int stop_audio = index_of(session::SessionTeardownStep::kStopAudio);

    CHECK(stop_video >= 0);
    CHECK(sample >= 0);
    CHECK(stop_audio >= 0);
    CHECK(sample > stop_video);   // 视频停稳之后才谈"同一时刻"
    CHECK(sample < stop_audio);   // 音频还活着才取得到真实位置
}

// ---- t31 执行器按数据走，没有自己的顺序 ----------------------------------
//
// 这条是本套测试的核心。若执行器内部另写了一套 switch 顺序，
// 前面四条测的全是"表好看"，一次都不会红。
void t31_runner_follows_plan() {
    RecordingHooks stop_hooks;
    session::run_session_teardown(session::session_stop_plan(), stop_hooks);
    CHECK(stop_hooks.calls.size() == session::kSessionStopPlanSize);
    CHECK(stop_hooks.calls[0] == session::SessionTeardownStep::kStopVideo);
    CHECK(stop_hooks.calls[1] == session::SessionTeardownStep::kSampleAudioPosition);
    CHECK(stop_hooks.calls[2] == session::SessionTeardownStep::kStopAudio);

    RecordingHooks destroy_hooks;
    session::run_session_teardown(session::session_destroy_plan(), destroy_hooks);
    CHECK(destroy_hooks.calls.size() == 7);
    // 逐项比对，而不是只看长度：长度对、内容错的 switch 一样能过前者。
    for (std::size_t i = 0; i < destroy_hooks.calls.size(); ++i) {
        CHECK(destroy_hooks.calls[i] == session::session_destroy_plan()[i]);
    }
}

// ---- t32 重复收尾不覆盖已取到的音频位置 ----------------------------------
//
// 真机路径上 `stop()` 与 `destroy()` 会被连着调（`teardownSession()` 先 stop，
// 退出时再 destroy）。第二遍走到采样步时音频已停，`now()` 读到的是回退档。
// 没有这层幂等保护，好数据会被坏数据覆盖 —— 而证据看起来依然完整。
void t32_repeated_teardown_keeps_first_sample() {
    RecordingHooks hooks;

    session::run_session_teardown(session::session_stop_plan(), hooks);
    const std::int64_t first = hooks.audio_position_at_video_stop_ns;
    CHECK(first > 0);

    session::run_session_teardown(session::session_destroy_plan(), hooks);
    CHECK(hooks.audio_position_at_video_stop_ns == first);  // 没被第二遍改掉
    CHECK(hooks.sample_attempts == 2);                      // 但确实又走过那一步
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"D4-1 销毁计划逐项正确", t27_destroy_plan_is_exact},
    {"D4-2 停止是严格前缀且同一块内存", t28_stop_plan_is_strict_prefix},
    {"D4-3 停全部早于放", t29_all_resets_after_all_stops},
    {"D4-4 采样夹在停视频与停音频之间", t30_sample_between_video_and_audio_stop},
    {"D4-5 执行器按计划走", t31_runner_follows_plan},
    {"D4-6 重复收尾不覆盖首采", t32_repeated_teardown_keeps_first_sample},
};

}  // namespace

int main() {
    std::printf("=== P12-D4 session teardown tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
