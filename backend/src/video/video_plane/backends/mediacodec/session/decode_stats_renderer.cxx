#include "video/video_plane/backends/mediacodec/session/decode_stats_renderer.hpp"

#include "video/video_plane/backends/mediacodec/decode/color_format_names.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"
#include "video/video_plane/backends/mediacodec/source/encryption_key_names.hpp"
#include "video/video_plane/core/seek/seek_budget.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_json_string_escaper.hpp"

#include <cstdio>
#include <sstream>

namespace kivo::video::mediacodec {
namespace {

/// 版本历史：
///   /1 → /2 → /3 见 `14-p12b-*.md` 与 `17-p12c5-design.md`
///   /4（C7）：加 `pipeline.present_schedule` 与 `pace.schedule_lead_ns`。
///            只加字段不改旧字段，/3 的消费方仍能读；版本号照样要升 ——
///            否则一份带 schedule_lead 的证据会被当成 /3 解释，
///            读者找不到"为什么 lateness 全是 0"。
///   /7（E1）：加 `color`（源 profile / 实际输出格式 / 位深判定 / 不匹配时的理由）。
///   /8（E2）：`color` 加源侧色彩键与下发事实（`source_color_*` / `delivered_color_*` /
///            `configure_format`），并新增 `color.hdr`（分类依据 / 静态元数据字节数 /
///            dataspace 设置结果）。**只加字段不改旧字段**，/7 的消费方仍能读。
///            版本号必须升：一份带 `hdr` 的证据被当成 /7 解释，读者会以为
///            "HDR 这一段没测"，而不是"这份文件里本来就没有它"。
///   /13（P13-B）：新增 `cold_start` 段（G8a：冷启动首帧 ≤ 200 ms）。
///            **和 `timing.first_frame_latency_us` 不是一回事**：后者在 seek 后会被重置，
///            只反映最后一段；`cold_start` 只在第一次 start 时取值，seek 不覆盖。
///            两者混读会得出"4K 超预算"的假结论 —— P13-A 就是这么误判的。
///   /12（F3B）：新增 `source.encryption`（`crypto-mode` / IV 长度 / 判定 / reason）。
///            **判定要能区分三种情况**：声明加密、明确声明不加密、平台没说。
///            把第三种写成"不加密"正是 F3 第一版的错。
///   /11（F1）：新增 `seek` 段（target / landing / prepare / first_frame / total /
///            budget / pass / reason）。判定用 `assess_seek()`，与纯函数同源。
///            **`requests == 0` 时 `pass` 必须是 false**：没 seek 过的运行不能证明 G8。
///   /10（E4）：`color.hdr` 加 `tonemap`（是否实现 / 为什么没做）。
///            **这个字段的价值是"写清没做"**：没有它，读者只能从一堆颜色字段里猜。
///   /9（E3）：`color.hdr.dataspace` 加 `readback`（窗口回读）。
///            **`applied` 与 `readback.ok` 是两个不同的事实**：前者是"调用成功"，
///            后者是"窗口上现在是这个值"；都不等于"合成器按 HDR 处理"。
///   /6（D1）：加 `surface_rebuild`（换面次数、空窗、恢复耗时、领先帧数）。
///   /5（C8）：加 `pace.clock_mix`（逐帧时钟分档）。
///            音画偏差**没有**加进这里：它等于 `lateness`，只在 `cpu_paced` 下成立，
///            单独放一份账会与 `lateness` 打架。要读它去看 `kivo.p12c.sync/1`。
///   /14（P14-A）：新增 `metrics` —— 三把以前没有的尺子（CPU 占比 / 堆分配次数 /
///            输出缓冲归属）。**每个都带可用性与理由**：这一族最容易把"没测到"
///            印成 0，而 0 看起来永远是好消息。
constexpr const char* kSchema = "kivo.p12c.decode/14";

/// 可选 int：`< 0` 一律写 `null`。
/// E1 定的规矩（`08-p12-family-tree.md` §三之十二 第 3 条）：未知不写 0/-1，
/// 否则会被读成"值就是 0"。这里抽成函数，避免新加的六个字段各写一遍 if。
void render_optional_int(std::ostringstream& out, const char* key, std::int32_t value,
                         bool trailing_comma) {
    out << "    \"" << key << "\": ";
    if (value < 0) {
        out << "null";
    } else {
        out << value;
    }
    out << (trailing_comma ? "," : "") << "\n";
}

[[nodiscard]] std::string fixed6(double value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    return std::string(buffer);
}

/// 一段平台调用的耗时。单独成函数是为了四段写法完全一致 ——
/// 手写四遍最容易出现"某一段漏了 max"这种看起来没问题、比的时候才发现没法比的错。
void render_stage_cost(std::ostringstream& out, const StageCost& cost, bool trailing_comma) {
    out << "      \"calls\": " << cost.calls << ",\n";
    out << "      \"mean_us\": " << fixed6(cost.mean_us()) << ",\n";
    out << "      \"total_us\": " << cost.total_us << ",\n";
    out << "      \"max_us\": " << cost.max_us << "\n";
    out << "    }" << (trailing_comma ? "," : "") << "\n";
}

}  // namespace

const char* decode_schema_id() {
    return kSchema;
}

std::string render_decode_stats_json(const DecodeStats& stats,
                                     std::int64_t generated_at_unix_ms) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"schema\": \"" << kSchema << "\",\n";
    out << "  \"generated_at_unix_ms\": " << generated_at_unix_ms << ",\n";

    out << "  \"source\": {\n";
    out << "    \"mime\": \"" << escape_json_string(stats.mime) << "\",\n";
    out << "    \"width\": " << stats.width << ",\n";
    out << "    \"height\": " << stats.height << ",\n";
    out << "    \"duration_us\": " << stats.media_duration_us << ",\n";

    // P12-F3B：轨道加密，**嵌在 source 里**。
    //
    // 这里原本另起过一个 "source" 键，于是 JSON 出现重复键 ——
    // 解析器取后者，source.mime/width/height/duration_us 被整个吞掉。
    // 直到 P13-A 播 4K 时读 source.width 才暴露。
    // 教训：证据里新增段落前先查这个键有没有被用过；
    // 总收口层 1 现在直接查重复键（见 verify_p12_all.py）。
    const EncryptionAssessment encryption = assess_encryption(stats.encryption);
    out << "    \"encryption\": {\n";
    out << "      \"crypto_mode_present\": " << (stats.encryption.mode_present ? "true" : "false")
        << ",\n";
    out << "      \"crypto_mode\": ";
    if (stats.encryption.mode_present) {
        out << stats.encryption.crypto_mode;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "      \"crypto_mode_name\": ";
    if (stats.encryption.mode_present) {
        out << "\"" << crypto_key::crypto_mode_name(stats.encryption.crypto_mode) << "\"";
    } else {
        out << "null";
    }
    out << ",\n";
    out << "      \"default_iv_size\": ";
    if (stats.encryption.default_iv_size >= 0) {
        out << stats.encryption.default_iv_size;
    } else {
        out << "null";
    }
    out << ",\n";
    out << "      \"key_declared\": " << (stats.encryption.key_declared ? "true" : "false")
        << ",\n";
    out << "      \"verdict\": \"" << encryption.verdict << "\",\n";
    out << "      \"reason\": ";
    if (encryption.reason.empty()) {
        out << "null\n";
    } else {
        out << "\"" << escape_json_string(encryption.reason) << "\"\n";
    }
    out << "    }\n";
    out << "  },\n";

    out << "  \"decoder\": {\n";
    out << "    \"codec_name\": \"" << escape_json_string(stats.codec_name) << "\",\n";
    out << "    \"output_target\": \"surface\"\n";
    out << "  },\n";

    // P12-E1：颜色。**判定在这里算，不在统计结构里算** —— 统计只记事实。
    // 未知一律写 `null`，不写 0/-1：0 会被读成"值就是 0"，null 才表示"没有这个事实"。
    const ColorAssessment color = assess_color(stats.color);
    out << "  \"color\": {\n";
    out << "    \"source_mime\": \"" << escape_json_string(stats.color.mime) << "\",\n";
    out << "    \"source_profile\": ";
    if (stats.color.source_profile < 0) {
        out << "null";
    } else {
        out << stats.color.source_profile;
    }
    out << ",\n";
    out << "    \"source_is_10bit\": ";
    if (!color.source_bit_depth_known) {
        out << "null";
    } else {
        out << (color.source_is_10bit ? "true" : "false");
    }
    out << ",\n";
    out << "    \"negotiated_format\": ";
    if (stats.color.negotiated_format < 0) {
        out << "null";
    } else {
        out << stats.color.negotiated_format;
    }
    out << ",\n";
    out << "    \"negotiated_name\": \""
        << escape_json_string(color_format::format_name(stats.color.negotiated_format))
        << "\",\n";
    out << "    \"negotiated_bit_depth\": ";
    const std::int32_t depth = color_format::format_bit_depth(stats.color.negotiated_format);
    if (depth == 0) {
        out << "null";
    } else {
        out << depth;
    }
    out << ",\n";
    out << "    \"color_standard\": " << stats.color.color_standard << ",\n";
    out << "    \"color_transfer\": " << stats.color.color_transfer << ",\n";
    out << "    \"color_range\": " << stats.color.color_range << ",\n";

    // P12-E2：源侧的三个键 + 我们实际下发的三个键。
    // 三段分列，是为了让"源没有标记"、"我们没发下去"、"平台不认"能一眼分开。
    render_optional_int(out, "source_color_standard", stats.color.source_color_standard, true);
    render_optional_int(out, "source_color_transfer", stats.color.source_color_transfer, true);
    render_optional_int(out, "source_color_range", stats.color.source_color_range, true);
    render_optional_int(out, "delivered_color_standard", stats.color.delivered_color_standard, true);
    render_optional_int(out, "delivered_color_transfer", stats.color.delivered_color_transfer, true);
    render_optional_int(out, "delivered_color_range", stats.color.delivered_color_range, true);

    out << "    \"configure_format\": {\n";
    out << "      \"copied\": " << (stats.color.configure_format_copied ? "true" : "false") << ",\n";
    out << "      \"fallback_reason\": ";
    if (stats.color.configure_fallback_reason.empty()) {
        out << "null\n";
    } else {
        out << "\"" << escape_json_string(stats.color.configure_fallback_reason) << "\"\n";
    }
    out << "    },\n";

    // P12-E2：HDR 交付事实。
    // `kind` 与 `kind_reason` 永远成对出现 —— 判成 sdr 也要说依据，
    // 否则读者分不清"查了、不是 HDR"与"根本没查"。
    out << "    \"hdr\": {\n";
    out << "      \"kind\": \"" << hdr_kind_name(stats.hdr.kind) << "\",\n";
    out << "      \"kind_reason\": \"" << escape_json_string(stats.hdr.kind_reason) << "\",\n";
    out << "      \"static_info_bytes\": ";
    if (stats.hdr.static_info_bytes < 0) {
        out << "null,\n";
    } else {
        out << stats.hdr.static_info_bytes << ",\n";
    }
    out << "      \"dataspace\": {\n";
    out << "        \"applied\": " << (stats.hdr.dataspace.applied ? "true" : "false") << ",\n";
    out << "        \"value\": " << stats.hdr.dataspace.value << ",\n";
    out << "        \"reason\": ";
    if (stats.hdr.dataspace.reason.empty()) {
        out << "null\n";
    } else {
        out << "\"" << escape_json_string(stats.hdr.dataspace.reason) << "\"\n";
    }
    out << "      },\n";
    // P12-E3：窗口回读。三层事实各占一个字段，谁也别替谁下结论：
    // applied（我们调用成功）→ readback（窗口上真是这个值）→ SF 图层 dump（合成器认账）。
    out << "      \"dataspace_readback\": {\n";
    out << "        \"ok\": " << (stats.hdr.dataspace_readback.ok ? "true" : "false") << ",\n";
    out << "        \"value\": " << stats.hdr.dataspace_readback.value << ",\n";
    out << "        \"reason\": ";
    if (stats.hdr.dataspace_readback.reason.empty()) {
        out << "null\n";
    } else {
        out << "\"" << escape_json_string(stats.hdr.dataspace_readback.reason) << "\"\n";
    }
    out << "      },\n";
    // P12-E4：色调映射状态。`implemented` 恒为 false —— 它是事实，不是功能开关。
    out << "      \"tonemap\": {\n";
    out << "        \"implemented\": " << (stats.hdr.tonemap.implemented ? "true" : "false")
        << ",\n";
    out << "        \"reason\": \"" << escape_json_string(stats.hdr.tonemap.reason) << "\"\n";
    out << "      }\n";
    out << "    },\n";

    out << "    \"verdict\": \"" << color_verdict_name(color.verdict) << "\",\n";
    out << "    \"reason\": ";
    if (color.reason.empty()) {
        out << "null";
    } else {
        out << "\"" << escape_json_string(color.reason) << "\"";
    }
    out << "\n";
    out << "  },\n";

    // P12-F1：seek。判定在这里算（`assess_seek`），统计结构只存事实。
    const core::seek::SeekAssessment seek = core::seek::assess_seek(stats.seek);
    out << "  \"seek\": {\n";
    out << "    \"requests\": " << stats.seek.requests << ",\n";
    auto optional_us = [&out](const char* key, std::int64_t value, bool comma) {
        out << "    \"" << key << "\": ";
        if (value < 0) {
            out << "null";
        } else {
            out << value;
        }
        out << (comma ? "," : "") << "\n";
    };
    optional_us("target_us", stats.seek.target_us, true);
    optional_us("landing_us", stats.seek.landing_us, true);
    optional_us("prepare_us", stats.seek.prepare_us, true);
    optional_us("first_frame_us", stats.seek.first_frame_us, true);
    optional_us("total_us", seek.total_us, true);
    out << "    \"budget_us\": " << core::seek::kDefaultBudgetUs << ",\n";
    out << "    \"pass\": " << (seek.pass ? "true" : "false") << ",\n";
    optional_us("video_first_pts_us", stats.seek.video_first_pts_us, true);
    out << "    \"reason\": ";
    if (seek.reason.empty()) {
        out << "null,\n";
    } else {
        out << "\"" << escape_json_string(seek.reason) << "\",\n";
    }
    // P12-F2：音画两侧。**没有音轨时 `applicable=false`、`pass=false`** ——
    // 没音频的运行无法证明"音画不脱节"，让它通过等于用不存在的证据交差。
    const core::seek::AvSeekAssessment av = core::seek::assess_av_seek(stats.seek);
    out << "    \"av\": {\n";
    out << "      \"applicable\": " << (av.applicable ? "true" : "false") << ",\n";
    out << "      \"audio_present\": " << (stats.seek.audio_present ? "true" : "false")
        << ",\n";
    optional_us("audio_landing_us", stats.seek.audio_landing_us, true);
    // gap 是**带符号**的（音频在前为正、在后为负），不能走 optional_us ——
    // 那个函数把负值写成 null，而 -8000 恰恰是一个有效且重要的读数。
    out << "      \"gap_us\": " << av.audio_gap_us << ",\n";
    out << "      \"limit_us\": " << core::seek::kMaxAvGapUs << ",\n";
    out << "      \"pass\": " << (av.pass ? "true" : "false") << ",\n";
    out << "      \"reason\": ";
    if (av.reason.empty()) {
        out << "null\n";
    } else {
        out << "\"" << escape_json_string(av.reason) << "\"\n";
    }
    out << "    }\n";
    out << "  },\n";


    out << "  \"counters\": {\n";
    out << "    \"samples_submitted\": " << stats.samples_submitted << ",\n";
    out << "    \"samples_rejected\": " << stats.samples_rejected << ",\n";
    out << "    \"bytes_submitted\": " << stats.bytes_submitted << ",\n";
    out << "    \"frames_received\": " << stats.frames_received << ",\n";
    out << "    \"frames_released\": " << stats.frames_released << ",\n";
    out << "    \"frames_rendered\": " << stats.frames_rendered << ",\n";
    out << "    \"frames_dropped_late\": " << stats.frames_dropped_late << ",\n";
    // D1：换面空窗期丢的帧。与"节拍判它太晚"分开记，否则一换面就像节拍崩了。
    out << "    \"frames_discarded_no_surface\": " << stats.frames_discarded_no_surface << "\n";
    out << "  },\n";

    // 流水线段：P12-C6 的全部原料。
    // `mode` 必须写：同一张表里 split 与 merged 的数字混在一起就没法归因。
    out << "  \"pipeline\": {\n";
    out << "    \"mode\": \"" << escape_json_string(stats.pipeline_mode) << "\",\n";
    out << "    \"present\": " << (stats.present_frames ? "true" : "false") << ",\n";
    // C7：谁在等。`cpu_paced` 的抖动看 lateness，`surface_at_time` 的看 schedule_lead，
    // 两条不能混读 —— 前者是"迟了多少"，后者是"提前多久"。
    out << "    \"present_schedule\": \"" << escape_json_string(stats.present_schedule) << "\",\n";
    out << "    \"submit\": {\n";
    render_stage_cost(out, stats.submit_cost, true);
    out << "    \"eos\": {\n";
    render_stage_cost(out, stats.eos_cost, true);
    out << "    \"receive\": {\n";
    render_stage_cost(out, stats.receive_cost, true);
    out << "    \"release\": {\n";
    render_stage_cost(out, stats.release_cost, false);
    out << "  },\n";

    // P12-D1：换面。`rebuilds == 0` 表示这一轮没换过面，其余字段保持 -1，
    // 不写 0 —— 0 会被读成"恢复耗时 0ms"，那是比没有更坏的数字。
    out << "  \"surface_rebuild\": {\n";
    out << "    \"rebuilds\": " << stats.surface_rebuild.rebinds << ",\n";
    out << "    \"gap_ms\": " << stats.surface_rebuild.last_gap_ms << ",\n";
    out << "    \"resume_ms\": " << stats.surface_rebuild.last_resume_ms << ",\n";
    out << "    \"frames_lead\": " << stats.surface_rebuild.last_frames_lead << "\n";
    out << "  },\n";

    // P14-A：三条以前没有尺子的目标。
    //
    // 写法与其它块不同：每一项都自带 `valid`/`reason` 或 `hook_installed`。
    // `calls_steady: 0` 在没有钩子的构建上完全一样，所以"钩子装没装"必须出版 ——
    // P13-T 的 `drift_rebases` 漏写就是同一个病（"门没触发" vs "门没接上"）。
    out << "  \"metrics\": {\n";
    out << "    \"cpu\": {\"valid\": " << (stats.metrics.cpu.valid ? "true" : "false")
        << ", \"reason\": " << "\"" << escape_json_string(stats.metrics.cpu.reason) << "\""
        << ", \"wall_ns\": " << stats.metrics.cpu.wall_ns
        << ", \"thread_cpu_ns\": " << stats.metrics.cpu.thread_cpu_ns
        << ", \"process_cpu_ns\": " << stats.metrics.cpu.process_cpu_ns
        << ", \"thread_ratio\": " << fixed6(stats.metrics.cpu.thread_ratio)
        << ", \"process_ratio\": " << fixed6(stats.metrics.cpu.process_ratio) << "},\n";
    out << "    \"alloc\": {\"hook_installed\": "
        << (stats.metrics.alloc.hook_installed ? "true" : "false")
        << ", \"armed\": " << (stats.metrics.alloc.armed ? "true" : "false")
        << ", \"calls_total\": " << stats.metrics.alloc.calls_total
        << ", \"calls_steady\": " << stats.metrics.alloc.calls_steady
        << ", \"bytes_total\": " << stats.metrics.alloc.bytes_total
        << ", \"bytes_steady\": " << stats.metrics.alloc.bytes_steady << "},\n";
    out << "    \"buffers\": {\"pool_owner\": " << "\"" << escape_json_string(stats.metrics.buffers.pool_owner) << "\""
        << ", \"received\": " << stats.metrics.buffers.received
        << ", \"released\": " << stats.metrics.buffers.released
        << ", \"in_flight\": " << stats.metrics.buffers.in_flight
        << ", \"leaked\": " << stats.metrics.buffers.leaked
        << ", \"reason\": " << "\"" << escape_json_string(stats.metrics.buffers.reason) << "\"" << "}\n";
    out << "  },\n";

    out << "  \"timing\": {\n";
    out << "    \"wall_us\": " << stats.wall_micros << ",\n";
    out << "    \"first_frame_latency_us\": " << stats.first_frame_latency_us << ",\n";
    out << "    \"decode_fps\": " << fixed6(stats.decode_fps) << ",\n";
    out << "    \"speed_vs_realtime\": " << fixed6(stats.speed_vs_realtime) << "\n";
    out << "  },\n";

    // P13-B：G8a —— 冷启动首帧。判定在这里算，与 `assess_cold_start()` 同源。
    const core::seek::ColdStartAssessment cold =
        core::seek::assess_cold_start(stats.cold_start_first_frame_us);
    out << "  \"cold_start\": {\n";
    out << "    \"applicable\": " << (cold.applicable ? "true" : "false") << ",\n";
    out << "    \"first_frame_us\": ";
    if (cold.first_frame_us < 0) {
        out << "null";
    } else {
        out << cold.first_frame_us;
    }
    out << ",\n";
    out << "    \"budget_us\": " << cold.budget_us << ",\n";
    out << "    \"pass\": " << (cold.pass ? "true" : "false") << ",\n";
    out << "    \"reason\": ";
    if (cold.reason.empty()) {
        out << "null\n";
    } else {
        out << "\"" << escape_json_string(cold.reason) << "\"\n";
    }
    out << "  },\n";

    out << "  \"outcome\": {\n";
    out << "    \"completed\": " << (stats.completed ? "true" : "false") << ",\n";
    out << "    \"error\": \"" << escape_json_string(stats.error) << "\"\n";
    out << "  },\n";

    // 节拍段：G6 的全部原料都在这里。`enabled=false` 时只有一格，
    // 不写一堆全 0 的数 —— 那会让"没开节拍"看起来像"节拍完美"。
    const DecodePaceStats& pace = stats.pace;
    out << "  \"pace\": {\n";
    out << "    \"enabled\": " << (pace.enabled ? "true" : "false") << ",\n";
    if (pace.enabled) {
        out << "    \"clock_source\": \"" << escape_json_string(pace.clock_source) << "\",\n";
        out << "    \"frame_duration_ns\": " << pace.frame_duration_ns << ",\n";
        out << "    \"derived_fps\": " << fixed6(pace.derived_fps) << ",\n";
        out << "    \"frames_decided\": " << pace.frames_decided << ",\n";
        out << "    \"presented\": " << pace.presented << ",\n";
        out << "    \"dropped\": " << pace.dropped << ",\n";
        out << "    \"drop_ratio\": " << fixed6(pace.drop_ratio) << ",\n";
        out << "    \"catch_up_events\": " << pace.catch_up_events << ",\n";
        out << "    \"catch_up_frames\": " << pace.catch_up_frames << ",\n";
        out << "    \"over_budget_frames\": " << pace.over_budget_frames << ",\n";
        out << "    \"lateness_ns\": {\n";
        out << "      \"p50\": " << pace.lateness_p50_ns << ",\n";
        out << "      \"p95\": " << pace.lateness_p95_ns << ",\n";
        out << "      \"p99\": " << pace.lateness_p99_ns << ",\n";
        out << "      \"min\": " << pace.lateness_min_ns << ",\n";
        out << "      \"max\": " << pace.lateness_max_ns << "\n";
        out << "    },\n";
        out << "    \"interval_ns\": {\n";
        out << "      \"p50\": " << pace.interval_p50_ns << ",\n";
        out << "      \"p95\": " << pace.interval_p95_ns << ",\n";
        out << "      \"p99\": " << pace.interval_p99_ns << ",\n";
        out << "      \"max\": " << pace.interval_max_ns << "\n";
        out << "    },\n";
        // C8：逐帧时钟分档。三者之和应等于 `frames_decided` —— 不等就是有帧漏记了。
        out << "    \"clock_mix\": {\n";
        out << "      \"primary_frames\": " << pace.clock_primary_frames << ",\n";
        out << "      \"fallback_frames\": " << pace.clock_fallback_frames << ",\n";
        out << "      \"system_frames\": " << pace.clock_system_frames << "\n";
        out << "    },\n";
        // C7：提交提前量。负 = 提前交出去。`spread` 才是判据（每帧是否提前得一样多），
        // 均值大只说明提前得多，不代表更稳。
        out << "    \"schedule_lead_ns\": {\n";
        out << "      \"samples\": " << pace.scheduled_samples << ",\n";
        out << "      \"p01\": " << pace.schedule_lead_p01_ns << ",\n";
        out << "      \"p10\": " << pace.schedule_lead_p10_ns << ",\n";
        out << "      \"p50\": " << pace.schedule_lead_p50_ns << ",\n";
        out << "      \"p90\": " << pace.schedule_lead_p90_ns << ",\n";
        out << "      \"p99\": " << pace.schedule_lead_p99_ns << ",\n";
        out << "      \"min\": " << pace.schedule_lead_min_ns << ",\n";
        out << "      \"max\": " << pace.schedule_lead_max_ns << ",\n";
        out << "      \"spread\": " << pace.schedule_lead_spread_ns << "\n";
        out << "    },\n";
        out << "    \"clock_health\": {\n";
        out << "      \"extrapolated_frames\": " << pace.extrapolated_frames << ",\n";
        out << "      \"free_run_frames\": " << pace.free_run_frames << ",\n";
        out << "      \"jump_rebases\": " << pace.jump_rebases << ",\n";
        out << "      \"monotonic_rejects\": " << pace.monotonic_rejects << ",\n";
        out << "      \"stale_readings\": " << pace.stale_readings << "\n";
        out << "    },\n";
        out << "    \"wait\": {\n";
        out << "      \"count\": " << pace.waited << ",\n";
        out << "      \"total_ns\": " << pace.wait_total_ns << "\n";
        out << "    },\n";
        out << "    \"sampled_at_frames\": " << pace.snapshot_at_frames << "\n";
    } else {
        // 末尾补一个换行占位，保持 JSON 结构一致
        out << "    \"reason\": \"pacing not attached\"\n";
    }
    out << "  }\n";
    out << "}\n";
    return out.str();
}

}  // namespace kivo::video::mediacodec
