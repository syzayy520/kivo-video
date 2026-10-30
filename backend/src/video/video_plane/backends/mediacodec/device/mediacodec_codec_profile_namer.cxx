#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_profile_namer.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_profile_level_ids.hpp"

#include <array>
#include <span>

namespace kivo::video::mediacodec {
namespace {

struct NamedValue {
    std::int32_t value;
    std::string_view name;
};

constexpr std::string_view kMimeAvc = "video/avc";
constexpr std::string_view kMimeHevc = "video/hevc";
constexpr std::string_view kMimeVp9 = "video/x-vnd.on2.vp9";
constexpr std::string_view kMimeVp8 = "video/x-vnd.on2.vp8";
constexpr std::string_view kMimeAv1 = "video/av01";
constexpr std::string_view kMimeMpeg4 = "video/mp4v-es";
constexpr std::string_view kMimeMpeg2 = "video/mpeg2";
constexpr std::string_view kMimeH263 = "video/3gpp";

constexpr std::array<NamedValue, 9> kAvcProfiles{{
    {kAvcProfileBaseline, "avc-baseline"},
    {kAvcProfileMain, "avc-main"},
    {kAvcProfileExtended, "avc-extended"},
    {kAvcProfileHigh, "avc-high"},
    {kAvcProfileHigh10, "avc-high10"},
    {kAvcProfileHigh422, "avc-high422"},
    {kAvcProfileHigh444, "avc-high444"},
    {kAvcProfileConstrainedBaseline, "avc-constrained-baseline"},
    {kAvcProfileConstrainedHigh, "avc-constrained-high"},
}};

constexpr std::array<NamedValue, 5> kHevcProfiles{{
    {kHevcProfileMain, "hevc-main"},
    {kHevcProfileMain10, "hevc-main10"},
    {kHevcProfileMainStill, "hevc-mainstill"},
    {kHevcProfileMain10Hdr10, "hevc-main10-hdr10"},
    {kHevcProfileMain10Hdr10Plus, "hevc-main10-hdr10plus"},
}};

constexpr std::array<NamedValue, 4> kVp9Profiles{{
    {kVp9Profile0, "vp9-profile0"},
    {kVp9Profile1, "vp9-profile1"},
    {kVp9Profile2, "vp9-profile2"},
    {kVp9Profile3, "vp9-profile3"},
}};

constexpr std::array<NamedValue, 4> kAv1Profiles{{
    {kAv1ProfileMain8, "av1-main8"},
    {kAv1ProfileMain10, "av1-main10"},
    {kAv1ProfileMain10Hdr10, "av1-main10-hdr10"},
    {kAv1ProfileMain10Hdr10Plus, "av1-main10-hdr10plus"},
}};

constexpr std::array<NamedValue, 20> kAvcLevels{{
    {kAvcLevel1, "1"},      {kAvcLevel1b, "1b"},     {kAvcLevel11, "1.1"},
    {kAvcLevel12, "1.2"},   {kAvcLevel13, "1.3"},    {kAvcLevel2, "2"},
    {kAvcLevel21, "2.1"},   {kAvcLevel22, "2.2"},    {kAvcLevel3, "3"},
    {kAvcLevel31, "3.1"},   {kAvcLevel32, "3.2"},    {kAvcLevel4, "4"},
    {kAvcLevel41, "4.1"},   {kAvcLevel42, "4.2"},    {kAvcLevel5, "5"},
    {kAvcLevel51, "5.1"},   {kAvcLevel52, "5.2"},    {kAvcLevel6, "6"},
    {kAvcLevel61, "6.1"},   {kAvcLevel62, "6.2"},
}};

constexpr std::array<NamedValue, 26> kHevcLevels{{
    {kHevcMainTierLevel1, "main-tier-1"},   {kHevcHighTierLevel1, "high-tier-1"},
    {kHevcMainTierLevel2, "main-tier-2"},   {kHevcHighTierLevel2, "high-tier-2"},
    {kHevcMainTierLevel21, "main-tier-2.1"}, {kHevcHighTierLevel21, "high-tier-2.1"},
    {kHevcMainTierLevel3, "main-tier-3"},   {kHevcHighTierLevel3, "high-tier-3"},
    {kHevcMainTierLevel31, "main-tier-3.1"}, {kHevcHighTierLevel31, "high-tier-3.1"},
    {kHevcMainTierLevel4, "main-tier-4"},   {kHevcHighTierLevel4, "high-tier-4"},
    {kHevcMainTierLevel41, "main-tier-4.1"}, {kHevcHighTierLevel41, "high-tier-4.1"},
    {kHevcMainTierLevel5, "main-tier-5"},   {kHevcHighTierLevel5, "high-tier-5"},
    {kHevcMainTierLevel51, "main-tier-5.1"}, {kHevcHighTierLevel51, "high-tier-5.1"},
    {kHevcMainTierLevel52, "main-tier-5.2"}, {kHevcHighTierLevel52, "high-tier-5.2"},
    {kHevcMainTierLevel6, "main-tier-6"},   {kHevcHighTierLevel6, "high-tier-6"},
    {kHevcMainTierLevel61, "main-tier-6.1"}, {kHevcHighTierLevel61, "high-tier-6.1"},
    {kHevcMainTierLevel62, "main-tier-6.2"}, {kHevcHighTierLevel62, "high-tier-6.2"},
}};

constexpr std::array<NamedValue, 14> kVp9Levels{{
    {kVp9Level1, "1"},    {kVp9Level11, "1.1"},  {kVp9Level2, "2"},
    {kVp9Level21, "2.1"}, {kVp9Level3, "3"},     {kVp9Level31, "3.1"},
    {kVp9Level4, "4"},    {kVp9Level41, "4.1"},  {kVp9Level5, "5"},
    {kVp9Level51, "5.1"}, {kVp9Level52, "5.2"},  {kVp9Level6, "6"},
    {kVp9Level61, "6.1"}, {kVp9Level62, "6.2"},
}};

constexpr std::array<NamedValue, 20> kAv1Levels{{
    {kAv1Level2, "2"},     {kAv1Level21, "2.1"},   {kAv1Level22, "2.2"},
    {kAv1Level23, "2.3"},  {kAv1Level3, "3"},      {kAv1Level31, "3.1"},
    {kAv1Level32, "3.2"},  {kAv1Level33, "3.3"},   {kAv1Level4, "4"},
    {kAv1Level41, "4.1"},  {kAv1Level42, "4.2"},   {kAv1Level43, "4.3"},
    {kAv1Level5, "5"},     {kAv1Level51, "5.1"},   {kAv1Level52, "5.2"},
    {kAv1Level53, "5.3"},  {kAv1Level6, "6"},      {kAv1Level61, "6.1"},
    {kAv1Level62, "6.2"},  {kAv1Level63, "6.3"},
}};

[[nodiscard]] const NamedValue* find_named(std::span<const NamedValue> table,
                                           std::int32_t value) {
    for (const NamedValue& entry : table) {
        if (entry.value == value) {
            return &entry;
        }
    }
    return nullptr;
}

[[nodiscard]] std::span<const NamedValue> profile_table_for(std::string_view mime) {
    if (mime == kMimeAvc) {
        return kAvcProfiles;
    }
    if (mime == kMimeHevc) {
        return kHevcProfiles;
    }
    if (mime == kMimeVp9) {
        return kVp9Profiles;
    }
    if (mime == kMimeAv1) {
        return kAv1Profiles;
    }
    return {};
}

[[nodiscard]] std::span<const NamedValue> level_table_for(std::string_view mime) {
    if (mime == kMimeAvc) {
        return kAvcLevels;
    }
    if (mime == kMimeHevc) {
        return kHevcLevels;
    }
    if (mime == kMimeVp9) {
        return kVp9Levels;
    }
    if (mime == kMimeAv1) {
        return kAv1Levels;
    }
    return {};
}

[[nodiscard]] std::string_view codec_prefix_for(std::string_view mime) {
    if (mime == kMimeAvc) {
        return "avc";
    }
    if (mime == kMimeHevc) {
        return "hevc";
    }
    if (mime == kMimeVp9) {
        return "vp9";
    }
    if (mime == kMimeVp8) {
        return "vp8";
    }
    if (mime == kMimeAv1) {
        return "av1";
    }
    if (mime == kMimeMpeg4) {
        return "mpeg4";
    }
    if (mime == kMimeMpeg2) {
        return "mpeg2";
    }
    if (mime == kMimeH263) {
        return "h263";
    }
    return "unknown";
}

[[nodiscard]] std::string unknown_with_raw(std::string_view prefix, std::int32_t raw) {
    return std::string(prefix) + "-unknown(" + std::to_string(raw) + ")";
}

}  // namespace

std::string describe_profile(std::string_view mime, std::int32_t profile) {
    if (profile < 0) {
        return unknown_with_raw(codec_prefix_for(mime), profile);
    }
    if (const NamedValue* hit = find_named(profile_table_for(mime), profile); hit != nullptr) {
        return std::string(hit->name);
    }
    return unknown_with_raw(codec_prefix_for(mime), profile);
}

std::string describe_level(std::string_view mime, std::int32_t level) {
    if (level < 0) {
        return unknown_with_raw(codec_prefix_for(mime), level);
    }
    if (const NamedValue* hit = find_named(level_table_for(mime), level); hit != nullptr) {
        return std::string(codec_prefix_for(mime)) + "-" + std::string(hit->name);
    }
    return unknown_with_raw(codec_prefix_for(mime), level);
}

}  // namespace kivo::video::mediacodec
