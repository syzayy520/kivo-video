#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video::mediacodec {

/// 一条「codec × 分辨率」的帧率能力记录（P13-A）。
///
/// 为什么一个 codec 要占多行：`getSupportedFrameRatesFor(w, h)` 的返回值**与分辨率绑定**。
/// 1080p 能跑 240fps 的组件在 4K 上可能只到 60fps —— 一条记录里塞不下这个关系。
struct FramerateRecord {
    std::string name;
    std::string mime;
    std::int32_t profile = -1;
    std::int32_t width = 0;
    std::int32_t height = 0;
    /// 1 = 该尺寸支持，0 = 不支持，-1 = 平台没说（API 拿不到）
    std::int32_t size_supported = -1;
    /// `getSupportedFrameRatesFor(w,h).upper`；拿不到 -1。
    double max_fps = -1.0;
    /// `achievableFrameRatesFor(w,h).upper`；拿不到 -1。
    double achievable_fps = -1.0;
};

/// 探针结果。`code` 与 P12-A 的能力探针保持同一套语义：
/// 0 成功；1 没有记录；2 输出路径为空；3 全部解析失败；4 写盘失败。
struct FramerateProbeOutcome {
    bool ok = false;
    std::int32_t code = 0;
    std::string message;
    std::size_t parsed_records = 0;
    std::size_t rejected_records = 0;
};

/// 解析一行 TSV（8 字段）。字段顺序见 `MediaFramerateInventory` 的注释。
/// 解析失败返回 false —— 调用方负责计数，不静默丢弃。
[[nodiscard]] bool parse_framerate_record(const std::string& raw, FramerateRecord& out);

/// 在一组记录里找「某 mime + 某尺寸」的最大可达帧率；没有支持该尺寸的记录返回 -1。
///
/// **取 min(声明上限, 吞吐可达)**，不是取声明上限：厂商常声明"4K60 支持"，
/// 但 `achievableFramesPerSecond` 只到 30 —— 真播会掉帧。
/// 这个函数是 verdict 的全部判定逻辑，所以它被单独暴露出来做单测。
[[nodiscard]] double best_fps_for(const std::vector<FramerateRecord>& records,
                                  const std::string& mime, std::int32_t width,
                                  std::int32_t height);

/// 解析全部记录、渲染 JSON、写盘。
///
/// **这一步是"判定"，不是"采集"**：`verdict` 段直接回答
/// "HEVC 4K60 能不能"，而不是把一堆数字丢给读者自己算。
[[nodiscard]] FramerateProbeOutcome probe_framerates(const std::string& output_path,
                                                     const std::vector<std::string>& records);

}  // namespace kivo::video::mediacodec
