// submit_sequencer_tests.cpp — P12-C6 tests: 送包时序状态机
//
// 这组测试守的是两条"踩过就丢数据"的规矩（见 19-p12c6-design.md §十）：
//   ① 输入满（送失败）之后不许读下一包 —— 读一次读指针就前进，那一包永久丢失；
//   ② EOS 只发一次，且 pts 取已送出包的最大值 —— 用"最后一包"会在 B 帧重排时倒退。
//
// 并发本身在桌面上测不了（没有 MediaCodec），所以只测有分支的那部分；
// 线程间的事由真机计数器闭合来守（frames_received == frames_released == 轨道总帧数）。
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "video/video_plane/core/pipeline/submit_sequencer.hpp"
#include "video/video_plane/core/pipeline/submit_types.hpp"

namespace pipeline = kivo::video::core::pipeline;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                        \
    do {                                                                   \
        ++g_checked;                                                       \
        if (!(expr)) {                                                     \
            ++g_failed;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);  \
        }                                                                  \
    } while (false)

// ---- C6-1 新建：先读包 -----------------------------------------------------
void t1_fresh_reads_first() {
    pipeline::SubmitSequencer sequencer;
    CHECK(sequencer.next() == pipeline::SubmitStep::kReadSample);
    CHECK(!sequencer.has_pending());
    CHECK(!sequencer.aborted());
    CHECK(sequencer.eos_presentation_us() == 0);
}

// ---- C6-2 读到一包：先送，送出去才回到读 --------------------------------
void t2_sample_then_submit() {
    pipeline::SubmitSequencer sequencer;
    sequencer.note_sample(33'000);
    CHECK(sequencer.next() == pipeline::SubmitStep::kSubmitPending);
    CHECK(sequencer.has_pending());
    CHECK(sequencer.pending_pts_us() == 33'000);

    sequencer.note_submitted();
    CHECK(!sequencer.has_pending());
    CHECK(sequencer.next() == pipeline::SubmitStep::kReadSample);

    // 送出去的那一包进了 EOS 的 pts 候选
    CHECK(sequencer.eos_presentation_us() == 33'000);
}

// ---- C6-3 输入满：同一包重试，绝不读下一包 ------------------------------
void t3_input_full_never_reads_next() {
    pipeline::SubmitSequencer sequencer;
    sequencer.note_sample(66'000);

    // 模拟 submit 返回 TRY_AGAIN：**不调用 note_submitted**
    for (int attempt = 0; attempt < 5; ++attempt) {
        CHECK(sequencer.next() == pipeline::SubmitStep::kSubmitPending);
        CHECK(sequencer.pending_pts_us() == 66'000);
    }

    // 即使此时已经读到轨道末尾，也必须先把手上这包送掉：
    // 让 EOS 插到它前面等于丢掉最后一帧。
    sequencer.note_end_of_stream();
    CHECK(sequencer.next() == pipeline::SubmitStep::kSubmitPending);

    // 终于送出
    sequencer.note_submitted();
    CHECK(sequencer.next() == pipeline::SubmitStep::kSignalEos);
}

// ---- C6-4 读完：发 EOS，pts 取已送出包的最大值 -------------------------
void t4_eos_uses_max_submitted_pts() {
    pipeline::SubmitSequencer sequencer;
    sequencer.note_sample(0);
    sequencer.note_submitted();
    sequencer.note_sample(40'000);
    sequencer.note_submitted();

    sequencer.note_end_of_stream();
    CHECK(sequencer.next() == pipeline::SubmitStep::kSignalEos);
    CHECK(sequencer.eos_presentation_us() == 40'000);
}

// ---- C6-5 EOS 送出即收工 ------------------------------------------------
void t5_eos_signaled_finishes() {
    pipeline::SubmitSequencer sequencer;
    sequencer.note_sample(1'000);
    sequencer.note_submitted();
    sequencer.note_end_of_stream();
    CHECK(sequencer.next() == pipeline::SubmitStep::kSignalEos);

    sequencer.note_eos_signaled();
    CHECK(sequencer.next() == pipeline::SubmitStep::kFinished);
}

// ---- C6-6 EOS 只发一次 --------------------------------------------------
void t6_eos_signaled_only_once() {
    pipeline::SubmitSequencer sequencer;
    sequencer.note_end_of_stream();
    CHECK(sequencer.next() == pipeline::SubmitStep::kSignalEos);
    sequencer.note_eos_signaled();

    // 即使又被上报一次"读完了"，也不许再发第二次 EOS
    sequencer.note_end_of_stream();
    CHECK(sequencer.next() == pipeline::SubmitStep::kFinished);
    CHECK(sequencer.next() == pipeline::SubmitStep::kFinished);
}

// ---- C6-7 读失败 / 加密包：立刻终止且带原因 ----------------------------
void t6_abort_carries_reason() {
    pipeline::SubmitSequencer sequencer;
    CHECK(std::strlen(sequencer.abort_reason()) == 0);

    sequencer.note_aborted("encrypted sample encountered");
    CHECK(sequencer.next() == pipeline::SubmitStep::kAbort);
    CHECK(sequencer.aborted());
    CHECK(std::strlen(sequencer.abort_reason()) > 0);

    // 终止优先于一切：即使手上还有包也不许再送
    pipeline::SubmitSequencer with_pending;
    with_pending.note_sample(5'000);
    with_pending.note_aborted("extractor read failed");
    CHECK(with_pending.next() == pipeline::SubmitStep::kAbort);

    // 空原因不许透传成"空字符串"——那等于静默失败（G9）
    pipeline::SubmitSequencer empty_reason;
    empty_reason.note_aborted("");
    CHECK(std::strlen(empty_reason.abort_reason()) > 0);
}

// ---- C6-8 pts 乱序：EOS 取最大值而不是最后一包 -------------------------
void t7_out_of_order_pts() {
    pipeline::SubmitSequencer sequencer;
    sequencer.note_sample(100'000);
    sequencer.note_submitted();
    sequencer.note_sample(40'000);  // B 帧重排：显示时刻更早
    sequencer.note_submitted();
    sequencer.note_sample(90'000);
    sequencer.note_submitted();

    sequencer.note_end_of_stream();
    CHECK(sequencer.next() == pipeline::SubmitStep::kSignalEos);
    CHECK(sequencer.eos_presentation_us() == 100'000);
}

// ---- 步骤名的稳定性（进日志与证据，改名字会让历史证据对不上）----------
void t8_step_names_are_stable() {
    CHECK(std::strcmp(pipeline::submit_step_name(pipeline::SubmitStep::kReadSample),
                      "read_sample") == 0);
    CHECK(std::strcmp(pipeline::submit_step_name(pipeline::SubmitStep::kSubmitPending),
                      "submit_pending") == 0);
    CHECK(std::strcmp(pipeline::submit_step_name(pipeline::SubmitStep::kSignalEos),
                      "signal_eos") == 0);
    CHECK(std::strcmp(pipeline::submit_step_name(pipeline::SubmitStep::kFinished),
                      "finished") == 0);
    CHECK(std::strcmp(pipeline::submit_step_name(pipeline::SubmitStep::kAbort), "abort") == 0);
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase kCases[] = {
    {"C6-1 新建先读包", t1_fresh_reads_first},
    {"C6-2 读到包先送再读", t2_sample_then_submit},
    {"C6-3 输入满不读下一包", t3_input_full_never_reads_next},
    {"C6-4 EOS 取已送出最大 pts", t4_eos_uses_max_submitted_pts},
    {"C6-5 EOS 送出即收工", t5_eos_signaled_finishes},
    {"C6-6 EOS 只发一次", t6_eos_signaled_only_once},
    {"C6-7 终止带非空原因", t6_abort_carries_reason},
    {"C6-8 pts 乱序不影响 EOS", t7_out_of_order_pts},
    {"C6-9 步骤名稳定", t8_step_names_are_stable},
};

}  // namespace

int main() {
    std::printf("=== P12-C6 SubmitSequencer tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
