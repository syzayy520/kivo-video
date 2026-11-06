# P16-P1B17 · `drift_band` 参数化与四组对照实验

- 日期：2026-11-06
- 定位：把 `197` 的"调 band 够不够"从**推演**变成**实测**。门闸：构思 → 规划设计 → 家族树 → 实现 → 验收。
- 上位：`197` §五「追问：调大 band 够不够」、`183` §七 Q2。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**；无新目录。

---

## 一、构思

`197` 用 `PacePolicy` 的逻辑推出"调大 band 能同时降残差、减触发，但单次修正变大"，
并标注了**这是推演不是实测**。要变成实测，得先把 `drift_band_ns` 从编译期常量
变成可传参数 —— 它目前没有任何入口（`PacePolicy` 默认 5 ms）。

参数化本身也有独立价值：以后做任何 pacing 调参/伺服 A/B，都需要这个入口。

---

## 二、规划设计

1. **默认值不变**：`kivo.drift_band_ms` 不传 = 5 ms，行为与现状逐位一致。
2. **必须小于门限**：`band >= limit` 会让 `target = limit − band` 变负，构造时 clamp 掉。
3. **只用于对照实验**：不出版到证据（`drift_gate` 块已有 `limit_ns`/`band_ns`，
   但那是 `PacePolicy` 的静态值；本轮不扩 schema）。
4. **改 5 处**：Kotlin（`PlayerActivity` 读参数、`PlaybackSession`/`NativePlayer` 传参）、
   JNI bridge、`jni_session_handle`。
5. 只操作 `dd137cf7`；`b6e950ee` 不碰。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/198-p16-p1b17-band-param-plan.md` | 新增 | 本卡规划、判据、结果、结论 | 不复制 `197` 全文 |
| `android/.../ui/PlayerActivity.kt` | 改 | 读 `kivo.drift_band_ms`，传给 `configurePacing` | 不改默认 5 |
| `android/.../bridge/PlaybackSession.kt` | 改 | `configurePacing` 加参数并转发 | 不改其它调用 |
| `android/.../bridge/NativePlayer.kt` | 改 | external 声明加参数 | 不改其它 native 声明 |
| `android/.../cpp/jni/jni_player_bridge.cxx` | 改 | `nativeSessionSetPacing` 加参数 | 不改异常边界 |
| `android/.../cpp/jni/jni_session_handle.{hpp,cxx}` | 改 | 设置 `policy.drift_band_ns`，带 clamp | 不改其它 policy 字段 |

新增 1 文件、改 **6** 个既有文件；**无新目录、无 schema 变更**。

### 判据（B1–B3）

| 判据 | 内容 |
| --- | --- |
| B1 | 四组 `max_correction_ns` **精确等于** band（5/10/15/20 ms，容差 ±0.2） |
| B2 | `drift_rebases` 随 band 增大**单调不增**（推演预期 5→3→2→1） |
| B3 | 四组都 `frames_rendered=1050`、`dropped=0`、`completed=true`（调参不破坏播放） |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | NDK clang `-fsyntax-only`（JNI 两 TU）；`verify_p16_p0_gates` | 0 错；G1–G5 全守（bridge 288 行 < 300） |
| L2 | Gradle `assembleDebug` | BUILD SUCCESSFUL |
| L2 | `dd137cf7` 四组（band 5/10/15/20 × 2000 ppm） | B1–B3 全过 |
| L3 | 六个门禁 | 全 PASS |

---

## 五、收口（2026-11-06）

### 交付

新增 1 文件（`198` 文档），改 6 个既有文件（`PlayerActivity.kt`、`PlaybackSession.kt`、
`NativePlayer.kt`、`jni_player_bridge.cxx`、`jni_session_handle.hpp/.cxx`）。
无新目录、无 schema 变更、默认值不变。

### 实测（`dd137cf7`，2000 ppm，四组）

| band | `rate_ppm` | `drift_rebases` | `max_correction` | `corrected_total` | 帧数 / 丢帧 |
| --- | --- | --- | --- | --- | --- |
| 5 | 2008.4 | **5** | **5.1 ms** | 25.3 ms | 1050 / 0 |
| 10 | 2012.4 | **3** | **10.1 ms** | 30.2 ms | 1050 / 0 |
| 15 | 1995.3 | **2** | **15.1 ms** | 30.1 ms | 1050 / 0 |
| 20 | 2006.8 | **2** | **20.1 ms** | 40.1 ms | 1050 / 0 |

### 判据核对

| 判据 | 结果 |
| --- | --- |
| B1（`max_correction` = band，±0.2） | **PASS**：5.1 / 10.1 / 15.1 / 20.1 |
| B2（`rebases` 单调不增） | **PASS**：5 → 3 → 2 → 2（推演预期 5→3→2→1，实测末段持平） |
| B3（1050 帧、0 丢帧、completed） | **PASS**：四组全过 |

### 结论

- **推演的核心假设被证实**：单次修正量精确等于 `band`，`rebases` 随 band 递减。
- 但**修不完**：band=20 时 `corrected_total` 40.1 ms，仍小于累积 70 ms；
  残差 = `limit − band` = 25 ms，比现状 40 ms 好，但没有消除。
- **0 丢帧**说明"一次修 20 ms"在这台设备上没造成可见丢帧 —— 这是推演里
  最不确定的一点，实测给出了正面答案。
- 所以"调 band"是一个**可用的缓解**（残差 40→25 ms、触发 5→2 次），
  但仍不是根治；根治要改速率而不是改位置。

### 一处编译坑（值得记）

`PlaybackSession.kt` 多传参数后，Kotlin 报 `Too many arguments` ——
`NativePlayer.kt` 的 `external fun` 声明没同步。**JNI 改签名是 4 处联动**
（Kotlin 调用方、external 声明、bridge 实现、native 实现），漏一处就编译失败。

### 未做

不改默认值、不改 `drift_limit_ns`、不换伺服、不出版 band 到证据、
不碰 `pace_stats.*`、不碰 `F:\kivo`、不 `uninstall`。

---

## 六、明确不做

- 不改 `PacePolicy` 默认值、不改 `drift_limit_ns`、不换伺服；
- 不出版 band 到证据（schema 不动）；
- 不碰 `pace_stats.*` / `av_sync_report.cxx`；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`、`b6e950ee` 不碰。
