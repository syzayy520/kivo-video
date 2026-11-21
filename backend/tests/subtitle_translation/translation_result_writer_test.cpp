#include <cassert>
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/translation_result_writer.hpp"

using namespace kivo::cinema_engine;

/// assert 失败不弹 Debug 对话框（弹窗会卡住 ctest 60s 超时并打扰同机使用）。
/// 失败改写 stderr，ctest --output-on-failure 能完整捕获。
void disable_assert_dialog() {
#ifdef _MSC_VER
    // 三类报告全部静默：_CRT_ASSERT（断言）、_CRT_ERROR（vector 越界等
    // Debug 校验 —— 2026-11-21 两张弹窗截图的嫌疑根因）、_CRT_WARN。
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _set_error_mode(_OUT_TO_STDERR);
    // Debug CRT 的 assert() 失败在报告之后还会调 abort() —— abort 本身在
    // /MDd 下默认弹「abort() has been called」对话框（_CALL_REPORTFAULT），
    // 关掉它让失败直接退进程、由 ctest 捕获（2026-11-21 弹窗截图第二轮）。
    _set_abort_behavior(0, _CALL_REPORTFAULT);
#endif
}

// ============================================================================
// 译文写出（SRT）的契约测试。
//
// 落位路径: backend/tests/subtitle_translation/translation_result_writer_test.cpp
//
// 【为什么单独测它】
//   译文最终要落成 .srt 文件交给播放器的外挂字幕入口
//   （`mpv.command("sub-add", "<path> select")`）。
//   写错了有两个症状，都很难从播放画面反推：
//     - 序号不连续 → 部分播放器直接拒收整份字幕（表现为「字幕不出现」）
//     - 时间戳形状错 → 同上，且不报任何错
//   所以格式细节必须由单测钉住，而不是靠肉眼。
// ============================================================================

namespace {

SubtitleTranslationTimedLine make_line(int64_t start_ms, int64_t end_ms, std::string text) {
    SubtitleTranslationTimedLine line;
    line.start_time_ms = start_ms;
    line.end_time_ms = end_ms;
    line.text = std::move(text);
    return line;
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

}  // namespace

void test_empty_input_writes_nothing() {
    const std::string out = write_translated_srt({});
    assert(out.empty());
    std::cout << "PASS: test_empty_input_writes_nothing" << std::endl;
}

void test_single_entry_shape() {
    const std::string out = write_translated_srt({make_line(1000, 2000, "hello")});
    assert(out.rfind("1\r\n", 0) == 0);
    assert(contains(out, "00:00:01,000 --> 00:00:02,000\r\n"));
    assert(contains(out, "hello\r\n"));
    std::cout << "PASS: test_single_entry_shape" << std::endl;
}

void test_entries_are_separated_by_blank_line() {
    const std::string out = write_translated_srt(
        {make_line(0, 1000, "a"), make_line(1000, 2000, "b")});
    assert(contains(out, "a\r\n\r\n"));
    assert(contains(out, "b\r\n\r\n"));
    std::cout << "PASS: test_entries_are_separated_by_blank_line" << std::endl;
}

void test_numbering_is_sequential() {
    const std::string out = write_translated_srt(
        {make_line(0, 1000, "a"), make_line(1000, 2000, "b"), make_line(2000, 3000, "c")});
    assert(contains(out, "1\r\n"));
    assert(contains(out, "2\r\n"));
    assert(contains(out, "3\r\n"));
    assert(!contains(out, "4\r\n"));
    std::cout << "PASS: test_numbering_is_sequential" << std::endl;
}

/// 翻译失败的行（空文本）必须被跳过，且**序号不能跳号**。
/// 跳号会让部分播放器拒收整份文件 —— 症状是「字幕完全不出现」，没有错误提示。
void test_failed_line_is_skipped_without_gap() {
    const std::string out = write_translated_srt(
        {make_line(0, 1000, "a"), make_line(1000, 2000, ""), make_line(2000, 3000, "c")});
    assert(contains(out, "1\r\n"));
    assert(contains(out, "2\r\n"));
    assert(!contains(out, "3\r\n"));

    // 被跳过的那条（1000ms~2000ms）不出现。
    assert(!contains(out, "00:00:01,000 --> 00:00:02,000"));

    // 第三条接在编号 2 上 —— 序号连续，没有因为跳过而留空。
    assert(contains(out, "2\r\n00:00:02,000 --> 00:00:03,000"));
    std::cout << "PASS: test_failed_line_is_skipped_without_gap" << std::endl;
}

/// 时间倒挂的行（end <= start）不写 —— 这种行写出去只会让播放器丢弃它。
void test_inverted_timing_is_skipped() {
    const std::string out = write_translated_srt(
        {make_line(2000, 1000, "bad"), make_line(1000, 2000, "good")});
    assert(!contains(out, "bad"));
    assert(contains(out, "good"));
    assert(contains(out, "1\r\n"));  // 序号从 1 开始，不留空
    std::cout << "PASS: test_inverted_timing_is_skipped" << std::endl;
}

/// 时间戳形状：小时不封顶、毫秒补足三位、负值钳到 0。
void test_timestamp_formatting() {
    const std::string out = write_translated_srt(
        {make_line(3661234, 3662000, "long"), make_line(-500, 100, "negative")});
    assert(contains(out, "01:01:01,234 --> 01:01:02,000"));
    assert(contains(out, "00:00:00,000 --> 00:00:00,100"));
    std::cout << "PASS: test_timestamp_formatting" << std::endl;
}

/// 毫秒必须补足三位（"5" 要写成 "005"），否则部分播放器解析失败。
void test_milliseconds_are_zero_padded() {
    const std::string out = write_translated_srt({make_line(5, 6, "x")});
    assert(contains(out, "00:00:00,005 --> 00:00:00,006"));
    std::cout << "PASS: test_milliseconds_are_zero_padded" << std::endl;
}

/// 行尾必须是 CRLF。
void test_crlf_line_endings() {
    const std::string out = write_translated_srt({make_line(0, 1000, "a")});
    assert(contains(out, "\r\n"));
    assert(!contains(out, "\n\n\r"));
    // 纯 LF（无 CR）不应出现
    std::size_t index = 0;
    while ((index = out.find('\n', index)) != std::string::npos) {
        assert(index > 0 && out[index - 1] == '\r');
        ++index;
    }
    std::cout << "PASS: test_crlf_line_endings" << std::endl;
}

int main() {
    disable_assert_dialog();
    test_empty_input_writes_nothing();
    test_single_entry_shape();
    test_entries_are_separated_by_blank_line();
    test_numbering_is_sequential();
    test_failed_line_is_skipped_without_gap();
    test_inverted_timing_is_skipped();
    test_timestamp_formatting();
    test_milliseconds_are_zero_padded();
    test_crlf_line_endings();

    std::cout << "All Subtitle Translation Result Writer tests passed." << std::endl;
    return 0;
}
