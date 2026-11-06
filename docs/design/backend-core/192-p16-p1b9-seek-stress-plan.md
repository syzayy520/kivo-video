# P16-P1B9 · seek 偶发异常压测复现

- 日期：2026-11-06
- 定位：`190` C 轮出现过一次 9 秒首帧（`landing=0` 却 `video_first_pts=9.03 s`），
  `191` 同参数未复现。本卡量化它的**偶发率**，决定"继续查"还是"记为已知偶发"。
  门闸：构思 → 规划设计 → 自然家族树（本文）→ 执行 → 验收。
- 上位：`190` §五 C 轮、`191` §五 如实说明、`03-goals.md` G10。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照；**零新增脚本、零代码改动**。

---

## 一、构思

一次出现、一次消失，说明不了任何事。要么它可复现（有明确触发条件），
要么它是低概率竞态（靠压测估概率，别再投入）。两种结论都要求同一件事：**重复**。

本卡不改任何代码，只做重复实验：同片源、同参数、同设备，跑 6 轮，
统计 `video_first_pts_us > target_us` 的命中次数与 `total_us` 分布。

---

## 二、规划设计

1. **只读**：每轮只读 `files/evidence/*.json`，不改产品代码、不改证据。
2. **同参数**：`--ei kivo.seek.ms 5000 --ei kivo.seek.at.ms 1500`，与 `190` C 轮逐字一致。
3. **同片源**：`kivo_p1a_golden.mp4`（640×360 / 35 s），与 `190/191` 一致。
4. **零新增脚本**：压测用一条 shell 循环，完整命令写进本文（可复算），
   不往 `tools/` 再加一个一次性实验脚本。
5. **只操作 `dd137cf7`**；`b6e950ee` 不碰；不 `uninstall`。

### 压测命令（可复算）

```sh
ADB="C:/Android/sdk/platform-tools/adb.exe"
CLIP=/sdcard/Android/data/com.kivo.cinema/files/media/kivo_p1a_golden.mp4
for i in 1 2 3 4 5 6; do
  "$ADB" -s dd137cf7 shell "am force-stop com.kivo.cinema; am start -n com.kivo.cinema/.ui.PlayerActivity \
    --es kivo.media $CLIP --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced \
    --ei kivo.seek.ms 5000 --ei kivo.seek.at.ms 1500 --es kivo.evidence p16p1b9-r$i.json"
  "$ADB" -s dd137cf7 shell "sleep 45"
  "$ADB" -s dd137cf7 shell "run-as com.kivo.cinema cat files/evidence/p16p1b9-r$i.json" > build/p16p1b9-r$i.json
done
```

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/192-p16-p1b9-seek-stress-plan.md` | 新增 | 本卡规划、命令、判据、结果、结论 | 不复制 `190/191` 全文 |

**新增 1 个文件、零代码改动、零新增脚本。**

### 判据（R1–R3）

| 判据 | 内容 |
| --- | --- |
| R1 | 6 轮都产出 decode 证据（`outcome.completed` 有值），缺轮必须显式报出 |
| R2 | 统计 `video_first_pts_us > target_us` 的命中次数；命中轮保留 `total_us` / `pts` / `landing` |
| R3 | 汇总 `total_us` 的 min / median / max，与 `191` 的 41.8 ms 对照 |

**结论规则（写死，避免事后找理由）**：
- 命中 ≥ 2 次 → 可复现，另开卡查触发条件；
- 命中 1 次 → 低概率，记为"偶发（1/6）"，再跑 6 轮确认；
- 命中 0 次 → 记为"无法复现的偶发"，停止投入，保留归因分支作为防御。

---

## 四、验收

- 6 轮证据落在 `build/p16p1b9-r*.json`（本机临时副本）与设备 `files/evidence/`；
- 本文「五、结果」按判据填表；
- 不改任何产品代码，六个门禁保持全绿（本卡不触碰它们）。

---

## 五、结果（8 轮样本，**0 命中**）

### 为什么上轮 19 分钟只跑 2 轮

`mStayOn=false`、`mStayOnWhilePluggedInSetting=0` —— **设备息屏后 `sleep` 与播放都停住**，
每轮实际被拉长到 9 分钟。修法：压测前 `svc power stayon usb` + `input keyevent 224` +
`wm dismiss-keyguard`（已设为常亮）。

### 提速方法

`PlayerActivity` 支持 `--es kivo.finalize on_stop`（`PlayerActivity.kt:555-557`）：
把"进入后台"当成一次收尾并写证据。于是每轮变成
`start → sleep 4 → input keyevent 3(HOME) → sleep 2`，**约 6 秒/轮**，
且 `seek` 字段口径与慢速轮完全一致（`seek_at` 1.5 s、target 5000 ms）。

### 结果（2 慢 + 6 快）

| 轮 | target | landing | video_first_pts | total_us | pass | hit |
| --- | --- | --- | --- | --- | --- | --- |
| slow-r1 | 5,000,000 | 0 | 0 | 24,930 | true | no |
| slow-r2 | 5,000,000 | 0 | 0 | 38,118 | true | no |
| fast-f1 | 5,000,000 | 0 | 0 | 31,193 | true | no |
| fast-f2 | 5,000,000 | 0 | 0 | 63,096 | true | no |
| fast-f3 | 5,000,000 | 0 | 0 | 27,582 | true | no |
| fast-f4 | 5,000,000 | 0 | 0 | 48,378 | true | no |
| fast-f5 | 5,000,000 | 0 | 0 | 56,348 | true | no |
| fast-f6 | 5,000,000 | 0 | 0 | 47,366 | true | no |

`total_us` min / median / max = **24.9 / 42.7 / 63.1 ms**，全部 `pass=true`。

### 结论（按 §三 事先写死的规则）

- **命中 0 / 8 → 记为"无法复现的偶发"，停止投入**，不再为它开新卡，除非现场再遇到；
- `191` 的归因分支保留为防御：若再出现 `pts > target`，`seek.reason` 会直接指向关键帧/落点；
- `190` C 轮那 9 秒在这 8 轮里一次都没再出现。

### 如实说明

- 0/8 **不等于概率为 0**，只能说"在这台设备、这个片源、这组参数下，8 轮未复现"；
- 慢速轮 `end_reason=completed`、快速轮 `end_reason=interrupted`，但 `seek` 字段口径相同，可比；
- 不再宣称"已修复"——归因分支只是让下次说得清，不是消灭它。

### 方法学收获（写给以后）

1. 批量真机实验前先 `svc power stayon usb`，否则息屏会把 `sleep` 和播放一起冻住；
2. `--es kivo.finalize on_stop` + HOME 能把 seek 实验从 45 s/轮压到 6 s/轮，
   且不改变 `seek` 字段口径 —— 以后做 seek 矩阵优先用它。

---

## 六、明确不做

- 不改解码送包/丢弃策略、不改 `assess_seek` 阈值、不改 schema；
- 不新增 `tools/` 脚本（压测命令已在 §二）；
- 不碰他人主线、不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 不碰。
