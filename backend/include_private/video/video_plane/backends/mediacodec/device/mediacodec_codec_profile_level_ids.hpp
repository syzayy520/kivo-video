#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

// Android `MediaCodecInfo.CodecProfileLevel` 的常量快照。
//
// 为什么固化而不 include：这些常量定义在 Java 侧的 CodecProfileLevel 里，
// NDK 头文件不导出。宿主侧（桌面 CI）编译 mediacodec 记录层时也没有
// <media/...> 可用，因此必须在此处固化。
//
// 唯一真相来源：AOSP frameworks/base/media/java/android/media/MediaCodecInfo.java
// 改动前必须对照该文件，不许凭印象增删。
//
// 本头文件只放数据，不放任何逻辑。

// ---- H.264 / AVC ----------------------------------------------------------
inline constexpr std::int32_t kAvcProfileBaseline = 0x01;
inline constexpr std::int32_t kAvcProfileMain = 0x02;
inline constexpr std::int32_t kAvcProfileExtended = 0x04;
inline constexpr std::int32_t kAvcProfileHigh = 0x08;
inline constexpr std::int32_t kAvcProfileHigh10 = 0x10;
inline constexpr std::int32_t kAvcProfileHigh422 = 0x20;
inline constexpr std::int32_t kAvcProfileHigh444 = 0x40;
inline constexpr std::int32_t kAvcProfileConstrainedBaseline = 0x10000;
inline constexpr std::int32_t kAvcProfileConstrainedHigh = 0x80000;

// ---- H.265 / HEVC ---------------------------------------------------------
inline constexpr std::int32_t kHevcProfileMain = 0x01;
inline constexpr std::int32_t kHevcProfileMain10 = 0x02;
inline constexpr std::int32_t kHevcProfileMainStill = 0x04;
inline constexpr std::int32_t kHevcProfileMain10Hdr10 = 0x1000;
inline constexpr std::int32_t kHevcProfileMain10Hdr10Plus = 0x2000;

// ---- VP9 ------------------------------------------------------------------
inline constexpr std::int32_t kVp9Profile0 = 0x01;
inline constexpr std::int32_t kVp9Profile1 = 0x02;
inline constexpr std::int32_t kVp9Profile2 = 0x04;
inline constexpr std::int32_t kVp9Profile3 = 0x08;

// ---- AV1 ------------------------------------------------------------------
inline constexpr std::int32_t kAv1ProfileMain8 = 0x01;
inline constexpr std::int32_t kAv1ProfileMain10 = 0x02;
inline constexpr std::int32_t kAv1ProfileMain10Hdr10 = 0x1000;
inline constexpr std::int32_t kAv1ProfileMain10Hdr10Plus = 0x2000;

// ---- level ---------------------------------------------------------------
// level 是按 mime 各自编码的位标量，同一数值在不同 mime 下含义不同，
// 因此查询时必须带 mime，不可跨 mime 复用。

inline constexpr std::int32_t kAvcLevel1 = 0x01;
inline constexpr std::int32_t kAvcLevel1b = 0x02;
inline constexpr std::int32_t kAvcLevel11 = 0x04;
inline constexpr std::int32_t kAvcLevel12 = 0x08;
inline constexpr std::int32_t kAvcLevel13 = 0x10;
inline constexpr std::int32_t kAvcLevel2 = 0x20;
inline constexpr std::int32_t kAvcLevel21 = 0x40;
inline constexpr std::int32_t kAvcLevel22 = 0x80;
inline constexpr std::int32_t kAvcLevel3 = 0x100;
inline constexpr std::int32_t kAvcLevel31 = 0x200;
inline constexpr std::int32_t kAvcLevel32 = 0x400;
inline constexpr std::int32_t kAvcLevel4 = 0x800;
inline constexpr std::int32_t kAvcLevel41 = 0x1000;
inline constexpr std::int32_t kAvcLevel42 = 0x2000;
inline constexpr std::int32_t kAvcLevel5 = 0x4000;
inline constexpr std::int32_t kAvcLevel51 = 0x8000;
inline constexpr std::int32_t kAvcLevel52 = 0x10000;
inline constexpr std::int32_t kAvcLevel6 = 0x40000;
inline constexpr std::int32_t kAvcLevel61 = 0x80000;
inline constexpr std::int32_t kAvcLevel62 = 0x100000;

inline constexpr std::int32_t kHevcMainTierLevel1 = 0x01;
inline constexpr std::int32_t kHevcHighTierLevel1 = 0x02;
inline constexpr std::int32_t kHevcMainTierLevel2 = 0x04;
inline constexpr std::int32_t kHevcHighTierLevel2 = 0x08;
inline constexpr std::int32_t kHevcMainTierLevel21 = 0x10;
inline constexpr std::int32_t kHevcHighTierLevel21 = 0x20;
inline constexpr std::int32_t kHevcMainTierLevel3 = 0x40;
inline constexpr std::int32_t kHevcHighTierLevel3 = 0x80;
inline constexpr std::int32_t kHevcMainTierLevel31 = 0x100;
inline constexpr std::int32_t kHevcHighTierLevel31 = 0x200;
inline constexpr std::int32_t kHevcMainTierLevel4 = 0x400;
inline constexpr std::int32_t kHevcHighTierLevel4 = 0x800;
inline constexpr std::int32_t kHevcMainTierLevel41 = 0x1000;
inline constexpr std::int32_t kHevcHighTierLevel41 = 0x2000;
inline constexpr std::int32_t kHevcMainTierLevel5 = 0x4000;
inline constexpr std::int32_t kHevcHighTierLevel5 = 0x8000;
inline constexpr std::int32_t kHevcMainTierLevel51 = 0x10000;
inline constexpr std::int32_t kHevcHighTierLevel51 = 0x20000;
inline constexpr std::int32_t kHevcMainTierLevel52 = 0x40000;
inline constexpr std::int32_t kHevcHighTierLevel52 = 0x80000;
inline constexpr std::int32_t kHevcMainTierLevel6 = 0x100000;
inline constexpr std::int32_t kHevcHighTierLevel6 = 0x200000;
inline constexpr std::int32_t kHevcMainTierLevel61 = 0x400000;
inline constexpr std::int32_t kHevcHighTierLevel61 = 0x800000;
inline constexpr std::int32_t kHevcMainTierLevel62 = 0x1000000;
inline constexpr std::int32_t kHevcHighTierLevel62 = 0x2000000;

inline constexpr std::int32_t kVp9Level1 = 0x01;
inline constexpr std::int32_t kVp9Level11 = 0x02;
inline constexpr std::int32_t kVp9Level2 = 0x04;
inline constexpr std::int32_t kVp9Level21 = 0x08;
inline constexpr std::int32_t kVp9Level3 = 0x10;
inline constexpr std::int32_t kVp9Level31 = 0x20;
inline constexpr std::int32_t kVp9Level4 = 0x40;
inline constexpr std::int32_t kVp9Level41 = 0x80;
inline constexpr std::int32_t kVp9Level5 = 0x100;
inline constexpr std::int32_t kVp9Level51 = 0x200;
inline constexpr std::int32_t kVp9Level52 = 0x400;
inline constexpr std::int32_t kVp9Level6 = 0x800;
inline constexpr std::int32_t kVp9Level61 = 0x1000;
inline constexpr std::int32_t kVp9Level62 = 0x2000;

inline constexpr std::int32_t kAv1Level2 = 0x01;
inline constexpr std::int32_t kAv1Level21 = 0x02;
inline constexpr std::int32_t kAv1Level22 = 0x04;
inline constexpr std::int32_t kAv1Level23 = 0x08;
inline constexpr std::int32_t kAv1Level3 = 0x10;
inline constexpr std::int32_t kAv1Level31 = 0x20;
inline constexpr std::int32_t kAv1Level32 = 0x40;
inline constexpr std::int32_t kAv1Level33 = 0x80;
inline constexpr std::int32_t kAv1Level4 = 0x100;
inline constexpr std::int32_t kAv1Level41 = 0x200;
inline constexpr std::int32_t kAv1Level42 = 0x400;
inline constexpr std::int32_t kAv1Level43 = 0x800;
inline constexpr std::int32_t kAv1Level5 = 0x1000;
inline constexpr std::int32_t kAv1Level51 = 0x2000;
inline constexpr std::int32_t kAv1Level52 = 0x4000;
inline constexpr std::int32_t kAv1Level53 = 0x8000;
inline constexpr std::int32_t kAv1Level6 = 0x10000;
inline constexpr std::int32_t kAv1Level61 = 0x20000;
inline constexpr std::int32_t kAv1Level62 = 0x40000;
inline constexpr std::int32_t kAv1Level63 = 0x80000;

}  // namespace kivo::video::mediacodec
