#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"

#include <algorithm>
#include <chrono>
#include <string>

#include "video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_stream_opener.hpp"

namespace kivo::video::aaudio {
namespace {

// 状态等待上限。AAudio 的 start/stop 都是异步请求，
// 等不到就如实报超时，不假装成功。
constexpr std::int64_t kStateWaitNanos = 200'000'000;  // 200 ms

std::int64_t steady_now_millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

AaudioOutputSession::~AaudioOutputSession() { close(); }

AaudioOpenOutcome AaudioOutputSession::open(const AaudioStreamConfig& config, SampleSource& source) {
    close();

    AaudioStreamOpener::Request request;
    request.config = config;
    request.data_callback = &AaudioOutputSession::on_data;
    request.user_data = this;

    AAudioStream* stream = nullptr;
    outcome_ = AaudioStreamOpener::open(request, &stream);

    if (stream == nullptr) {
        if (outcome_.ok()) {
            outcome_.code = AaudioOpenOutcome::Code::kStreamInvalid;
            outcome_.message = "开流报告成功但流指针为空";
        }
        return outcome_;
    }

    // 回调通路只实现了 float。协商到定点必须当场拒绝：
    // 按 float 解释 int16 数据不会崩溃，只会出一片噪声，比崩溃更难查。
    if (outcome_.platform_format != AAUDIO_FORMAT_PCM_FLOAT) {
        AAudioStream_close(stream);
        outcome_.code = AaudioOpenOutcome::Code::kOpenFailed;
        outcome_.message = std::string("回调通路只支持 float，实际协商到 ") +
                           std::string(platform_format_name(outcome_.platform_format));
        return outcome_;
    }

    // P16-P1B2 / G9：burst 是时钟陈旧阈值的输入，取不到就不能猜。
    //
    // 旧行为是让 AaudioClockSource 悄悄退回 192（K40 在 48 kHz 下的 4 ms）：
    // 证据里会写成"协商到 192"，与真协商到 192 无法区分；而别的设备上
    // stale_after_ns = 2 × burst / sample_rate 会把"新鲜"误判成"陈旧"，
    // 进而触发 FreeRun 降级，G7 的数字跟着失真。
    //
    // 按 Direct Play First 的既有约定（AudioPlayback::open 对采样率不一致直接拒绝），
    // 这里也明确失败，并带非空 reason 走既有失败链：
    //   message → stats_.error → av_sync 的 audio.error / completed=false → last_error → UI。
    // 检查必须放在 stream_ 赋值之前，失败路径与上面 format 分支逐字一致。
    if (outcome_.frames_per_burst <= 0) {
        AAudioStream_close(stream);
        outcome_.code = AaudioOpenOutcome::Code::kOpenFailed;
        outcome_.message = "AAudioStream_getFramesPerBurst returned " +
                           std::to_string(outcome_.frames_per_burst) +
                           "; refusing to fall back to 192 because device burst is required "
                           "for the audio clock stale threshold (G9: no silent downgrade)";
        return outcome_;
    }

    stream_ = stream;
    source_ = &source;

    // 时钟必须在拿到协商结果之后才建：陈旧阈值要按实际 burst 长度算，
    // 用请求值算会在设备给出不同 burst 时把"新鲜"误判成"陈旧"。
    clock_reader_ = std::make_unique<AaudioTimestampReader>(stream_);
    clock_ = std::make_unique<AaudioClockSource>(*clock_reader_, outcome_.sample_rate,
                                                 outcome_.frames_per_burst);
    return outcome_;
}

AaudioOpenOutcome AaudioOutputSession::start() {
    if (stream_ == nullptr) {
        AaudioOpenOutcome outcome;
        outcome.code = AaudioOpenOutcome::Code::kStreamInvalid;
        outcome.message = "会话未开流，无法启动";
        return outcome;
    }

    xrun_baseline_ = AAudioStream_getXRunCount(stream_);
    started_at_millis_ = steady_now_millis();
    // 必须在 requestStart 之前取：这就是 C4 计算启动开销的 t0。
    start_mono_ns_ = monotonic_now_ns();

    const aaudio_result_t request_result = AAudioStream_requestStart(stream_);
    if (request_result != AAUDIO_OK) {
        started_ = false;
        AaudioOpenOutcome outcome = outcome_;
        outcome.code = AaudioOpenOutcome::Code::kOpenFailed;
        outcome.message = std::string("请求启动失败: ") +
                          std::string(result_name(static_cast<std::int32_t>(request_result)));
        return outcome;
    }

    // requestStart 是异步的。必须确认真的进了 STARTED，
    // 否则后面统计出 0 帧时，会让人误判成"设备没出声"而不是"根本没启动成功"。
    aaudio_stream_state_t state = AAudioStream_getState(stream_);
    if (state == AAUDIO_STREAM_STATE_STARTING) {
        aaudio_stream_state_t next = state;
        AAudioStream_waitForStateChange(stream_, state, &next, kStateWaitNanos);
        state = next;
    }

    if (state != AAUDIO_STREAM_STATE_STARTED) {
        started_ = false;
        AaudioOpenOutcome outcome = outcome_;
        outcome.code = AaudioOpenOutcome::Code::kOpenFailed;
        outcome.message = "流未进入 STARTED，当前状态码 " + std::to_string(static_cast<int>(state));
        return outcome;
    }

    started_ = true;
    return outcome_;
}

bool AaudioOutputSession::pause() {
    if (stream_ == nullptr || !started_) {
        return false;
    }
    if (AAudioStream_requestPause(stream_) != AAUDIO_OK) {
        return false;
    }
    aaudio_stream_state_t state = AAudioStream_getState(stream_);
    if (state == AAUDIO_STREAM_STATE_PAUSING) {
        aaudio_stream_state_t next = state;
        AAudioStream_waitForStateChange(stream_, state, &next, kStateWaitNanos);
    }
    return true;
}

bool AaudioOutputSession::resume() {
    if (stream_ == nullptr) {
        return false;
    }
    return AAudioStream_requestStart(stream_) == AAUDIO_OK;
}

void AaudioOutputSession::stop() {
    if (stream_ == nullptr) {
        started_ = false;
        return;
    }
    if (AAudioStream_requestStop(stream_) == AAUDIO_OK) {
        aaudio_stream_state_t state = AAudioStream_getState(stream_);
        if (state == AAUDIO_STREAM_STATE_STOPPING) {
            aaudio_stream_state_t next = state;
            AAudioStream_waitForStateChange(stream_, state, &next, kStateWaitNanos);
        }
    }
    started_ = false;
}

void AaudioOutputSession::close() {
    // 时钟持有 stream 的裸指针，必须先于 stream 销毁。
    clock_.reset();
    clock_reader_.reset();

    if (stream_ == nullptr) {
        started_ = false;
        source_ = nullptr;
        return;
    }
    stop();
    AAudioStream_close(stream_);
    stream_ = nullptr;
    source_ = nullptr;
}

AudioOutputStats AaudioOutputSession::stats() const {
    AudioOutputStats snapshot;
    snapshot.callbacks = callbacks_.load(std::memory_order_relaxed);
    snapshot.frames_rendered = frames_rendered_.load(std::memory_order_relaxed);
    snapshot.frames_silenced = frames_silenced_.load(std::memory_order_relaxed);
    snapshot.first_audible_frame = first_audible_frame_.load(std::memory_order_relaxed);
    snapshot.started = started_;

    // 头尾分解：首块真实数据之前的静音必然算头，其余算尾。
    // 全程没拿到数据时全算头部 —— 那不是"收尾排空"，是压根没供上。
    if (snapshot.first_audible_frame >= 0) {
        snapshot.silence_head_frames =
            std::min(snapshot.first_audible_frame, snapshot.frames_silenced);
        snapshot.silence_tail_frames = snapshot.frames_silenced - snapshot.silence_head_frames;
    } else {
        snapshot.silence_head_frames = snapshot.frames_silenced;
        snapshot.silence_tail_frames = 0;
    }

    if (stream_ != nullptr) {
        const std::int32_t cumulative = AAudioStream_getXRunCount(stream_);
        snapshot.xrun_count = cumulative - xrun_baseline_;
        if (snapshot.xrun_count < 0) {
            snapshot.xrun_count = 0;
        }
    }
    if (started_) {
        snapshot.wall_millis = steady_now_millis() - started_at_millis_;
    }
    return snapshot;
}

aaudio_data_callback_result_t AaudioOutputSession::on_data(AAudioStream*,
                                                           void* user_data,
                                                           void* audio_data,
                                                           std::int32_t num_frames) {
    auto* self = static_cast<AaudioOutputSession*>(user_data);
    if (self != nullptr && audio_data != nullptr && num_frames > 0) {
        self->render_into(static_cast<float*>(audio_data), num_frames);
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AaudioOutputSession::render_into(float* target, std::int32_t frame_count) {
    const std::int32_t channels = (outcome_.channel_count > 0) ? outcome_.channel_count : 1;

    std::int32_t produced = 0;
    if (source_ != nullptr) {
        produced = source_->render(target, frame_count);
        produced = std::clamp(produced, 0, frame_count);
    }

    // 记"首次拿到真实数据"的帧序号。必须在 frames_rendered_ 累加**之前**取，
    // 这样记下的是本块的起始序号；也必须在补静音**之前**取，
    // 否则会把本块补的静音算进头部。
    if (produced > 0 && first_audible_frame_.load(std::memory_order_relaxed) < 0) {
        first_audible_frame_.store(frames_rendered_.load(std::memory_order_relaxed),
                                   std::memory_order_relaxed);
    }

    if (produced < frame_count) {
        const auto offset = static_cast<std::size_t>(produced) * static_cast<std::size_t>(channels);
        const auto samples =
            static_cast<std::size_t>(frame_count - produced) * static_cast<std::size_t>(channels);
        std::fill_n(target + offset, samples, 0.0F);
        frames_silenced_.fetch_add(frame_count - produced, std::memory_order_relaxed);
    }

    callbacks_.fetch_add(1, std::memory_order_relaxed);
    frames_rendered_.fetch_add(frame_count, std::memory_order_relaxed);
}

}  // namespace kivo::video::aaudio
