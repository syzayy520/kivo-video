# 364 · 方案 D（chunkPrefetch）公网两臂验证：**预取确实工作，但公网慢源站下无可测收益**

> #65 长期挂着 `not_measured`（279 §14：本地装置 240s 长稳 pf 臂零 buffering 已证，
> 缺"公网真实源站表头 1.5–8s 随机 + 限并发"下复验）。本批用真实 Emby 公网源站，
> `chunkPrefetch=off/on` 两臂对照，**各 150s 覆盖起播窗口**，闭合这一环。

## 一、条件（与 279 §14.3 判据同源）

| 项 | 值 |
|---|---|
| 片源 | bangumi《读书会》Videos/232935，1.02GB（D357/358 同片，已知稳定） |
| 起点 | seek 396s（与 D357/358 同一起点） |
| 公网源站基线 | TTFB 0.79/0.91/1.59s，下载 35/57/76 KB/s（慢窗口，适合判此问题） |
| 出画 | gpu-next + 硬解（截图确认两臂真实出画，非黑屏） |
| 开关 | `debug.kivo.chunkPrefetch` off（ctl）/ on（pf）；`originKeepAlive=0` 固定 |
| 采样 | 每臂 150s，判据取首帧后 0–40s 起播窗口 |

## 二、结果（正确口径复算）

| 判据 | ctl（off 对照） | pf（on 方案D） | 判定 |
|---|---:|---:|---|
| Activity 调度空窗（排除项） | 95.2s（冷启动） | 0.4s | 与 chunkPrefetch 无关，已排除 |
| 真实首帧（起播→首帧） | 3.3s | 3.7s | +0.4s，噪声内，ctl 不劣 |
| `Enter buffering` | 1 次 / 0.4s | 1 次 / 0.4s | **完全相同** |
| underrun（首帧后） | 3 次 / 首次 +2.8s | 2 次 / 首次 +3.2s | pf 略少但首次更晚，无稳定收益 |
| 预取动作（warmup/window/tail） | 2 | 34 | **方案D确实在工作** |
| `prefetch 默认关闭` 日志 | 6 | 0 | 开关生效证据 |
| `conn.fail` | 3 | 4 | pf 多 1，均在起播瞬时 |
| 实际出画 | ✅ | ✅ | 两臂均正常 |

## 三、结论

1. **开关确实生效**：pf 臂 34 次 warmup/window/tail 预取动作、0 条"默认关闭"；ctl 臂相反（2 次、6 条）。方案 D 代码路径在公网真机上是通的。
2. **公网慢源站下无可测收益**：首帧 3.3 vs 3.7s（噪声内）、`Enter buffering` 两臂完全相同（1 次 0.4s）。本地装置上"pf 臂零 buffering"的优势**未外推到公网**——公网 TTFB 1.5s 随机 + 限并发的噪声淹没了预取收益。
3. **没有负收益**：pf 臂首帧不慢于噪声、无额外 buffering、无稳态丢帧；`conn.fail` 多 1 次在起播瞬时，与 D358/D360 观察一致。
4. **underrun 不计入卡顿**：按 D362 裁决，`Render queue underrun` 是 mpv 官方预期日志（PL_QUEUE_MORE），且本批两臂均无可见卡顿（截图 + 0 稳态丢帧）。

**净判定**：方案 D 保持 **opt-in 默认 off** 不变——它不劣化公网播放，但在公网慢源站下也拿不出值得默认开启的收益。本地装置的优势是受控限速下的特例。这与 279 §14 的保守立场一致。

## 四、诚实标注 / 过程纠正

- **脚本两处统计口径 bug（已修，见 commit）**：首版判据把首帧基准取成 `net[0]`（"预备开始"），而预备完成→起播之间有 **95.2s 的 Activity 冷启动调度空窗**（ctl 臂），导致首版输出 `ctl 98.5s vs pf 4.0s` 的假 24 倍差；实际内核起播→首帧 ctl 3.3s / pf 3.7s。第二处把 prefetch 标记写成 `[prefetch]`，而真实标记是 `[play-seek] origin warmup` / `[tail] prefetch window` / `[play-tail] origin chunk`，导致首版"预取动作 0 vs 0"的假象（实际 2 vs 34）。两处均已在脚本注释与代码中修正，并用本批日志离线复验一致。**结论以修正口径为准。**
- **95s 空窗归因**：ctl 臂 kernel.log 显示 `loadfile→Starting playback` 仅 3.1s（正常），空窗在 `[prep] 预备完成(903ms)→起播开始` 之间，属前一次 force-stop 后 AMS 冷启动 Activity 的调度延迟，与 chunkPrefetch 无关；两臂均以"起播开始"为真实起点重算，已排除该污染。
- **arm 顺序固定**（ctl 先 pf 后）：冷启动空窗只落在 ctl，若交换顺序该空窗会转移到 pf。因判据用"起播开始"为起点、不受空窗影响，且两臂都真实出画，故未额外交换重跑。
- **单臂 150s**：起播窗口判据只需 0–40s，150s 覆盖 3.7 倍，足够；未做 300s 长稳（本地装置 240s 已覆盖稳态，本批只补公网起播窗口）。

## 五、证据

| 文件 | 内容 | 入库 |
|---|---|---|
| `v364/ctl/playing.png` / `pf/playing.png` | 两臂实际出画截图 | ✅ |
| `tools/p294_chunkprefetch_public_ab.sh` | 两臂采集脚本（含口径修正与前置守卫） | ✅ |
| `v364/{ctl,pf}/net.log` | 完整 net-diag（95s 空窗、预取动作标记、underrun 计时源） | `.log` 被 gitignore，本地保留 |
| `v364/{ctl,pf}/kernel.log` | mpv 内核日志（`Enter buffering` 计时源） | 同上 |
| `v364/{ctl,pf}/diag.log` | 诊断日志 | 同上 |

> 第二节对照表已是逐条复算结果的固化；原始 `.log` 遵循项目惯例（同 D358/D360）本地保留，
> 需要复查时按 `tools/p294_chunkprefetch_public_ab.sh` 重跑即可再生。

设备已恢复常态（force-stop、未锁屏、飞行 off）。ui_gate PASS。
