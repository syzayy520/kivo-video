#include "video/video_plane/backends/aaudio/device/aaudio_stream_opener.hpp"

#include <string>
#include <utility>

#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"

namespace kivo::video::aaudio {
namespace {

aaudio_format_t to_platform_format(AaudioStreamConfig::SampleFormat format) {
    return (format == AaudioStreamConfig::SampleFormat::kInt16) ? AAUDIO_FORMAT_PCM_I16
                                                                : AAUDIO_FORMAT_PCM_FLOAT;
}

/// 结果码 → "名字(数值)"。只给 unknown 补数值，
/// 因为已知名字本身已经够读，多打数字反而噪声。
std::string describe_result(aaudio_result_t result) {
    const auto raw = static_cast<std::int32_t>(result);
    std::string text(result_name(raw));
    if (text == "unknown") {
        text += "(";
        text += std::to_string(raw);
        text += ")";
    }
    return text;
}

/// 建 builder 并配好全部参数。失败时保证不泄漏 builder。
aaudio_result_t create_builder(const AaudioStreamOpener::Request& request,
                               aaudio_sharing_mode_t sharing_mode,
                               AAudioStreamBuilder** out_builder) {
    *out_builder = nullptr;

    AAudioStreamBuilder* builder = nullptr;
    const aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        return result;
    }
    if (builder == nullptr) {
        return AAUDIO_ERROR_NULL;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSampleRate(builder, request.config.sample_rate);
    AAudioStreamBuilder_setChannelCount(builder, request.config.channel_count);
    AAudioStreamBuilder_setFormat(builder, to_platform_format(request.config.format));
    AAudioStreamBuilder_setSharingMode(builder, sharing_mode);
    AAudioStreamBuilder_setPerformanceMode(
        builder, request.config.prefer_low_latency ? AAUDIO_PERFORMANCE_MODE_LOW_LATENCY
                                                   : AAUDIO_PERFORMANCE_MODE_NONE);
    if (request.config.buffer_capacity_frames > 0) {
        AAudioStreamBuilder_setBufferCapacityInFrames(builder,
                                                     request.config.buffer_capacity_frames);
    }
    if (request.data_callback != nullptr) {
        AAudioStreamBuilder_setDataCallback(builder, request.data_callback, request.user_data);
    }

    *out_builder = builder;
    return AAUDIO_OK;
}

void fill_negotiated(AAudioStream* stream, AaudioOpenOutcome& outcome) {
    outcome.sample_rate = AAudioStream_getSampleRate(stream);
    outcome.channel_count = AAudioStream_getChannelCount(stream);
    outcome.platform_format = static_cast<std::int32_t>(AAudioStream_getFormat(stream));
    outcome.platform_sharing_mode =
        static_cast<std::int32_t>(AAudioStream_getSharingMode(stream));
    outcome.platform_performance_mode =
        static_cast<std::int32_t>(AAudioStream_getPerformanceMode(stream));
    outcome.frames_per_burst = AAudioStream_getFramesPerBurst(stream);
    outcome.buffer_capacity_frames = AAudioStream_getBufferCapacityInFrames(stream);
}

/// 把「参数被平台改掉」也记进降级原因。
///
/// 这属于静默降级的变种：我们要 float/48k，设备给了 i16/44.1k 也能出声，
/// 但如果没人记录，后面 A/V 漂移查不出来是采样率不是我们要的那个。
void append_negotiation_notes(const AaudioStreamConfig& config, AaudioOpenOutcome& outcome) {
    std::string notes;
    const auto append = [&notes](std::string text) {
        if (!notes.empty()) {
            notes += "；";
        }
        notes += std::move(text);
    };

    // 共享/性能模式是「提示」而不是契约：openStream 可能成功，
    // 但系统悄悄把独占降成共享、把低延迟降成普通。
    // 这不是失败，可必须记录 —— 独占与否直接决定延迟量级，
    // 正是 G7 能不能达标的前提。本机实测就是这种情况：请求独占、拿到共享。
    // 若已经因为「独占被拒后回退」写过原因，这里不重复第二遍。
    if (config.prefer_exclusive && outcome.downgrade_reason.empty() &&
        outcome.platform_sharing_mode != static_cast<std::int32_t>(AAUDIO_SHARING_MODE_EXCLUSIVE)) {
        append(std::string("申请独占但实际为 ") +
               std::string(sharing_mode_name(outcome.platform_sharing_mode)));
    }
    if (config.prefer_low_latency &&
        outcome.platform_performance_mode !=
            static_cast<std::int32_t>(AAUDIO_PERFORMANCE_MODE_LOW_LATENCY)) {
        append(std::string("申请低延迟但实际为 ") +
               std::string(performance_mode_name(outcome.platform_performance_mode)));
    }

    if (outcome.platform_format != static_cast<std::int32_t>(to_platform_format(config.format))) {
        append(std::string("采样格式被改为 ") + std::string(platform_format_name(outcome.platform_format)));
    }
    if (outcome.sample_rate != config.sample_rate) {
        append("采样率被改为 " + std::to_string(outcome.sample_rate));
    }
    if (outcome.channel_count != config.channel_count) {
        append("声道数被改为 " + std::to_string(outcome.channel_count));
    }

    if (notes.empty()) {
        return;
    }
    if (!outcome.downgrade_reason.empty()) {
        outcome.downgrade_reason += "；";
    }
    outcome.downgrade_reason += notes;
}

}  // namespace

AaudioOpenOutcome AaudioStreamOpener::open(const Request& request, AAudioStream** out_stream) {
    AaudioOpenOutcome outcome;

    if (out_stream == nullptr) {
        outcome.code = AaudioOpenOutcome::Code::kInvalidConfig;
        outcome.message = "out_stream 为空指针";
        return outcome;
    }
    *out_stream = nullptr;

    if (!request.config.plausible()) {
        outcome.code = AaudioOpenOutcome::Code::kInvalidConfig;
        outcome.message = "请求参数非法: sample_rate=" + std::to_string(request.config.sample_rate) +
                          " channel_count=" + std::to_string(request.config.channel_count);
        return outcome;
    }

    // 尝试顺序：要独占就先独占、失败再共享；不要独占就直接共享。
    aaudio_sharing_mode_t attempts[2] = {AAUDIO_SHARING_MODE_SHARED, AAUDIO_SHARING_MODE_SHARED};
    int attempt_count = 0;
    if (request.config.prefer_exclusive) {
        attempts[attempt_count++] = AAUDIO_SHARING_MODE_EXCLUSIVE;
    }
    attempts[attempt_count++] = AAUDIO_SHARING_MODE_SHARED;

    std::string first_failure;

    for (int index = 0; index < attempt_count; ++index) {
        AAudioStreamBuilder* builder = nullptr;
        const aaudio_result_t build_result = create_builder(request, attempts[index], &builder);
        if (build_result != AAUDIO_OK) {
            outcome.code = AaudioOpenOutcome::Code::kBuilderFailed;
            outcome.message = "创建 builder 失败: " + describe_result(build_result);
            return outcome;
        }

        AAudioStream* stream = nullptr;
        const aaudio_result_t open_result = AAudioStreamBuilder_openStream(builder, &stream);

        // builder 用完即弃：不复用它重开。厂商实现对「开失败的 builder」状态处理不一致，
        // 重建一个的成本可以忽略，换来的是降级路径不会踩到脏状态。
        AAudioStreamBuilder_delete(builder);

        if (open_result == AAUDIO_OK && stream != nullptr) {
            fill_negotiated(stream, outcome);
            if (index > 0) {
                outcome.downgrade_reason =
                    "独占模式被拒(" + first_failure + ")，已回退共享模式";
            }
            append_negotiation_notes(request.config, outcome);
            *out_stream = stream;
            return outcome;
        }

        if (stream != nullptr) {
            AAudioStream_close(stream);
        }
        if (first_failure.empty()) {
            first_failure = describe_result(open_result);
        }
    }

    outcome.code = AaudioOpenOutcome::Code::kOpenFailed;
    outcome.message = "AAudio 开流失败: " + (first_failure.empty() ? std::string("未知") : first_failure);
    return outcome;
}

}  // namespace kivo::video::aaudio
