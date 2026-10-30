#pragma once

#include <jni.h>

#include <string>

namespace kivo::jni_bridge {

/// 一次播放会话在 JNI 层的句柄。
///
/// 为什么需要它：ANativeWindow 是从 Java Surface 换来的，必须在 Java 侧 Surface
/// 消亡前释放；而解码会话又依赖它。把「窗口持有」与「解码会话」绑在同一个句柄上，
/// 释放顺序就只有一处，不会出现「会话还在跑但窗口已被回收」的悬空指针。
///
/// 句柄不对 Java 暴露任何字段，Kotlin 侧只有一个 Long。
struct SessionHandle;

[[nodiscard]] SessionHandle* session_create();
void session_destroy(SessionHandle* handle);

/// 换输出 Surface（P12-D1）。**可在播放过程中调用。**
///
/// @param surface 新 Surface；传 `nullptr` 表示"面没了，先摘掉"（切到 ByteBuffer 模式）。
///        传 null 之后必须尽快传一个新的，否则这段期间解出的画面全部丢弃。
///
/// 这是"切后台再回来继续播"与"从头重播"的分界。旧实现直接在 `onSurfaceLost()` 里
/// 销毁整个会话，于是回来时是从第一帧重开 —— 那不是恢复。
///
/// 换面不动会话状态：帧计数、节拍锚点、时钟全部连续，所以恢复只需等第一帧，
/// 不需要重新追帧。
[[nodiscard]] int session_rebind_window(SessionHandle* handle, JNIEnv* env, jobject surface,
                                        std::string& error);

/// 取 Surface 对应窗口并打开媒体。窗口所有权转入句柄。
/// @param fd 只读文件描述符（Java 侧持有，调用本函数期间必须有效）
/// @param length 文件字节数
/// @param label 诊断用显示名
/// 成功返回 0；失败返回非 0 并写入 error。
[[nodiscard]] int session_open(SessionHandle* handle, JNIEnv* env, jobject surface, int fd,
                               std::int64_t length, const std::string& label,
                               std::string& error);

[[nodiscard]] int session_start(SessionHandle* handle, std::string& error);

/// 定位到指定时刻并重新起跑（P12-F1）。**必须从控制线程调用。**
///
/// @param target_us 目标位置（微秒）。实现走 `PREVIOUS_SYNC`，所以**实际落点会 ≤ 目标**，
///        落点由证据里的 `seek.landing_us` 如实记录。
///
/// 会停掉并重建解码线程，帧计数归零（新位置就是新时间轴）。调用方不该在 seek 期间
/// 同时销毁会话；`PlaybackSession` 用单线程顺序调用保证了这一点。
///
/// 成功返回 0；失败返回非 0 并把原因写进 error。
[[nodiscard]] int session_seek(SessionHandle* handle, std::int64_t target_us, std::string& error);
void session_stop(SessionHandle* handle);

/// 开启按节拍呈现（P12-C5）。必须在 `session_start` 之前调用。
///
/// @param target_fps 期望帧率，仅用于阈值兜底；真实帧时长由 pts 差值观测得出。
///        传 0 表示不预设，用 40ms 兜底。
/// @param allow_drop 是否允许按节拍丢帧（false 时只睡不丢，用于对照）。
///
/// **时钟选择（P12-C8）**：`session_open_audio` 成功起流之后，这里用**音频主时钟**；
/// 否则用 `SystemClockSource`，证据里 `clock_source` 会如实写 `system`。
/// 这条分支就是"有没有做 A/V 同步"的唯一开关 —— 不许在同一轮里一半音频一半系统。
[[nodiscard]] int session_set_pacing(SessionHandle* handle, int target_fps, bool allow_drop,
                                    std::string& error);

/// 打开音轨并起流（P12-C8）。必须在 `session_set_pacing` 与 `session_start` 之前调用。
///
/// @param fd 与视频**同一个文件的另一个描述符**（Java 侧另开一次）
///
/// 未调用它 = 纯视频播放，行为与 C7 完全一致；调用后节拍器改由音频时钟驱动。
/// 失败一定返回非 0 且写 error：音频开不起来时**不许**悄悄退回 system 时钟继续播，
/// 否则会产出一份"看起来有 av_drift、其实是节拍偏移"的假证据。
[[nodiscard]] int session_open_audio(SessionHandle* handle, int fd, std::int64_t length,
                                     const std::string& label, std::string& error);

/// 写 A/V 合并证据（`kivo.p12c.sync/1`）。**必须在 `session_stop` 之后调用**：
/// 它要读节拍器的完整直方图，那个接口只在工作线程 join 之后才安全。
[[nodiscard]] int session_write_sync_stats(SessionHandle* handle, const std::string& path,
                                           std::string& error);

/// 选择流水线模式（P12-C6）。必须在 `session_start` 之前调用。
///
/// @param split true = 送包与收帧各一个线程（实验路径）；false = 两段同线程交替（默认）。
///
/// **默认 merged 是 P12-C6 判据的回退结果**：真机 A/B 的拆线程增益在噪声内，
/// 4K 的增益（+0.5%）反而小于 1080p10（+2.0%），不满足"增益随像素走"。
/// 保留 split 的理由不是"兼容旧行为"，而是 A/B 取证必须在同一个二进制上交替跑，
/// 以及这次的判据只否定了"送收往返是瓶颈"，没有否定"两段可并行"本身。
/// 推导见 `20-p12c6-status.md`。
/// @param present 是否把帧交给 Surface。false 只用于"只解码不上屏"的诊断轮，
///        用来把解码吞吐与呈现链路限速分开量（见 `20-p12c6-status.md`）。
/// @param at_time 呈现排程方式（P12-C7）。true = 不睡，把 `due_at` 交给
///        `releaseOutputBufferAtTime` 由合成器定时（**默认**）；false = CPU 睡到
///        `due_at` 再交（C5 结构，回退路径）。只在配了节拍且 `present == true` 时有意义。
///        默认 true 的依据见 `23-p12c7-status.md` §五。
[[nodiscard]] int session_set_pipeline(SessionHandle* handle, bool split, bool present,
                                       bool at_time, std::string& error);

/// 把当前统计写成 JSON 证据文件。
[[nodiscard]] int session_write_stats(SessionHandle* handle, const std::string& path,
                                      std::string& error);

/// 供日志使用的一行摘要（不含 JSON）。
[[nodiscard]] std::string session_summary(SessionHandle* handle);

/// 最近一次失败的原因。
///
/// 之所以必须有这条通道：本机（HyperOS）的 logcat 对 adb 完全不可见，
/// 日志一行都取不到。失败原因若不通过句柄回传，就只能靠猜。
[[nodiscard]] std::string session_last_error(SessionHandle* handle);

}  // namespace kivo::jni_bridge
