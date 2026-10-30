# P12-C5 · 呈现节拍器 · 验收

- 版本：V1.0
- 日期：2026-10-29
- 上游：`17-p12c5-design.md`（设计与家族树）
- 证据：`docs/evidence/p12c/p12c5-{pace-24fps,pace-30fps,pace-60fps,nopace-24fps}.json`
- 设备：Redmi K40（M2012K11AC / alioth），codec `OMX.qcom.video.decoder.avc`

---

## 一、结论

**G6 三项全部达标，且余量两个数量级。**

| 档位 | 提交帧 | 呈现 | 丢帧 | 丢帧率 | `present−due` p99 | 间隔偏差 p99 | 墙钟 / 片长 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 24 fps | 192 | 193 | **0** | **0.00%** | **0.125 ms** | 0.074 ms | 8.047 s / 8.000 s |
| 30 fps | 180 | 181 | **0** | **0.00%** | **0.125 ms** | 0.072 ms | 6.072 s / 6.000 s |
| 60 fps | 480 | 481 | **0** | **0.00%** | **0.125 ms** | 0.125 ms | 8.048 s / 8.000 s |
| 对照（关节拍，24 fps） | 192 | 193 | — | — | 未测 | 未测 | **1.087 s** = 7.36× 实时 |

G6 判据：丢帧率 < 0.5%（实测 0%）、p99 < 1 帧时长（24 fps 判据 41.7 ms，实测 0.125 ms）。

**0.125 ms 是桶宽的一半**，也就是这套直方图能表达的最小非零值。真值在 0–0.25 ms 之间，
所以要读成「**抖动小于测量分辨率**」，不许读成「精确等于 0.125 ms」。

`frames_decided` 比片源帧数多 1 是正常现象：MediaCodec 会额外吐一个带 EOS 标志的空帧。
它走「pts 重复」分支，按一帧时长顺延，不影响统计（这也是 8.047 s 比 8.000 s 多 47 ms 的来源）。

---

## 二、对照组的价值

关掉节拍后同一片源 **1.087 秒**跑完（7.36× 实时）。这正是 P12-B 测到的行为：
解码器全速跑，画面靠 SurfaceFlinger 自己队列化，跟"按时间呈现"完全无关。

所以这三档 0.99× 的数字不是"解码变慢了"，而是**时间轴接管了呈现节奏**。
没有这一组对照，"p99 = 0.125ms"无法排除"其实根本没在等"的可能。

---

## 三、时钟健康度：三种降级一次都没触发

| 指标 | 三档实测 |
| --- | --- |
| `clock_source` | `system`（纯视频，无音轨 —— **不是** `audio_timestamp`） |
| `extrapolated_frames` | 0 |
| `free_run_frames` | 0 |
| `jump_rebases` | 0 |
| `monotonic_rejects` | 0 |
| `stale_readings` | 0 |

**这一格必须说清楚**：C5 的真机取证用的是 `SystemClockSource`（独立单调时间轴），
因为这一阶段视频单独播，没有音轨可锚。所以：

- ✅ 证明了 **G6（节奏）**：按到期时刻呈现，抖动 < 0.25 ms；
- ❌ **没有**证明 **G7（音画同步）**：G7 要 `audio_clock` 驱动 + 音视频同时跑。

把 C5 的数字写成"已达成 G7"属于红线第 5 条。G7 是 C8 的闸门。

三种降级（外推 / 自走 / 重锚）在真机上一次都没触发，因为它们要"时钟停更或跳变"才发生，
正常播放不会出现。**这三条由单测守**（C5-9/10/11），不是靠真机碰运气。

---

## 四、`frame_duration` 是观测出来的，不是声明的

| 档位 | 容器声明 | 观测值 | 推导 fps |
| --- | --- | --- | --- |
| 24 | 24/1 | 41,667,000 ns | 23.9998 |
| 30 | 30/1 | 33,333,000 ns | 30.0000 |
| 60 | 60/1 | 16,667,000 ns | 59.9990 |

证据里同时写 `frame_duration_ns` 与 `derived_fps`，第三方可复算。
若哪天出现"声明 30 实测 29.97"，这里会先暴露，而不是让节拍器悄悄按错帧率睡。

---

## 五、单测覆盖（17 例 / 69 断言）

`kivo_p12c5_pace_tests`，桌面 Debug，全绿；`ctest -R kivo_p12c` 4/4 无回归。

| 用例 | 守的是什么 |
| --- | --- |
| C5-1 首帧即刻到期 | 首帧不因启动/解码延迟被判迟到 |
| C5-2 早到则睡到到期 | `wait_ns` 与真实睡眠时刻都准 |
| C5-3 容差内不睡 | 2 ms 边界不触发 sleep 系统调用 |
| C5-4 迟到 50 ms 仍呈现 | 迟到不等于丢 |
| C5-5 迟到 150 ms 丢弃 | 丢弃带 reason |
| C5-6 连续落后触发追帧 | 第 11 帧进 `kCatchUp`，且不退回 `kDrop` |
| C5-7 追到即退出追帧 | 落后 ≤ 50 ms 立即恢复呈现 |
| C5-8 pts 重复/倒退 | `due_at` 只前进，不出现负睡眠 |
| C5-9 时钟跳变重锚 | 跳 1.5 s 不导致整段判迟到 |
| C5-10 短暂停更 | 按墙钟外推，不丢帧 |
| C5-11 停更过久 | 降 `kFreeRun` 且降级当帧从现在起算 |
| C5-12 随时钟推进 | 正常路径 `due == now + (pts − elapsed)` |
| C5-13 阈值边界 | 2 ms / 100 ms 边界精确，`sane()` 拒绝自相矛盾配置 |
| C5-14 分位数准确 | ±0.5 ms 内对上手算 |
| C5-15 溢出桶 | ±128 ms 之外仍给精确极值 |
| C5-16 睡过头 | 0.5 ms 过冲如实进直方图（迟到与间隔两处） |
| C5-17 分位数不超极值 | 防止 "p99 > max" 这种自相矛盾的证据 |

C5-11 和 C5-17 是**实现过程中被真数据逼出来的**：

- C5-11：一开始降级后接着用降级前的 `last_due`，结果时间轴比现实落后 500 ms+，
  随后几十帧全被判迟到 —— 降级自己制造了一场丢帧风暴；
- C5-17：首轮真机证据出现 `interval.p99 = 0.125 ms` 而 `max = 0.079 ms`，
  因为分位数取桶中心、`max` 取真实值。已加钳制，并留用例。

---

## 六、落位与改动清单

**新增（全部零平台依赖）**

```
backend/include_private/video/video_plane/core/pace/
├── pace_action.hpp          # 动作与健康度枚举 + PaceDecision
├── pace_policy.hpp          # 阈值 + decide() 纯函数
├── pace_histogram.hpp       # 有符号定宽直方图（1026 桶 / 零分配）
├── pace_stats.hpp           # 聚合计数与分位数
├── clock_position_tracker.hpp  # 陈旧外推 / 跳变重锚 / 倒退钳制
├── frame_pacer.hpp          # 编排：pts → due_at → 动作 → 等待 → 记账
└── pace_now.hpp             # NowFn / WaitFn（可注入）
backend/src/video/video_plane/core/pace/*.cxx（7 个）
backend/tests/video/pace/p12c5_pace_tests/frame_pacer_tests.cpp
```

**改动**

| 文件 | 改了什么 | 为什么 |
| --- | --- | --- |
| `mediacodec/decode/decoded_frame.hpp`（新） | 帧描述：`index` / `pts` / `eos` | 呈现决策需要"还没交出去"的帧 |
| `mediacodec_video_decoder.{hpp,cxx}` | `receive(frame, timeout)` 不再内部 release；新增 `release_frame(index, render)` | `render` 参数是唯一能表达"丢帧"的开关 |
| `mediacodec/session/decode_session.{hpp,cxx}` | 新增 `attach_pacing()`、按决策 wait/drop、节拍采样 | C5 的接线点 |
| `mediacodec/session/decode_pace_stats.hpp`（新） | 节拍度量（不带直方图） | 快照不能每次复制 24 KB |
| `mediacodec/session/decode_pace_report.{hpp,cxx}`（新） | `PaceSummary → DecodePaceStats` | 转换只有一处，改口径只改这里 |
| `decode_stats_renderer.cxx` | 新增 `pace` 段；schema → `kivo.p12c.decode/2` | 证据要能自证 |
| `jni_session_handle.{hpp,cxx}` | `session_set_pacing(fps, allow_drop)`；`SystemClockSource` 成员 | 时钟必须比会话活得久 |
| `jni_player_bridge.cxx` | 导出 `nativeSessionSetPacing` | — |
| `PlayerActivity.kt` / `NativePlayer.kt` | 支持 `kivo.media/pace/drop/fps/evidence` 五个 extra | 三档取证要能脚本化，不靠手点 |
| `android/.../CMakeLists.txt` | 新增 `kivo_core_pace` target；mediacodec 链它 | 同一份源两端编 |

**架构检查**：`core/pace/` 下无 `<android/*>`、无 `<GLES*>`
（`grep -rn "android/\|GLES" backend/include_private/video/video_plane/core/pace backend/src/video/video_plane/core/pace` 为空）。

---

## 七、明确没做（防止下一张卡跑偏）

| 没做 | 归谁 |
| --- | --- |
| `releaseOutputBufferAtTime` 交给合成器排程 | C7 |
| 解送/收帧分线程（现在解码在等待期间是空闲的） | C6 |
| `\|video_pts − audio_clock\|` 偏差统计 | C8 |
| seek 后重建节拍（只留了 `reset()` 接口） | P12-F |
| 上屏延迟（光子到眼睛）测量 | 本阶段口径是"调度抖动"，见 `17-p12c5-design.md` §六 |

**一条要提前说清的技术债**：C5 在**解码线程里**睡到 `due_at`，等待期间解码器是空闲的。
30/60 fps 下每帧睡 33/16 ms，期间不送包也不收帧。这不会丢帧（实测 0），
但把"解码"与"呈现"绑成了串行。C6 拆线程后这条会消失；**不要**在此之前
用"提高批大小"之类的办法去掩盖它。
