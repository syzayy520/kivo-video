#include "jni_session_handle.hpp"

#include "video/video_plane/backends/avsync/report/av_sync_report.hpp"
#include "video/video_plane/backends/avsync/session/audio_playback.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_device_identity.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_wall_clock.hpp"
#include "video/video_plane/backends/mediacodec/present/surface_window_binder.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_session.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stats_renderer.hpp"
#include "video/video_plane/backends/mediacodec/session/session_teardown_plan.hpp"
#include "video/video_plane/core/audio/clock/clock_offset_source.hpp"
#include "video/video_plane/core/audio/clock/system_clock_source.hpp"

#include <android/native_window_jni.h>

#include <cstdio>
#include <fstream>
#include <memory>

namespace kivo::jni_bridge {

using kivo::video::mediacodec::DecodeSession;
using kivo::video::mediacodec::SurfaceWindowBinder;

struct SessionHandle {
    /// 成员顺序**不是随手排的**（P12-D4）：析构是声明逆序，先声明者后亡。
    /// 持有裸指针的一方必须先死：`session` 的节拍器持着 `system_clock` / `audio` 的时钟，
    /// 所以那两个必须声明在 `session` 之前。
    ///
    /// 显式收尾走 `session_teardown_plan.hpp` 的计划表（`session_stop` / `session_destroy`）；
    /// 这里的声明顺序是它的**兜底** —— 万一某条路径没走计划直接 `delete`，
    /// 逆序析构至少还是对的。两处必须互相印证，所以排成员时先看计划表。
    std::unique_ptr<SurfaceWindowBinder> window;

    /// 纯视频时的时钟。**必须声明在 session 之前**。
    std::unique_ptr<kivo::video::core::audio::SystemClockSource> system_clock;

    /// 音频播放侧（P12-C8）。同样必须声明在 session 之前 —— 节拍器可能持着它的时钟。
    std::unique_ptr<kivo::video::avsync::AudioPlayback> audio;

    /// 探针的时钟台阶注入（P13-AB）。**声明在 `audio` 之后、`session` 之前**：
    /// 它持有 `audio`/`system_clock` 里那一个的引用（先于被引用者亡），
    /// 而 `session` 的节拍器持有它的指针（后于使用者亡）。
    std::unique_ptr<kivo::video::core::audio::ClockOffsetSource> offset_clock;

    std::unique_ptr<DecodeSession> session;
    /// 视频停下的那一刻的音频位置（见 AvSyncStats 同名注释）。
    std::int64_t audio_position_at_video_stop_ns = -1;
    std::int64_t source_bytes = 0;
    std::string source_label;
    std::string last_error;
};

namespace {

[[nodiscard]] bool write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    return static_cast<bool>(file);
}

/// 失败时同时把原因记到句柄上。返回码给 Java 判分支，原因给人看。
[[nodiscard]] int fail(SessionHandle* handle, int code, const std::string& reason) {
    if (handle != nullptr) {
        handle->last_error = reason;
    }
    return code;
}

/// 收尾计划的执行体（P12-D4）。七个动作，顺序由 `session_teardown_plan.hpp` 决定。
///
/// 这里**只写"每一步做什么"，不写"什么时候做"** —— 后者是那份 constexpr 数组的事。
/// 分成两半是为了让顺序能在桌面上被测：数组纯数据、执行器模板化，
/// JNI 侧只是提供一个 `Hooks` 实例。
struct SessionTeardownHooks {
    SessionHandle& handle;

    void stop_video() {
        if (handle.session) {
            handle.session->stop();
        }
    }

    void sample_audio_position() {
        if (handle.audio == nullptr || handle.audio->clock() == nullptr) {
            return;
        }
        // 只取第一次。`stop()` 之后再 `destroy()` 会走第二遍：
        // 那时音频已停，`now()` 读到的是停流后的回退档，
        // 再写一次就是拿坏数据覆盖好数据（P12-C8 那个 bug 的同一形状）。
        if (handle.audio_position_at_video_stop_ns >= 0) {
            return;
        }
        handle.audio_position_at_video_stop_ns = handle.audio->clock()->now().position_ns;
    }

    void stop_audio() {
        if (handle.audio) {
            handle.audio->stop();
        }
    }

    void reset_video() { handle.session.reset(); }
    void reset_audio() { handle.audio.reset(); }
    void reset_window() { handle.window.reset(); }
    void reset_clock() { handle.system_clock.reset(); }
};

}  // namespace

SessionHandle* session_create() {
    auto* handle = new SessionHandle();
    handle->session = std::make_unique<DecodeSession>();
    return handle;
}

void session_destroy(SessionHandle* handle) {
    if (handle == nullptr) {
        return;
    }
    // 走**完整**计划：停在前面、放资源在后面，一步不少。
    //
    // 这里刻意不自己写一遍顺序 —— 之前就是 `stop()` 和 `destroy()` 各写一份，
    // 两份只在"看起来一样"上保持一致。现在两边调的是同一个执行器。
    SessionTeardownHooks hooks{*handle};
    kivo::video::mediacodec::run_session_teardown(
        kivo::video::mediacodec::session_destroy_plan(), hooks);
    delete handle;
}

int session_open(SessionHandle* handle, JNIEnv* env, jobject surface, int fd,
                 std::int64_t length, const std::string& label, std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    if (surface == nullptr) {
        error = "surface is null";
        return fail(handle, 2, error);
    }
    if (handle->window) {
        error = "session already has a surface";
        return fail(handle, 3, error);
    }
    if (label.empty()) {
        error = "media label is empty";
        return fail(handle, 6, error);
    }

    ANativeWindow* raw_window = ANativeWindow_fromSurface(env, surface);
    if (raw_window == nullptr) {
        error = "ANativeWindow_fromSurface returned null (surface may be abandoned)";
        return fail(handle, 4, error);
    }
    handle->window = std::make_unique<SurfaceWindowBinder>(raw_window);

    const auto outcome = handle->session->open(fd, 0, length, label, raw_window);
    if (!outcome.ok()) {
        error = outcome.message;
        // 打开失败就不该占着窗口：析构行为与成功路径保持一致，避免残留引用。
        handle->window.reset();
        // P12-F3B：加密轨给单独返回码（12）。不合并进 5 的理由是取证：
        // "这份文件加密了"和"这份文件打不开"要能分开统计，否则 F3 的
        // 第四条判据只能靠翻 reason 字符串，而那正是最容易出错的地方。
        const bool encrypted = outcome.code ==
            kivo::video::mediacodec::SessionOutcome::Code::kEncryptedUnsupported;
        // 210：DV 拒绝给单独返回码（13），理由同加密那条：取证要能按码分开统计，
        // "这是 DV 档位不收"和"这份文件打不开"不能共用一个码。
        const bool dolby_vision_unsupported = outcome.code ==
            kivo::video::mediacodec::SessionOutcome::Code::kDolbyVisionUnsupported;
        return fail(handle, encrypted ? 12 : (dolby_vision_unsupported ? 13 : 5),
                    "open(" + label + "): " + error);
    }
    handle->last_error.clear();
    handle->source_bytes = length;
    handle->source_label = label;
    return 0;
}

int session_rebind_window(SessionHandle* handle, JNIEnv* env, jobject surface,
                          std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }

    // 摘面。
    //
    // **不放掉窗口引用**：codec 这时还指着它，release 完就是悬空指针。
    // 旧窗口留到"新面上好之后"再放（见下面成功分支），这一条是 P12-D1 第一个真机坑
    // 的直接产物：最初在这里 reset()，切回时驱动直接崩在 setOutputSurface 里。
    if (surface == nullptr) {
        const auto detached = handle->session->rebind_window(nullptr);
        if (!detached.ok()) {
            error = detached.message;
            return fail(handle, 12, "detach surface: " + error);
        }
        handle->last_error.clear();
        return 0;
    }

    ANativeWindow* raw_window = ANativeWindow_fromSurface(env, surface);
    if (raw_window == nullptr) {
        error = "ANativeWindow_fromSurface returned null (surface may be abandoned)";
        return fail(handle, 4, error);
    }

    // 先挂新面，再放旧的：`rebind_window` 成功之后 codec 已经不再引用旧窗口，
    // 这时的 release 才是安全的。反过来的话，中间那一瞬 codec 指着一个已释放的窗口。
    const auto rebound = handle->session->rebind_window(raw_window);
    if (!rebound.ok()) {
        error = rebound.message;
        // 换面失败：新窗口不能留着（没人持有它了），旧窗口也别丢 ——
        // 丢了之后 codec 还指着它。这句 release 只是把引用计数还回去。
        ANativeWindow_release(raw_window);
        return fail(handle, 13, "rebind surface: " + error);
    }
    handle->window = std::make_unique<SurfaceWindowBinder>(raw_window);
    handle->last_error.clear();
    return 0;
}

int session_start(SessionHandle* handle, std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    const auto outcome = handle->session->start();
    if (!outcome.ok()) {
        error = outcome.message;
        return fail(handle, 7, "start: " + error);
    }
    handle->last_error.clear();
    return 0;
}

int session_seek(SessionHandle* handle, std::int64_t target_us, std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }

    // P12-F2A 的编排顺序，与 F2 相反：
    //
    //   1. **视频先定位**（defer_start：只停线程 → 定位 → flush，不起跑），拿到**实际落点**；
    //   2. 音频按**视频的落点**定位；
    //   3. 视频再起跑；
    //   4. 把音频落点并进同一份证据。
    //
    // 为什么反过来了：视频走 `PREVIOUS_SYNC` 会落到关键帧上，与目标可能差一整个 GOP。
    // 实测后向 seek 到 500 ms —— 视频落 0、音频落 490 ms ⇒ gap 490 ms，按自己的判据就是"脱节"。
    // 让音频去追视频的落点，两边才在**同一个媒体时刻**开始。
    //
    // 为什么不能"音频先跳"：F2 那样做时视频会先起跑，而音频时钟还在旧位置，
    // 节拍器按旧时钟排帧 —— 那是 F2 里另一个隐患。现在视频起跑被推到音频就绪之后。
    const auto outcome = handle->session->seek_to(target_us, /*defer_start=*/true);
    // 线程已停，snapshot 在这里读是安全的（也正因为如此才不需要为 landing 单开一个 getter）。
    const std::int64_t video_landing = handle->session->snapshot().seek.landing_us;

    bool audio_present = false;
    std::int64_t audio_landing_us = -1;
    std::string audio_error;
    if (handle->audio != nullptr && handle->audio->is_started()) {
        audio_present = true;
        const std::int64_t audio_target = video_landing >= 0 ? video_landing : target_us;
        const auto audio_result = handle->audio->seek_to(audio_target);
        audio_landing_us = audio_result.landing_us;
        if (!audio_result.ok) {
            audio_error = audio_result.reason;
        }
    }

    // 无论前面成没成都要起跑：否则会话停在"线程已 join"的状态，下一次 snapshot 像卡死。
    const auto resumed = handle->session->resume_after_seek();
    handle->session->record_audio_seek(audio_present, audio_landing_us);

    if (!outcome.ok()) {
        error = outcome.message;
        const bool rejected =
            outcome.code == kivo::video::mediacodec::SessionOutcome::Code::kSeekRejected;
        return fail(handle, rejected ? 9 : 10, "seek: " + error);
    }
    if (!resumed.ok()) {
        error = resumed.message;
        return fail(handle, 10, "seek resume: " + error);
    }
    if (!audio_error.empty()) {
        // 音频失败不中止视频：画面仍然可看，而"不同步"这件事由证据里的 `seek.av` 明确写出。
        error = audio_error;
        return fail(handle, 11, "seek audio: " + error);
    }
    handle->last_error.clear();
    return 0;
}

void session_stop(SessionHandle* handle) {
    if (handle == nullptr) {
        return;
    }
    // 走计划的前三步：停视频 → 取此刻音频位置 → 停音频。
    //
    // 这三步的共同点是"必须发生在释放任何资源之前"，所以它们和
    // `session_destroy()` 是同一份计划的**前缀**，不是另写一遍。
    //
    // 音频那一步不能省：`AudioPlayback::stop()` 才是把 `feed` / `output` / `clock`
    // 三组统计填成终值的地方 —— 少了它，`kivo.p12c.sync/1` 里的音频段全是 0，
    // 而 `clock_source` 会因为 `queries == 0` 退回 `system`。
    // C8 第一轮就是这么写坏的（sync 报告里 clock_mix 说用了音频时钟，
    // 顶层 clock_source 却说 system，两个字段自相矛盾）。
    SessionTeardownHooks hooks{*handle};
    kivo::video::mediacodec::run_session_teardown(
        kivo::video::mediacodec::session_stop_plan(), hooks);
}

int session_set_paused(SessionHandle* handle, bool paused, std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    if (paused) {
        // P16-P1B32：**先停音频**（立刻静音），再停视频线程。
        // 只停视频的话声音会继续放 —— 这是 P1B31 的遗漏，静态审查才发现。
        if (handle->audio) {
            // 音频停不下来不阻塞视频暂停：画面停住比声音多放一秒更重要。
            // 平台拒绝时 `AaudioOutputSession::pause()` 已返回 false，这里不额外造错误码。
            (void)handle->audio->pause();
        }
        handle->session->pause();
        handle->last_error.clear();
        return 0;
    }
    // 恢复：先起音频（时钟先走），再起视频线程 —— 与 `seek_to` 的顺序一致。
    if (handle->audio) {
        (void)handle->audio->resume();
    }
    const auto outcome = handle->session->resume();
    if (!outcome.ok()) {
        error = outcome.message;
        return fail(handle, 12, "resume: " + error);
    }
    handle->last_error.clear();
    return 0;
}

int session_set_pacing(SessionHandle* handle, int target_fps, bool allow_drop,
                       int clock_offset_ms, int clock_offset_at_ms, int clock_rate_ppm,
                       bool drift_gate, bool step_repay, int drift_band_ms,
                       std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }

    // 时钟必须先于会话存在：会话里的节拍器只持指针，不持所有权。
    //
    // P12-C8：有音频就接音频主时钟 —— 这是"做没做 A/V 同步"的唯一开关。
    // 没有音频才退回 `SystemClockSource`，那时证据里的 `clock_source` 会写 `system`。
    kivo::video::core::audio::AudioClockSource* clock = nullptr;
    if (handle->audio && handle->audio->is_started()) {
        clock = handle->audio->clock();
    }
    if (clock == nullptr) {
        handle->system_clock = std::make_unique<kivo::video::core::audio::SystemClockSource>();
        clock = handle->system_clock.get();
    }

    // P13-AB：探针注入。包一层，不改变时钟链本身 —— 节拍器看到的仍是一个
    // `AudioClockSource`，差别只在位置里多了一次给定的台阶。
    //
    // 不带参数（0）时 `offset_clock` 保持空，`clock` 原样传下去：生产路径上
    // 一个分支都不多。这也是为什么注入点选在这一层，而不是往 tracker 里塞开关。
    if (clock_offset_ms != 0 || clock_rate_ppm != 0) {
        handle->offset_clock = std::make_unique<kivo::video::core::audio::ClockOffsetSource>(
            *clock, static_cast<std::int64_t>(clock_offset_ms) * 1'000'000,
            static_cast<std::int64_t>(clock_offset_at_ms) * 1'000'000, clock_rate_ppm);
        clock = handle->offset_clock.get();
    }

    kivo::video::mediacodec::PacingSetup setup;
    setup.clock = clock;
    setup.policy.allow_drop = allow_drop;
    // P13-AD：门关 = 对照组。位置完全按时钟走，门一次都不触发 ——
    // 没有这一组，"门没触发"就分不清"该沉默时沉默"和"根本没接上"。
    setup.policy.drift_gate_enabled = drift_gate;
    // P14-Z：台阶还不还。默认 true；false 只用于对照轮，且会出版到证据里。
    setup.policy.step_repay = step_repay;
    // P16-P1B17：迟滞带可调。默认 5 ms（不传 = 行为不变）。
    // 调大能把"小步高频"换成"大步低频"（`197` 的推演），代价是单次修正更大。
    // 只用于对照实验；必须小于门限，否则 `target = limit − band` 会变成负数。
    const std::int64_t band_ns = static_cast<std::int64_t>(drift_band_ms) * 1'000'000;
    if (band_ns > 0 && band_ns < setup.policy.drift_limit_ns) {
        setup.policy.drift_band_ns = band_ns;
    }
    if (target_fps > 0) {
        // 只是首帧兜底：真实帧时长由 pts 差值观测得出，不信这里传的值。
        setup.policy.default_frame_duration_ns = 1'000'000'000 / target_fps;
    }

    const auto outcome = handle->session->attach_pacing(setup);
    if (!outcome.ok()) {
        error = outcome.message;
        return fail(handle, 8, "pacing: " + error);
    }
    handle->last_error.clear();
    return 0;
}

int session_set_pipeline(SessionHandle* handle, bool split, bool present, bool at_time,
                         std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    kivo::video::mediacodec::PipelineSetup setup;
    setup.mode = split ? kivo::video::mediacodec::PipelineMode::kSplit
                       : kivo::video::mediacodec::PipelineMode::kMerged;
    setup.present_frames = present;
    setup.present_schedule = at_time
                                 ? kivo::video::mediacodec::PresentSchedule::kSurfaceAtTime
                                 : kivo::video::mediacodec::PresentSchedule::kCpuPaced;

    const auto outcome = handle->session->attach_pipeline(setup);
    if (!outcome.ok()) {
        error = outcome.message;
        return fail(handle, 9, "pipeline: " + error);
    }
    handle->last_error.clear();
    return 0;
}

int session_write_stats(SessionHandle* handle, const std::string& path, std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    if (path.empty()) {
        error = "stats path is empty";
        return fail(handle, 2, error);
    }

    const auto stats = handle->session->snapshot();
    const std::string json = kivo::video::mediacodec::render_decode_stats_json(
        stats, kivo::video::mediacodec::now_unix_millis());
    if (!write_file(path, json)) {
        error = "cannot write stats to " + path;
        return fail(handle, 3, error);
    }
    return 0;
}

int session_open_audio(SessionHandle* handle, int fd, std::int64_t length,
                       const std::string& label, std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    if (handle->audio) {
        error = "audio playback already created";
        return fail(handle, 3, error);
    }
    if (length <= 0) {
        error = "audio source length is invalid";
        return fail(handle, 2, error);
    }

    auto playback = std::make_unique<kivo::video::avsync::AudioPlayback>();
    if (!playback->open(fd, length, label)) {
        error = playback->stats().error;
        return fail(handle, 10, "audio open: " + error);
    }
    // `start()` 会起流并等到时钟首个有效时间戳。**必须在视频起播之前完成** ——
    // 否则前几十帧会读到不推进的时钟，节拍器切成自生成时间轴，整轮 av_drift 变噪声。
    if (!playback->start()) {
        error = playback->stats().error;
        return fail(handle, 11, "audio start: " + error);
    }
    handle->audio = std::move(playback);
    handle->last_error.clear();
    return 0;
}

int session_write_sync_stats(SessionHandle* handle, const std::string& path,
                             std::string& error) {
    if (handle == nullptr || !handle->session) {
        error = "session handle is null";
        return 1;
    }
    if (path.empty()) {
        error = "sync stats path is empty";
        return fail(handle, 2, error);
    }

    kivo::video::avsync::AvSyncStats stats;
    stats.source_label = handle->source_label;
    stats.source_bytes = handle->source_bytes;
    stats.has_video = true;
    stats.video = handle->session->snapshot();
    // 只在 stop() 之后调：这个接口读节拍器的完整直方图，跑起来时读就是数据竞争。
    stats.pace = handle->session->pace_summary_after_stop();
    stats.audio_position_at_video_stop_ns = handle->audio_position_at_video_stop_ns;
    if (handle->offset_clock != nullptr) {
        stats.clock_injection.offset_ns = handle->offset_clock->offset_ns();
        stats.clock_injection.arm_after_ns = handle->offset_clock->arm_after_ns();
        stats.clock_injection.armed_at_position_ns =
            handle->offset_clock->armed_at_position_ns();
        stats.clock_injection.rate_ppm = handle->offset_clock->rate_ppm();
    }
    stats.anchor_video_pts_ns = stats.pace.anchor_pts_ns;
    stats.anchor_audio_position_ns = stats.pace.anchor_position_ns;
    if (handle->audio) {
        stats.has_audio = handle->audio->is_open();
        stats.audio = handle->audio->stats();
    }
    stats.device = kivo::video::mediacodec::read_device_identity();
    stats.generated_at_unix_ms = kivo::video::mediacodec::now_unix_millis();

    const std::string json = kivo::video::avsync::render_av_sync_json(stats);
    if (!write_file(path, json)) {
        error = "cannot write sync stats to " + path;
        return fail(handle, 3, error);
    }
    return 0;
}

std::string session_summary(SessionHandle* handle) {
    if (handle == nullptr || !handle->session) {
        return "session=<null>";
    }
    const auto stats = handle->session->snapshot();

    char buffer[640] = {};
    std::snprintf(buffer, sizeof(buffer),
                  "codec=%s mime=%s %dx%d pipe=%s frames=%lld recv=%lld rel=%lld dropped=%lld "
                  "samples=%lld wall=%.3fs fps=%.2f speed=%.2fx first_frame=%.1fms pace=%s "
                  // 口径写进键名，不是写在旁边的注释里：这条日志是 `adb logcat` 里最常被
                  // 复制的一行，复制出去就只剩 "p99=0.1ms"。这个 p99 是**逐帧**的
                  // 节拍迟到量（每帧都进直方图），与 `av_drift` 里抽稀口径的 p99
                  // 不是同一个数，对着比会得出"两处矛盾"的假结论。
                  "pace_lateness_p99[per_frame]=%.1fms completed=%d error=%s",
                  stats.codec_name.c_str(), stats.mime.c_str(), stats.width, stats.height,
                  stats.pipeline_mode.c_str(),
                  static_cast<long long>(stats.frames_rendered),
                  static_cast<long long>(stats.frames_received),
                  static_cast<long long>(stats.frames_released),
                  static_cast<long long>(stats.frames_dropped_late),
                  static_cast<long long>(stats.samples_submitted),
                  static_cast<double>(stats.wall_micros) / 1'000'000.0, stats.decode_fps,
                  stats.speed_vs_realtime,
                  static_cast<double>(stats.first_frame_latency_us) / 1000.0,
                  stats.pace.enabled ? stats.pace.clock_source.c_str() : "off",
                  static_cast<double>(stats.pace.lateness_p99_ns) / 1'000'000.0,
                  stats.completed ? 1 : 0, stats.error.empty() ? "-" : stats.error.c_str());
    return std::string(buffer);
}

std::string session_last_error(SessionHandle* handle) {
    if (handle == nullptr) {
        return "session handle is null";
    }
    return handle->last_error;
}

void session_record_native_error(SessionHandle* handle, const char* where,
                                 const char* what) noexcept {
    if (handle == nullptr) {
        return;  // 无句柄的导出函数：原因已由 KIVO_LOGW 留下，没有可回传的通道
    }
    // 逐段 append 而不是一次拼接：即使这里在极端 OOM 下再次抛，
    // 也被吞掉并保住 noexcept 边界 —— 宁可丢了原因，不能让进程 terminate。
    try {
        std::string& out = handle->last_error;
        out = "native exception in ";
        out += where != nullptr ? where : "unknown entry";
        out += ": ";
        out += what != nullptr ? what : "no detail";
    } catch (...) {
    }
}

}  // namespace kivo::jni_bridge
