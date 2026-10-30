// episode_tests.cpp — P13-P tests: 最差那一下是「手慢了」还是「表快了」
//
// 这一组守的是六句话：
//   ① **没有最差 = 没有邻帧**，不是"邻帧为空数组、due_step 为 0" ——
//      后者会被读成"目标时刻没动、前后都很正常"，那是一份假观测。
//   ② **前两帧的时间顺序是从旧到新。** 反过来的话，恒等式里 `lateness[k−1]`
//      会取到更早那一帧，差额凭空多出一帧的量。
//   ③ **新最差必须丢掉旧最差的尾巴。** 两个尖峰的前后邻帧拼在一起，
//      会让"收敛用了 8 帧"这种结论凭空成立。
//   ④ **并列取时间上最早的那个**（严格大于才替换），与直方图 `max_ns()` 同口径。
//   ⑤ **`due_step` 记的是最差那一帧的**，不是会话里最后一次的。
//   ⑥ **`frame_worst` 与 `frame_episode` 是同一帧**，且 `of == lateness.count()`；
//      `due_step_supplied` 必须能区分"没位移"与"没接线"。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_stats.hpp"
#include "video/video_plane/core/pace/pace_worst_episode.hpp"

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

constexpr std::int64_t kMs = 1'000'000;
constexpr std::int64_t kFrame = 16'666'667;  // 60 fps

// ---- ① 空会话：没有最差，也没有邻帧 ----------------------------------------
void t01_empty_is_invalid() {
    const pace::WorstEpisodeRecorder rec;
    const pace::WorstEpisode& ep = rec.episode();

    CHECK(ep.valid == false);
    CHECK(ep.before_count == 0);
    CHECK(ep.after_count == 0);
    CHECK(ep.of == 0);
    // `position()` 也必须说"没有"，不能给出一个全零的假位置。
    CHECK(ep.position().valid == false);
}

// ---- ② 首帧即最差：前面没有，后面满 8 --------------------------------------
void t02_first_frame_worst() {
    pace::WorstEpisodeRecorder rec;
    // 首帧 9 ms，之后一路 1 ms。
    rec.note({.lateness_ns = 9 * kMs});
    for (std::int32_t i = 1; i <= 10; ++i) {
        rec.note({.lateness_ns = 1 * kMs, .media_ns = i * kFrame});
    }

    const pace::WorstEpisode& ep = rec.episode();
    CHECK(ep.valid);
    CHECK(ep.index == 0);
    CHECK(ep.lateness_ns == 9 * kMs);
    CHECK(ep.before_count == 0);  // ① 前面确实没有
    CHECK(ep.after_count == pace::WorstEpisode::kAfter);  // 只留 8 个，多的丢掉
    for (std::int32_t i = 0; i < ep.after_count; ++i) {
        CHECK(ep.after_ns[i] == 1 * kMs);
    }
}

// ---- ③ 中段新最差：前两帧按时间正序，尾巴从下一帧开始 ----------------------
void t03_midstream_worst() {
    pace::WorstEpisodeRecorder rec;
    const std::int64_t lateness[] = {1, 2, 3, 50, 4, 5, 6, 7, 8, 9};
    for (std::int32_t i = 0; i < 10; ++i) {
        rec.note({.lateness_ns = lateness[i] * kMs, .media_ns = i * kFrame,
                  .due_step_ns = i == 3 ? -45 * kMs : 0});
    }

    const pace::WorstEpisode& ep = rec.episode();
    CHECK(ep.valid);
    CHECK(ep.index == 3);
    CHECK(ep.lateness_ns == 50 * kMs);
    // ② 前两帧 = 第 1、2 帧（时间正序），不是倒序
    CHECK(ep.before_count == 2);
    CHECK(ep.before_ns[0] == 2 * kMs);
    CHECK(ep.before_ns[1] == 3 * kMs);
    // ⑤ due_step 是最差那一帧的
    CHECK(ep.due_step_ns == -45 * kMs);
    CHECK(ep.after_count == 6);  // 4..9
    CHECK(ep.after_ns[0] == 4 * kMs);
    CHECK(ep.after_ns[5] == 9 * kMs);
}

// ---- ④ 新最差替换旧最差：旧尾巴必须丢掉 -----------------------------------
void t04_new_worst_discards_tail() {
    pace::WorstEpisodeRecorder rec;
    // 第 1 帧 20 ms 是当时的最差，后面 3 帧是它的尾巴；
    // 第 5 帧 40 ms 才是真最差。
    const std::int64_t lateness[] = {1, 20, 2, 3, 40, 5};
    for (std::int32_t i = 0; i < 6; ++i) {
        rec.note({.lateness_ns = lateness[i] * kMs, .media_ns = i * kFrame});
    }

    const pace::WorstEpisode& ep = rec.episode();
    CHECK(ep.index == 4);
    CHECK(ep.lateness_ns == 40 * kMs);
    // ③ 只剩第 5 帧这一条尾巴；20 ms 那一段属于另一个尖峰
    CHECK(ep.after_count == 1);
    CHECK(ep.after_ns[0] == 5 * kMs);
    CHECK(ep.before_ns[0] == 2 * kMs);
    CHECK(ep.before_ns[1] == 3 * kMs);
}

// ---- ⑤ 并列：保留时间上最早的那个 ------------------------------------------
void t05_tie_keeps_earliest() {
    pace::WorstEpisodeRecorder rec;
    rec.note({.lateness_ns = 1 * kMs});
    rec.note({.lateness_ns = 9 * kMs, .media_ns = kFrame});
    rec.note({.lateness_ns = 9 * kMs, .media_ns = 2 * kFrame});

    const pace::WorstEpisode& ep = rec.episode();
    CHECK(ep.index == 1);
    CHECK(ep.lateness_ns == 9 * kMs);
    // 并列的那一帧成为尾巴的第一项 —— 它没有被当成"新的最差"重置
    CHECK(ep.after_count == 1);
    CHECK(ep.after_ns[0] == 9 * kMs);
}

// ---- ⑥ 会话结束：尾巴可以不满，不补零 -------------------------------------
void t06_tail_truncated_at_end() {
    pace::WorstEpisodeRecorder rec;
    rec.note({.lateness_ns = 1 * kMs});
    rec.note({.lateness_ns = 20 * kMs, .media_ns = kFrame});
    rec.note({.lateness_ns = 3 * kMs, .media_ns = 2 * kFrame});

    const pace::WorstEpisode& ep = rec.episode();
    CHECK(ep.after_count == 1);
    CHECK(ep.after_ns[1] == 0);  // 没采到的位置保持 0，但 count 说了它不算数
}

// ---- ⑦ 端到端：PaceStats 的两个口径 ---------------------------------------
void t07_pace_stats_wiring() {
    pace::PaceStats stats;
    const std::int64_t lateness[] = {1, 2, 3, 50, 4, 5, 6, 7, 8, 9};
    for (std::int32_t i = 0; i < 10; ++i) {
        stats.note_presented(lateness[i] * kMs, i * kFrame,
                             pace::DueStep{i == 3 ? -45 * kMs : 0, kFrame});
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);

    CHECK(s.frame_episode.valid);
    CHECK(s.frame_episode.index == 3);
    CHECK(s.frame_episode.due_step_ns == -45 * kMs);
    // ⑥ 两个口径必须是同一帧
    CHECK(s.frame_worst.valid);
    CHECK(s.frame_worst.index == s.frame_episode.index);
    CHECK(s.frame_worst.lateness_ns == s.frame_episode.lateness_ns);
    CHECK(s.frame_worst.media_ns == s.frame_episode.media_ns);
    CHECK(s.frame_worst.of == s.lateness.count());
    CHECK(s.frame_worst.of == 10);
    // ⑥ 接线自证：10 帧都带 due_step，其中 1 帧超一帧
    CHECK(s.due_step_supplied == 10);
    CHECK(s.due_step_frames == 1);
}

// ---- ⑧ 防线：不带 DueStep 时 committed 0，supplied 也必须 0 ----------------
//
// 这一条是给"忘了接线"准备的：如果 `note_presented(l, m)` 也把 supplied 加上去，
// `due_step_frames == 0` 就分不出"没有位移"和"没接参数"，校验器 P5 就失效了。
void t08_supplied_distinguishes_unwired() {
    pace::PaceStats stats;
    for (std::int32_t i = 0; i < 5; ++i) {
        stats.note_presented(1 * kMs, i * kFrame);  // 不给 DueStep
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);

    CHECK(s.presented == 5);
    CHECK(s.due_step_supplied == 0);  // 没接线
    CHECK(s.due_step_frames == 0);    // 这个 0 不可读作"没位移"
    // 极值仍然照常工作：这是 P13-F 的老行为，不能被这张卡改坏
    CHECK(s.frame_worst.valid);
    CHECK(s.frame_worst.index == 0);
    CHECK(s.frame_episode.of == 5);
}

// ---- ⑨ 跨帧账闭合（P13-Q）-------------------------------------------------
//
// 两侧各自独立测量：`present_step` 来自交出时刻的时间戳，`due_step` 来自钟。
// 所以"从首帧累积到现在"必须满足 `last − first == Σpresent − Σdue`；
// 这一条在**每步都很小**时仍然成立 —— 正是单帧字段抓不到的那种位移。
void t09_cumulative_closure() {
    pace::PaceStats stats;
    struct Row {
        std::int64_t lateness;
        std::int64_t due_step;
        std::int64_t present_step;
    };
    // 第 1 帧把 due 推早 20 ms、同时手慢了 10 ms；第 2 帧 due 回 10 ms、手快 10 ms。
    const Row rows[] = {{0, 0, 0}, {30 * kMs, -20 * kMs, 10 * kMs}, {10 * kMs, 10 * kMs, -10 * kMs}};
    for (std::int32_t i = 0; i < 3; ++i) {
        stats.note_presented(rows[i].lateness, i * kFrame,
                             pace::DueStep{rows[i].due_step, kFrame, rows[i].present_step});
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);

    CHECK(s.first_lateness_ns == 0);
    CHECK(s.last_lateness_ns == 10 * kMs);
    CHECK(s.sum_present_step_ns == 0);          // +10 −10
    CHECK(s.sum_due_step_ns == -10 * kMs);      // −20 +10
    CHECK(s.closure_gap_ns == 0);               // 10 − 0 == 0 − (−10)
    // 最差帧（第 1 帧）的累积账：到它为止 present 累 +10、due 累 −20 ⇒ 迟到 30
    CHECK(s.frame_episode.index == 1);
    CHECK(s.frame_episode.cumulative_present_step_ns == 10 * kMs);
    CHECK(s.frame_episode.cumulative_due_step_ns == -20 * kMs);
    CHECK(s.frame_episode.lateness_at_first_ns == 0);
}

// ---- ⑩ 账不平就得出版残差，不能悄悄算成 0 ---------------------------------
//
// 中间有一帧没被记账（丢帧 / 漏传 `present_step`）时，那条恒等式不成立。
// 这时 `closure_gap_ns` 必须非零 —— 否则一份"账不平"的证据看起来和"账平"一样。
void t10_closure_gap_is_published() {
    pace::PaceStats stats;
    stats.note_presented(0, 0, pace::DueStep{0, kFrame, 0});
    // 第二帧：due 推早 20 ms，但 present_step 漏传（0）⇒ Δlateness 只能由 due 解释
    stats.note_presented(20 * kMs, kFrame, pace::DueStep{-20 * kMs, kFrame, 0});
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);

    CHECK(s.closure_gap_ns == 0);  // 这一帧其实还是平的：Δ=20，present−due = 0−(−20)=20

    // 真漏一帧：lateness 跳 20 ms，但两侧都没解释它
    pace::PaceStats bad;
    bad.note_presented(0, 0, pace::DueStep{0, kFrame, 0});
    bad.note_presented(20 * kMs, kFrame, pace::DueStep{0, kFrame, 0});
    const pace::PaceSummary b = bad.summary(kFrame, pace::ClockHealth::kFresh);
    CHECK(b.closure_gap_ns == 20 * kMs);
}

// ---- ⑪ 目标时间轴为什么会动（P13-R）--------------------------------------
//
// 正常分支 `due = mono + (pts − anchor_pts − elapsed)`，于是四个跨度满足
// `due_span == arrive_mono_span + pts_span − elapsed_span`。
// 这一条把"钟比墙快"与"帧到得早"分开：前者让 `elapsed_span > mono_span`。
void t11_span_identity_and_clock_ahead() {
    const std::int64_t t = kFrame;
    // ① 钟与墙同步：due 跟着 pts 走，四个跨度全相等。
    {
        pace::PaceStats stats;
        for (std::int32_t i = 0; i < 4; ++i) {
            const std::int64_t media = i * t;
            const std::int64_t mono = i * t;
            const std::int64_t elapsed = i * t;
            stats.note_presented(0, media,
                                 pace::DueStep{.frame_duration_ns = t,
                                               .clock_mono_ns = mono,
                                               .clock_elapsed_ns = elapsed,
                                               .anchor_pts_ns = 0,
                                               .due_at_ns = mono + media - elapsed});
        }
        const pace::PaceSummary s = stats.summary(t, pace::ClockHealth::kFresh);
        CHECK(s.arrive_mono_span_ns == 3 * t);
        CHECK(s.pts_span_ns == 3 * t);
        CHECK(s.elapsed_span_ns == 3 * t);
        CHECK(s.due_span_ns == 3 * t);
        CHECK(s.due_span_ns == s.arrive_mono_span_ns + s.pts_span_ns - s.elapsed_span_ns);
    }
    // ② 钟比墙快 10 ms/帧：elapsed 多跑 30 ms ⇒ due 整体前移 30 ms。
    //    这条就是 P13-Q 里 `cumD` 的来源，只是这里被放大到看得见的量级。
    {
        pace::PaceStats stats;
        for (std::int32_t i = 0; i < 4; ++i) {
            const std::int64_t media = i * t;
            const std::int64_t mono = i * t;
            const std::int64_t elapsed = i * t + i * 10 * kMs;
            stats.note_presented(0, media,
                                 pace::DueStep{.frame_duration_ns = t,
                                               .clock_mono_ns = mono,
                                               .clock_elapsed_ns = elapsed,
                                               .anchor_pts_ns = 0,
                                               .due_at_ns = mono + media - elapsed});
        }
        const pace::PaceSummary s = stats.summary(t, pace::ClockHealth::kFresh);
        CHECK(s.due_span_ns == 3 * t - 30 * kMs);
        CHECK(s.elapsed_span_ns - s.arrive_mono_span_ns == 30 * kMs);  // 钟走快了 30 ms
        CHECK(s.due_span_ns == s.arrive_mono_span_ns + s.pts_span_ns - s.elapsed_span_ns);
    }
}

}  // namespace

int main() {
    t01_empty_is_invalid();
    t02_first_frame_worst();
    t03_midstream_worst();
    t04_new_worst_discards_tail();
    t05_tie_keeps_earliest();
    t06_tail_truncated_at_end();
    t07_pace_stats_wiring();
    t08_supplied_distinguishes_unwired();
    t09_cumulative_closure();
    t10_closure_gap_is_published();
    t11_span_identity_and_clock_ahead();

    std::printf("P13-P episode: %d checked, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
