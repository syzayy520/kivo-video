# P12-E3 验收：BT2020 色域标记 —— 合成器侧认了，且可复算

- 版本：V1.0
- 日期：2026-10-30
- 设计：`39-p12e3-design.md`
- 判据：**输出色域与片源一致，可查**
- 结论：**达成。** HDR10 片源播放时，SurfaceFlinger 图层 `dataspace = BT2020_ITU_PQ (298188800)`、
  `hdr metadata types = 3`，显示级 `dynamic_range = HDR`、`Current Color Mode = standard_hdr`；
  无标记 SDR 片源同样三项全不同（`V0_BT709` / `0` / `SDR` / `smart_MC`）。
  3 轮 HDR + 2 轮 SDR，`verify_p12e3_layer.py` 独立复算 5/5 通过，变异测试能打红。

---

## 一、证据（K40 / `b6e950ee` / Android 13）

| 轮次 | 片源 | 图层 dataspace | `hdr metadata types` | `dynamic_range` | 颜色模式 | 帧闭合 | 丢帧 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `hdr10-r1` | HDR10 | **`BT2020_ITU_PQ` (298188800)** | **3** | **HDR** | `standard_hdr` | 300/301/300 | 0 |
| `hdr10-r2` | HDR10 | `BT2020_ITU_PQ` (298188800) | 3 | HDR | `standard_hdr` | 300/301/300 | 0 |
| `hdr10-r3` | HDR10 | `BT2020_ITU_PQ` (298188800) | 3 | HDR | `standard_hdr` | 300/301/300 | 0 |
| `sdr10-r1` | 10bit SDR（无标记） | `V0_BT709` (281083904) | 0 | SDR | `smart_MC` | 300/301/300 | 0 |
| `sdr10-r2` | 同上 | `V0_BT709` (281083904) | 0 | SDR | `smart_MC` | 300/301/300 | 0 |

窗口级回读（`decode/9` 新增字段）：HDR 三轮 `readback.ok=true, value=163971072`（与我们设的一致）；
SDR 两轮 `ok=true, value=0`（我们没动窗口，回读就是原值 0）。五轮全部 `completed=true`、`samples_rejected=0`。

产物（`docs/evidence/p12e3/`，10 份）：
`p12e3-{hdr10-r1,hdr10-r2,hdr10-r3,sdr10-r1,sdr10-r2}-layer.json`（合成器侧）+
`p12e3-*-decode.json`（应用侧，schema `kivo.p12c.decode/9`）。

---

## 二、四件必须说清的事

### 2.1 三层事实分开记，谁也别替谁下结论

| 层 | 字段 | 本轮结论 |
| --- | --- | --- |
| 我们调用成功 | `hdr.dataspace.applied` | `true`（HDR）/ `false`（SDR，故意的） |
| 窗口上真是这个值 | `hdr.dataspace_readback` | `163971072` / `0`，`ok=true` |
| 合成器认了 | SF 图层 `dataspace` | `BT2020_ITU_PQ` / `V0_BT709` |
| 屏幕进入 HDR | SF `dynamic_range` | `HDR` / `SDR` |

E2 只能证明第一层；E3 把后三层补齐。**任何一层单独拿出来都不够** ——
E2 已经演示过"函数指针是 null 却报告成功"这种情况。

### 2.2 数值不等不是 bug：163971072 ≠ 298188800

我们设的是 `ADATASPACE_BT2020_PQ = 163971072`；SF 报的是 `BT2020_ITU_PQ = 298188800`，差 `0x08000000`。
SF 报的是**缓冲自己那份 dataspace 编码**（多一个 range 位），不是"我们设进去的那个整数"。
所以判据比的是**名字/语义**（BT2020 + PQ），比整数会得到"下发失败"的假结论。
本轮**没有**去解 SF 的位域打包（那要读 AOSP 源码，容易写成"看起来对"的东西）。

### 2.3 `HDR Panel:true` 不是判据

SDR 播放时它同样是 `true` —— 那是**面板能力**（和 `dumpsys display` 里
`mSupportedHdrTypes=[1,2,3,4]`、`mMaxLuminance=420` 一致），不是"现在正显示 HDR"。
校验器不用它做判据，探针只照记。这个坑和 D5 的"层名少 `#id`"同性质：
**一个恒定值看起来像状态，写进判据就会永远通过。**

### 2.4 `hdr metadata types=3` 是本轮最值得记的一笔（但别过度解读）

HDR 轮 SF 报 `metadata types=3`（SMPTE2086 | CTA861_3），SDR 轮为 0。
这说明**静态元数据确实到了合成器** —— 但它不是我们下发的：E2 已证
`hdr-static-info` 键在轨道格式里不存在，ST2086/CLL 只在 SEI 里。
合理解释是解码器自己解析 SEI 后附着在输出缓冲上。

**这只能证明"类型位存在"，不能证明"值正确"。** 值对不对要看 E4 或另立卡去读
`ANativeWindow`/`AHardwareBuffer` 上的 metadata（API 34 的 `AHardwareBuffer_getMetadata`），
本轮没做。所以 E2 那条"静态元数据未下发"的结论不变 —— 变的是**链路里其实有另一条路**。

---

## 三、工具与代码

| 改动 | 位置 | 说明 |
| --- | --- | --- |
| 图层探针（**新**） | `tools/probe_layer_dataspace.py` | 现取 `#id` 层名 → 截层块 → 读 dataspace / metadata types / dynamic_range；取不到显式失败并打印候选 |
| 独立复算（**新**） | `tools/verify_p12e3_layer.py` | **不 import 探针**，用自己的正则从 `raw_lines` 重算，再与字段比对；两组必须相异 |
| 窗口回读（**新**） | `present/surface_dataspace.{hpp,cxx}` | `read_surface_dataspace()`，`dlopen("libnativewindow.so")` + `dlsym` |
| 事实字段（**改**） | `decode/hdr_delivery_facts.hpp` | 加 `DataSpaceReadback` |
| 编排（**改**） | `session/decode_session.cxx` | 设完立刻回读 |
| 证据（**改**） | `session/decode_stats_renderer.cxx` | `hdr.dataspace_readback`，schema 升 **`/9`** |

**C++ 只加了一个回读函数**：真正的判据在合成器侧，去进程里加代码追它是猜。
这与 D5"上屏时刻 C++ 一行没加"是同一条取舍。

---

## 四、诚实清单（六条）

1. **"输出色域与片源一致"是在名称层面成立的**（`BT2020_ITU_PQ`），
   **没有**验证像素数值、白点、亮度曲线是否正确。那是 E4 的事。
2. **`dynamic_range` / `Current Color Mode` 是整屏状态**，不是本 App 私有。
   本轮每轮开播前 `force-stop` 再起，尽量让状态由当前片源决定；
   但**没有做"先播 HDR 再立刻播 SDR"的切换瞬态测试**，状态滞后未验证。
3. **只有一台设备、一个内屏**。投屏 / 外接 HDMI / 多显示器未测 ——
   `HDR Panel:true` 这类能力在不同链路上完全不同。
4. **`hdr metadata types=3` 只证明类型位存在**，不证明 mastering display / MaxCLL 的数值正确；
   也没有证据说明它来自 SEI 而不是别的路径（合理推断，不是观测）。
5. **层名匹配依赖 `(BLAST)` 与 `#id` 两个片段**。MIUI 或不同 Android 版本若改名，
   探针会失败（**会显式失败**，这是与 D5 那次"静默返回空"的关键差别）。
6. **探针在开播后 3 秒取样**，只代表那一刻的状态。长时间播放中途是否掉回 SDR，未测。

---

## 五、给 E4 的交接

- **E4 的第一件事不是写 tone mapping，而是定义"画面正确"怎么测。**
  本轮已证明：HDR 内容被交给合成器、面板切进 HDR 模式。若面板是 SDR（或用户关掉 HDR），
  画面会过曝 —— 但"过曝多少"需要截图 + 直方图，不是看指标。
- **可用起点**：`artifacts/p12e2/p12e2-hdr10-playback.png`（HDR 轮截图）、
  `tools/probe_layer_dataspace.py`（能把"这一轮到底有没有进 HDR"钉住，避免拿 SDR 轮当 HDR 证据）。
- **别用 `HDR Panel` 判"现在是不是 HDR"**（§2.3）。
- 若要补 E2 的静态元数据下发，先决定走哪条：自己解析 SEI 构造 `hdr-static-info`，
  还是承认"解码器已经把它带在缓冲上"（§2.4）。两者不能同时当结论。

---

## 六、复算步骤

```bash
ADB_WIN="C:/Android/sdk/platform-tools/adb.exe"; ADB="$ADB_WIN"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media

# 1. 播起来（必须带 Surface；探针在开播 3s 时取样）
MSYS_NO_PATHCONV=1 /c/Android/sdk/platform-tools/adb.exe shell \
  "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/hdr10_1080p30.mp4 \
   --ei kivo.pace 1 --ei kivo.drop 1 --ei kivo.fps 30 --es kivo.evidence p12e3-hdr10-r1-decode.json"
sleep 3

# 2. 合成器侧
ADB="$ADB_WIN" python tools/probe_layer_dataspace.py --tag hdr10-r1 \
  --out docs/evidence/p12e3/p12e3-hdr10-r1-layer.json

# 3. 应用侧（播完自退后）
MSYS_NO_PATHCONV=1 /c/Android/sdk/platform-tools/adb.exe shell \
  "run-as com.kivo.cinema cat files/evidence/p12e3-hdr10-r1-decode.json" \
  > docs/evidence/p12e3/p12e3-hdr10-r1-decode.json

# 4. 独立复算（5 份一起喂；缺一组就报"分组不全"）
python tools/verify_p12e3_layer.py docs/evidence/p12e3/*-layer.json

# 变异自检：把任一 HDR 轮的名字改成 V0_BT709，必须打红两条
```

> **Windows 上给 Python 传 adb 要用 Windows 路径**（`C:/Android/.../adb.exe`），
> 传 MSYS 路径 `/c/Android/...` 会 `FileNotFoundError: [WinError 2]` ——
> 尤其当 `MSYS_NO_PATHCONV=1` 时连自动转换都不会发生。这一轮踩了一次。
