// end_reason_tests.cpp — P14-R tests: 会话的收尾方式
//
// 守的对象是 `session_end_reason.hpp` 里的**派生规则**与**写进 JSON 的那三个词**。
//
// 为什么这两件事值得单测：
//   1. 派生规则被两份报告共用（`decode/19` 与 `sync/29`）。两处各写一个 `if`
//      就会在某一天各自演化，而证据里会出现两种收尾口径。
//   2. 那三个词是**协议**，不是日志文案。判据按字符串比对，改一个字就是改协议。
//      `interrupted` 拼成 `interrupt` 不会让谁编译失败，只会让判据永远不生效 ——
//      而"判据永远不生效"与"判据不存在"在输出上长得一样。
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "video/video_plane/backends/mediacodec/session/session_end_reason.hpp"

namespace session = kivo::video::mediacodec;

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

/// t1–t4：派生规则的四条路。
///
/// 四条路里最容易被写错的是 t2：**跑到了 EOS 但带着错误**。
/// 同步报告的 `completed` 是 `video.completed && audio.error.empty()`，
/// 而 `error` 会兜底取音频的错误 —— 所以"视频跑完、音频挂了"这一轮
/// 收尾方式必须是 `failed`。错误优先于完成，不是相反。
void test_derivation() {
    std::printf("t1 completed=true  error=empty -> completed\n");
    CHECK(session::session_end_reason(true, false) == session::SessionEndReason::kCompleted);

    std::printf("t2 completed=true  error非空    -> failed（错误优先于完成）\n");
    CHECK(session::session_end_reason(true, true) == session::SessionEndReason::kFailed);

    std::printf("t3 completed=false error非空    -> failed\n");
    CHECK(session::session_end_reason(false, true) == session::SessionEndReason::kFailed);

    std::printf("t4 completed=false error=empty  -> interrupted（这一格以前无名）\n");
    CHECK(session::session_end_reason(false, false) == session::SessionEndReason::kInterrupted);
}

/// t5：三个词是协议。逐字比对，不用 `strstr`。
void test_names() {
    std::printf("t5 写进 JSON 的三个词\n");
    CHECK(std::strcmp(session::session_end_reason_name(session::SessionEndReason::kCompleted),
                      "completed") == 0);
    CHECK(std::strcmp(session::session_end_reason_name(session::SessionEndReason::kInterrupted),
                      "interrupted") == 0);
    CHECK(std::strcmp(session::session_end_reason_name(session::SessionEndReason::kFailed),
                      "failed") == 0);
}

/// t6：认不出来的值不许被读成"跑完了"。
///
/// 这一条守的是一个方向：将来若有人加第四个枚举值却忘了补 `switch`，
/// 回落必须落在**不安全的那一侧之外**。回落到 `completed` 会让一份
/// 来路不明的报告拿到最优结论 —— 那是最坏的一种默认值。
void test_unknown_falls_back_to_failed() {
    std::printf("t6 未知枚举值回落 failed，不是 completed\n");
    const auto unknown = static_cast<session::SessionEndReason>(99);
    const char* name = session::session_end_reason_name(unknown);
    CHECK(std::strcmp(name, "failed") == 0);
    CHECK(std::strcmp(name, "completed") != 0);
}

/// t7：三个词两两不同。
///
/// 复制粘贴是这类枚举最常见的错法：新增 `kInterrupted` 时把上一行整段复制，
/// 名字忘了改。那时 `interrupted` 与 `completed` 会印成同一个词，
/// 而判据的字符串白名单照样通过 —— 两份意思相反的收尾在证据里无法区分。
void test_names_are_distinct() {
    std::printf("t7 三个词两两不同\n");
    const char* names[3] = {
        session::session_end_reason_name(session::SessionEndReason::kCompleted),
        session::session_end_reason_name(session::SessionEndReason::kInterrupted),
        session::session_end_reason_name(session::SessionEndReason::kFailed),
    };
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            CHECK(std::strcmp(names[i], names[j]) != 0);
        }
    }
}

}  // namespace

int main() {
    test_derivation();
    test_names();
    test_unknown_falls_back_to_failed();
    test_names_are_distinct();

    std::printf("\n%d checks / %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
