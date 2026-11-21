// ============================================================================
// Cue 装配器测试。契约见 subtitle_translation_cue_assembler.hpp。
//
// 落位路径: backend/tests/subtitle_translation/cue_assembler_test.cpp
//
// 覆盖（设计 v3.2 §4 + 评审 E31 畸形清单）：
//   正常装配 / cue 内折行合并（时间轴不变）/ 对话拆分 / 标签剥离 /
//   BOM 与 CRLF / 序号可选与乱序 / 零时长与倒序 / 空 cue / 非标时间戳 /
//   UTF-16 漏网拒收 / 语言检测三层（英中假名 Other）/ contains_chinese /
//   CPS 超限 / 时间轴不变性
// ============================================================================

#include <cassert>
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>
#include <string>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

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

namespace {

std::vector<SubtitleTranslationCueIssue> issues;

const SubtitleTranslationCueAssembly& assemble(const std::string& srt) {
    static SubtitleTranslationCueAssembly cached;
    issues.clear();
    cached = assemble_subtitle_translation_cues(srt, &issues);
    return cached;
}

int issue_count(const SubtitleTranslationCueAssembly& a, const char* category) {
    (void)a;  // issues 走 out_issues 出参通道（契约如此），不在 assembly 内部
    int count = 0;
    for (const auto& item : issues) {
        if (item.category == category) { ++count; }
    }
    return count;
}

}  // namespace

/// 正常装配：多行折行合并成单行，时间轴原样保留（DEC-07 v2 时间轴不变性）。
void test_assemble_normal() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:03,500\n"
        "I thought\n"
        "you were leaving.\n"
        "\n"
        "2\n"
        "00:00:04,000 --> 00:00:05,000\n"
        "Wait for me!\n";
    const auto& a = assemble(srt);
    if (a.cues.size() != 2) {
        std::cout << "DEBUG cues=" << a.cues.size();
        for (const auto& c : a.cues) {
            std::cout << " [" << c.start_time_ms << "-" << c.end_time_ms << " '" << c.text << "']";
        }
        for (const auto& i : issues) {
            std::cout << " issue{" << i.source_index << "," << i.category << "," << i.reason << "}";
        }
        std::cout << std::endl;
    }
    assert(a.cues.size() == 2);
    assert(a.cues[0].start_time_ms == 1000);
    assert(a.cues[0].end_time_ms == 3500);
    // 拉丁折行合并补空格。
    assert(a.cues[0].text == "I thought you were leaving.");
    assert(a.cues[1].text == "Wait for me!");
    assert(a.cues[0].dialogue_units.empty());
    assert(a.issues.empty());
    std::cout << "PASS: test_assemble_normal" << std::endl;
}

/// 对话式 cue：每行 "- " 开头 → 拆独立单元，时间轴不变。
void test_dialogue_split() {
    const std::string srt =
        "1\n"
        "00:00:10,000 --> 00:00:13,000\n"
        "- Hello.\n"
        "- How are you?\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 1);
    assert(a.cues[0].dialogue_units.size() == 2);
    assert(a.cues[0].dialogue_units[0] == "Hello.");
    assert(a.cues[0].dialogue_units[1] == "How are you?");
    std::cout << "PASS: test_dialogue_split" << std::endl;
}

/// 标签剥离：{\an8} 大括号与 <i> 内联标签都不进译文。
void test_tags_stripped() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:02,000\n"
        "{\\an8}<i>Get down!</i>\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 1);
    assert(a.cues[0].text == "Get down!");
    std::cout << "PASS: test_tags_stripped" << std::endl;
}

/// BOM 与 CRLF：编码/换行差异不影响装配。
void test_bom_crlf() {
    const std::string srt =
        "\xef\xbb\xbf" "1\r\n"    // BOM 独立写：\xbf 后紧跟十六进制字符会被吞（坑⑨）
        "00:00:01,000 --> 00:00:02,000\r\n"
        "Hello.\r\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 1);
    assert(a.cues[0].text == "Hello.");
    assert(a.issues.empty());
    std::cout << "PASS: test_bom_crlf" << std::endl;
}

/// 序号可选、乱序不阻断（评审 E31）。
void test_sequence_optional() {
    const std::string srt =
        "00:00:01,000 --> 00:00:02,000\n"
        "No sequence.\n"
        "\n"
        "9\n"
        "00:00:03,000 --> 00:00:04,000\n"
        "Out of order.\n"
        "\n"
        "9\n"
        "00:00:05,000 --> 00:00:06,000\n"
        "Duplicated.\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 3);
    assert(a.cues[2].text == "Duplicated.");
    std::cout << "PASS: test_sequence_optional" << std::endl;
}

/// 零时长与开始>结束：保留 + 上报（timestamp 类别）。
void test_zero_and_inverted_duration() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:01,000\n"
        "Zero.\n"
        "\n"
        "2\n"
        "00:00:05,000 --> 00:00:04,000\n"
        "Inverted.\n";
    const auto& a = assemble(srt);
    if (issue_count(a, "timestamp") != 2 || a.cues.size() != 2) {
        std::cout << "DEBUG cues=" << a.cues.size();
        for (const auto& c : a.cues) {
            std::cout << " [" << c.start_time_ms << "-" << c.end_time_ms << "]";
        }
        for (const auto& i : issues) {
            std::cout << " issue{" << i.source_index << "," << i.category << "}";
        }
        std::cout << std::endl;
    }
    assert(a.cues.size() == 2);   // 保留
    assert(issue_count(a, "timestamp") == 2);
    std::cout << "PASS: test_zero_and_inverted_duration" << std::endl;
}

/// 空 cue：跳过 + 上报。
void test_empty_cue_skipped() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:02,000\n"
        "\n"
        "2\n"
        "00:00:03,000 --> 00:00:04,000\n"
        "Real.\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 1);
    assert(a.cues[0].text == "Real.");
    assert(issue_count(a, "empty") == 1);
    std::cout << "PASS: test_empty_cue_skipped" << std::endl;
}

/// 非标时间戳：MM:SS,mmm 与点分隔毫秒都收。
void test_lenient_timestamp() {
    const std::string srt =
        "1\n"
        "01:05,500 --> 01:07.250\n"
        "Lenient.\n";
    const auto& a = assemble(srt);
    if (a.cues.empty() || a.cues[0].end_time_ms != 67250) {
        std::cout << "DEBUG cues=" << a.cues.size();
        for (const auto& c : a.cues) {
            std::cout << " [" << c.start_time_ms << "-" << c.end_time_ms << " '" << c.text << "']";
        }
        for (const auto& i : issues) {
            std::cout << " issue{" << i.source_index << "," << i.category << "," << i.reason << "}";
        }
        std::cout << std::endl;
    }
    assert(a.cues.size() == 1);
    assert(a.cues[0].start_time_ms == 65500);
    assert(a.cues[0].end_time_ms == 67250);
    std::cout << "PASS: test_lenient_timestamp" << std::endl;
}

/// UTF-16 漏网（NUL 字节）：整批拒收。
void test_utf16_rejected() {
    const std::string srt = std::string("1\x00\x00:00", 8);
    const auto& a = assemble(srt);
    assert(a.cues.empty());
    assert(issue_count(a, "encoding") == 1);
    std::cout << "PASS: test_utf16_rejected" << std::endl;
}

/// 语言检测三层：英（高频词）/中（表意占比）/假名优先/数字 Other。
void test_language_detection() {
    assert(detect_subtitle_cue_language("I cannot hear you clearly.") ==
           SubtitleCueLanguage::English);
    assert(detect_subtitle_cue_language("\xe4\xbd\xa0\xe5\xa5\xbd") ==
           SubtitleCueLanguage::Chinese);
    // 假名优先：含假名的日文不算中文（G5）。
    assert(detect_subtitle_cue_language(
               "\xe3\x81\x82\xe3\x81\xaa\xe3\x81\x9f\xe3\x81\xae\xe4\xba\xba") ==
           SubtitleCueLanguage::Japanese);
    assert(detect_subtitle_cue_language("...") == SubtitleCueLanguage::Other);
    std::cout << "PASS: test_language_detection" << std::endl;
}

/// 文件级 contains_chinese：中文 cue 占比 ≥30% 判已含中文。
void test_contains_chinese_ratio() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:02,000\n"
        "\xe4\xbd\xa0\xe5\xa5\xbd\n"   // 你好
        "\n"
        "2\n"
        "00:00:03,000 --> 00:00:04,000\n"
        "\xe8\xb0\xa2\xe8\xb0\xa2\n"   // 谢谢
        "\n"
        "3\n"
        "00:00:05,000 --> 00:00:06,000\n"
        "I will find you.\n"
        "\n"
        "4\n"
        "00:00:07,000 --> 00:00:08,000\n"
        "Stop right there.\n";
    const auto& a = assemble(srt);
    assert(a.chinese_cue_count == 2);
    assert(a.english_cue_count == 2);
    assert(a.contains_chinese());
    std::cout << "PASS: test_contains_chinese_ratio" << std::endl;
}

/// CPS 超限：长译文短时长 → cps 类 issue（占位阈值 25，DEC-17）。
void test_cps_issue() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:01,300\n"   // 0.3 秒
        "\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\x80\xe6\x9d\xa1\xe8\xbf\x9c\xe8\xb6\x85"
        "\xe9\x98\x85\xe8\xaf\xbb\xe9\x80\x9f\xe5\xba\xa6\xe7\x9a\x84\xe9\x95\xbf"
        "\xe5\xad\x97\xe5\xb9\x95\xe5\x86\x85\xe5\xae\xb9\xe7\x94\xa8\xe4\xbe\x8b\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 1);
    assert(issue_count(a, "cps") == 1);
    std::cout << "PASS: test_cps_issue" << std::endl;
}

/// cue 内部的纯数字文本行（年份/计数）不得被 SRT 序号判定吞掉
/// （集成测试 test_full_chain_mixed_source 的 "1945" 实测抓出，2026-11-21）。
void test_numeric_text_line_inside_cue() {
    const std::string srt =
        "1\n"
        "00:00:05,000 --> 00:00:06,000\n"
        "1945\n";
    const auto& a = assemble(srt);
    assert(a.cues.size() == 1);
    assert(a.cues[0].text == "1945");
    assert(a.cues[0].start_time_ms == 5000);
    assert(a.issues.empty());
    std::cout << "PASS: test_numeric_text_line_inside_cue" << std::endl;
}

int main() {
    disable_assert_dialog();
    test_assemble_normal();
    test_dialogue_split();
    test_tags_stripped();
    test_bom_crlf();
    test_sequence_optional();
    test_zero_and_inverted_duration();
    test_empty_cue_skipped();
    test_lenient_timestamp();
    test_utf16_rejected();
    test_language_detection();
    test_contains_chinese_ratio();
    test_cps_issue();
    test_numeric_text_line_inside_cue();

    std::cout << "All Cue Assembler tests passed." << std::endl;
    return 0;
}