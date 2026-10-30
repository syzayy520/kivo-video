# P12-E3 设计：BT2020 色域标记 —— "可查"到底查什么

- 版本：V1.0
- 日期：2026-10-30
- 卡片判据：**输出色域与片源一致，可查**
- 关联：`38-p12e2-status.md` §六（E2 给 E3 的交接）、`37-p12e2-design.md` §2.3

---

## 一、先把"可查"拆成三层，否则会拿"我设了"冒充"它认了"

E2 结束时能证明的只有一件事：`ANativeWindow_setBuffersDataSpace` 返回了 0。
那**不等于**合成器按 HDR 处理 —— 三次真机实测把这条链拆成了三段，每段各有观测点：

| 层 | 观测对象 | 谁提供 | E2 时的状态 |
| --- | --- | --- | --- |
| ① 我们设的 | `ANativeWindow_setBuffersDataSpace` 的返回值 + 回读 | 我们自己 | 有 `applied`/`value`，**没有回读** |
| ② 合成器看到的 | SF 图层 dump 里的 `dataspace=` / `hdr metadata types=` | `dumpsys SurfaceFlinger` | 未取，**本轮实测可读** |
| ③ 屏幕进入的状态 | SF 的 `current dynamic_range` / `Current Color Mode` | `dumpsys SurfaceFlinger` | 未取，**本轮实测随片源翻转** |

**卡片要的"可查"是 ② + ③**：别人拿同一份片源、同一台机器，能复算出"这一层被标成 BT2020/PQ"。
① 只是我们的账，不能单独当证据（E2 已经演示过"设成功了但符号其实是 null"这种自欺）。

---

## 二、本轮先做的一手观测（K40 / `b6e950ee`）

HDR10 片源（`probe_1080p_hevc10_hdr10.mp4`）播放中：

```
* Layer ... (SurfaceView[com.kivo.cinema/...ui.PlayerActivity](BLAST)#120557)
      ... dataspace=BT2020_ITU_PQ (298188800) hdr metadata types=3 ...
current dynamic_range: HDR
Current Color Mode: standard_hdr
HDR Panel:true
```

无标记 SDR 片源（`probe_1080p_hevc10.mp4`）播放中：

```
      ... dataspace=V0_BT709 (281083904) hdr metadata types=0 ...
current dynamic_range: SDR
Current Color Mode: smart_MC
HDR Panel:true
```

三件事因此确定：

1. **图层级可判别**：`dataspace` 名字不同（`BT2020_ITU_PQ` vs `V0_BT709`），`hdr metadata types` 3 vs 0。
2. **显示级随之翻转**：`dynamic_range` HDR↔SDR、`Current Color Mode` `standard_hdr`↔`smart_MC`。
   这是**整屏状态**，不是"这个 App 的状态"——判据写的时候要说清。
3. **`HDR Panel:true` 是恒定面板能力，不是判据**。SDR 播放时它也是 `true`，
   谁把它当"进入了 HDR"就是把能力当状态。这一条必须写进文档。

> ⚠️ **数值不等是正常的，不是 bug。** 我们设的是 `163971072`（`ADATASPACE_BT2020_PQ`），
> SF 报的是 `298188800`（`BT2020_ITU_PQ`），差 `0x08000000`。
> SF 报的是**缓冲自己的 dataspace 编码**（多一个 range 位），不是"我们设的那个值"。
> 所以判据比的是**名字/语义**（BT2020 + PQ），不是整数相等 ——
> 比整数会得到"下发失败"的假结论，而它其实成功了。数值两个都记进证据。
> 本轮**没有**去解 SF 的位域打包（那要读 AOSP 源码，容易写成"看起来对"的东西）；
> 名字是从 SF 自己的 `dataspace=` 打印里取的。

---

## 三、做法

### 3.1 主机侧探针（新增，唯一"取证"入口）

`tools/probe_layer_dataspace.py`：

1. `adb shell dumpsys SurfaceFlinger` 抓一份完整 dump（原始文本一并落盘，供复算）；
2. 按**完整层名**找到 App 的 BLAST 层块（`SurfaceView[<pkg>/<activity>](BLAST)#<id>`）——
   层名规则与 D5 的教训同源：**少了 `#id` 或少了 `(BLAST)` 就匹配不到，不报错**；
3. 从该块里取 `dataspace=<name> (<value>)` 与 `hdr metadata types=<n>`；
4. 从 dump 里取全局的 `current dynamic_range:`、`Current Color Mode:`、`HDR Panel:`；
5. 落 JSON（schema `kivo.p12e3.layer-dataspace/1`），含原始行 `raw_lines`。

**它不改播放器、不判断好坏**，只如实取数 —— 与 `probe_present_timing.py` 同一定位。

### 3.2 窗口级回读（改播放器，一行事实）

E2 的证据只有"我们调用了、返回 0"。补一个 `ANativeWindow_getBuffersDataSpace`
（API 29，同样走 `dlopen("libnativewindow.so")`）回读窗口当前值，进证据：

```json
"dataspace": {
  "applied": true, "value": 163971072,
  "readback": { "ok": true, "value": 163971072, "reason": null }
}
```

判据：HDR 轮 `readback.value == value`。**回读不等于合成器认账**，它只证明"设进去没被立刻改掉"；
真正的认账看 §3.1 的图层 dump。两个都记，别互相顶替。

### 3.3 独立复算工具

`tools/verify_p12e3_layer.py`：读探针产物，**不 import 探针**（同 D5 的规矩：
共用代码只能证明"它跟自己一致"），独立断言：

| 组 | 断言 |
| --- | --- |
| HDR 组（3 轮） | 图层 `dataspace_name == "BT2020_ITU_PQ"`、`metadata_types != 0`、`dynamic_range == "HDR"` |
| SDR 组（2 轮） | `dataspace_name != "BT2020_ITU_PQ"`（实测 `V0_BT709`）、`metadata_types == 0`、`dynamic_range == "SDR"` |
| 对照 | 两组的 `dataspace_value` 必须不同 —— 相同就说明"标记没起作用"，前面三条全绿也不算过 |

---

## 四、判据与它的边界

### 4.1 算过

| 项 | 判据 |
| --- | --- |
| 输出色域与片源一致 | HDR10 片源图层 `dataspace` 名字为 `BT2020_ITU_PQ`；无标记 SDR 片源不为 BT2020 |
| 可查 | 证据里有 `dataspace` 的**名字与数值**、`hdr metadata types`、`dynamic_range`，且带原始 dump 行 |
| 可复算 | `verify_p12e3_layer.py` 从原始 dump 独立重算，5 份全过 |
| 不冒充 | 文档与证据都写明：`applied` 只是我们的账；`HDR Panel:true` 是面板能力不是状态 |

### 4.2 明确不做

- **不做 tone mapping**（E4）。本轮只证明"链路认得出这是 HDR"，不证明"画面看起来对"。
- **不追求整数相等**（见 §二 的 ⚠️）。比名字，不比位域。
- **不覆盖 HLG / Dolby Vision**。没有 HLG 片源；DV 连片源都没有，`hvcC` 里的 `dvcC` 也没碰。
- **不测多显示器 / 投屏 / 外接 HDMI**。K40 只有内屏一条链。
- **不解释 SF 的 dataspace 位域打包**。

---

## 五、家族树（新增/改动）

| 新增/改动 | 位置 | 理由 |
| --- | --- | --- |
| `probe_layer_dataspace.py`（**新**） | `tools/` | 读的是 `dumpsys`，不是我们的内存 —— 与 `probe_present_timing.py` 同族 |
| `verify_p12e3_layer.py`（**新**） | `tools/` | 独立复算，不 import 探针 |
| `present/surface_dataspace.{hpp,cxx}` | 加 `read_surface_dataspace()` | 仍然是"对窗口做的事"，与 set 同族 |
| `decode/hdr_delivery_facts.hpp` | 加 `DataSpaceReadback` | 纯事实，与 `DataSpaceOutcome` 并列 |
| `session/decode_session.cxx` | set 之后回读一次 | 会话是窗口的持有者 |
| `session/decode_stats_renderer.cxx` | `hdr.dataspace.readback`，schema 升 `/9` | 证据出口 |
| 证据目录 | `docs/evidence/p12e3/` | 按阶段分目录，与 `p12c/`、`p12d/` 一致 |

**不新开族**：`tools/` 已有 6 个脚本，这是第 7、8 个，职责与 D5 的两个正交（那两个测上屏时刻）。

---

## 六、风险

| 风险 | 处置 |
| --- | --- |
| 层名匹配不到（少了 `#id`/`(BLAST)`） | **不报错、只返回空** —— 这是 D5 踩过的坑。探针匹配不到时显式失败并打印候选层名，不写空文件 |
| 显示级状态是"整屏"的，可能滞后一轮 | 每轮都重新 dump；`dynamic_range` 与图层 dataspace 必须**同一份 dump** 内自洽，跨 dump 不比较 |
| 比整数相等导致假阴性 | 判据只比名字（§二 ⚠️），数值两个都记 |
| `HDR Panel:true` 被当成"进入 HDR" | 文档两处点名；`verify` 不用它做判据 |
| 探针在 App 退出后 dump | 探针先确认层存在；PlayActivity 带 `kivo.evidence` 时会播完自退，所以**取证轮不带 evidence extra**，由探针自己 dump |
