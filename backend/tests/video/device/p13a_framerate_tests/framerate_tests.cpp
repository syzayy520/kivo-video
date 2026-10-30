// framerate_tests.cpp — P13-A：帧率能力解析与判定的单测
//
// 这一层不碰 Android：`parse_framerate_record` 与 `best_fps_for` 都是纯函数。
// 之所以必须能在桌面上测，是因为判决逻辑一旦错了，整张"4K60 能不能"的结论就是错的，
// 而那个结论会直接决定造什么规格的片源 —— 错一次白测一轮。
#include <cstdio>
#include <string>
#include <vector>

#include "video/video_plane/backends/mediacodec/device/mediacodec_framerate_capability.hpp"

namespace {

namespace mediacodec = kivo::video::mediacodec;

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* expr, const char* test, int line) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL %s:%d  %s\n", test, line, expr);
    }
}

#define CHECK(expr) check((expr), #expr, __func__, __LINE__)

/// 造一行 TSV，字段顺序与 `MediaFramerateInventory` 一致。
std::string row(const char* name, const char* mime, int profile, int w, int h, int ok,
                double maxFps, double achFps) {
    return std::string(name) + "\t" + mime + "\t" + std::to_string(profile) + "\t" +
           std::to_string(w) + "\t" + std::to_string(h) + "\t" + std::to_string(ok) + "\t" +
           std::to_string(maxFps) + "\t" + std::to_string(achFps);
}

mediacodec::FramerateRecord make(const char* mime, int w, int h, int ok, double maxFps,
                                 double achFps) {
    mediacodec::FramerateRecord r;
    r.name = "c";
    r.mime = mime;
    r.width = w;
    r.height = h;
    r.size_supported = ok;
    r.max_fps = maxFps;
    r.achievable_fps = achFps;
    return r;
}

// ---- P13A-1 正常解析 ----------------------------------------------------
void t1_parse_ok() {
    mediacodec::FramerateRecord out;
    const bool ok = mediacodec::parse_framerate_record(
        row("OMX.qcom.video.decoder.hevc", "video/hevc", 2, 3840, 2160, 1, 60.0, 30.0), out);
    CHECK(ok);
    CHECK(out.name == "OMX.qcom.video.decoder.hevc");
    CHECK(out.mime == "video/hevc");
    CHECK(out.profile == 2);
    CHECK(out.width == 3840);
    CHECK(out.height == 2160);
    CHECK(out.size_supported == 1);
    CHECK(out.max_fps > 59.9 && out.max_fps < 60.1);
    CHECK(out.achievable_fps > 29.9 && out.achievable_fps < 30.1);
}

// ---- P13A-2 字段数不对就拒，不猜 ---------------------------------------
void t2_reject_wrong_field_count() {
    mediacodec::FramerateRecord out;
    CHECK(!mediacodec::parse_framerate_record("only\tthree\tfields", out));
    CHECK(!mediacodec::parse_framerate_record("", out));
}

// ---- P13A-3 数值非法就拒 -----------------------------------------------
void t3_reject_bad_number() {
    mediacodec::FramerateRecord out;
    // 第 4 个字段（width）是 abc
    CHECK(!mediacodec::parse_framerate_record("n\tvideo/hevc\t2\tabc\t2160\t1\t60.0\t30.0", out));
    // 帧率是 nan / inf —— 不能当成"无穷快"
    CHECK(!mediacodec::parse_framerate_record("n\tvideo/hevc\t2\t3840\t2160\t1\tnan\t30.0", out));
    CHECK(!mediacodec::parse_framerate_record("n\tvideo/hevc\t2\t3840\t2160\t1\tinf\t30.0", out));
}

// ---- P13A-4 名字为空就拒（占位记录会污染统计）--------------------------
void t4_reject_empty_name() {
    mediacodec::FramerateRecord out;
    CHECK(!mediacodec::parse_framerate_record("\tvideo/hevc\t2\t3840\t2160\t1\t60.0\t30.0", out));
}

// ---- P13A-5 取 min(声明, 可达) —— 这是本卡最重要的一条 ----------------
void t5_verdict_takes_min() {
    std::vector<mediacodec::FramerateRecord> records{
        make("video/hevc", 3840, 2160, 1, 60.0, 30.0),
    };
    // 声明 60、可达 30 ⇒ 判 30。若取声明值，会得到一个"能跑 4K60"的假结论。
    CHECK(mediacodec::best_fps_for(records, "video/hevc", 3840, 2160) == 30.0);
}

// ---- P13A-6 可达缺失时退回声明值 ---------------------------------------
void t6_fallback_to_declared() {
    std::vector<mediacodec::FramerateRecord> records{
        make("video/hevc", 3840, 2160, 1, 45.0, -1.0),
    };
    CHECK(mediacodec::best_fps_for(records, "video/hevc", 3840, 2160) == 45.0);
}

// ---- P13A-7 不支持该尺寸 ⇒ -1（不是 0）--------------------------------
void t7_unsupported_size_is_negative() {
    std::vector<mediacodec::FramerateRecord> records{
        make("video/hevc", 3840, 2160, 0, 60.0, 60.0),
    };
    // -1 是"没说/不支持"，与 0 必须分开：0 会被当成"支持但跑 0 帧"。
    CHECK(mediacodec::best_fps_for(records, "video/hevc", 3840, 2160) == -1.0);
}

// ---- P13A-8 多 profile 取最大（main10 比 main 强时不取第一个）---------
void t8_multiple_profiles_take_best() {
    std::vector<mediacodec::FramerateRecord> records{
        make("video/hevc", 3840, 2160, 1, 30.0, 30.0),   // main
        make("video/hevc", 3840, 2160, 1, 60.0, 60.0),   // main10
    };
    CHECK(mediacodec::best_fps_for(records, "video/hevc", 3840, 2160) == 60.0);
}

// ---- P13A-9 mime 与尺寸都要匹配 ---------------------------------------
void t9_requires_mime_and_size_match() {
    std::vector<mediacodec::FramerateRecord> records{
        make("video/avc", 3840, 2160, 1, 60.0, 60.0),
    };
    CHECK(mediacodec::best_fps_for(records, "video/hevc", 3840, 2160) == -1.0);
    CHECK(mediacodec::best_fps_for(records, "video/avc", 1920, 1080) == -1.0);
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"P13A-1 正常解析", t1_parse_ok},
    {"P13A-2 字段数不对就拒", t2_reject_wrong_field_count},
    {"P13A-3 数值非法就拒", t3_reject_bad_number},
    {"P13A-4 空名字就拒", t4_reject_empty_name},
    {"P13A-5 取 min(声明,可达)", t5_verdict_takes_min},
    {"P13A-6 可达缺失退回声明", t6_fallback_to_declared},
    {"P13A-7 不支持尺寸返回 -1", t7_unsupported_size_is_negative},
    {"P13A-8 多 profile 取最大", t8_multiple_profiles_take_best},
    {"P13A-9 mime 与尺寸都要匹配", t9_requires_mime_and_size_match},
};

}  // namespace

int main() {
    std::printf("=== P13-A framerate capability tests ===\n");
    for (const TestCase& test : kCases) {
        const int before = g_failures;
        test.run();
        std::printf("%s %s\n", g_failures == before ? "ok  " : "FAIL", test.name);
    }
    std::printf("%d cases, %d assertions, %d failures\n",
                static_cast<int>(sizeof(kCases) / sizeof(kCases[0])), g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
