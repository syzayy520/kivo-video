# P16-P1B4 · 交接可复算性：解除本机路径绑定

- 日期：2026-11-06
- 定位：本卡修 `181`–`186` 期间新增脚本里一个**真实的交接缺陷**。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`AGENTS.md` §0.3「换机器必补三样」、`03-goals.md` G10（证据要能被第三方复算）、`08-p12-family-tree.md`。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照，不在其中写文件；无新目录、无项目包。

---

## 一、构思

`AGENTS.md` §0.3 明确写着：`android/local.properties`、FFmpeg junction、adb 授权这三样
"绑定本机"，换机器后代码是全的、构建却起不来。
`186` 新增的 `tools/run_p16_burst_behavior_test.py` 又添了第四样：

```python
ADB = Path("C:/Android/sdk/platform-tools/adb.exe")
NDK_GLOB = "C:/Android/sdk/ndk/*/toolchains/llvm/prebuilt/windows-x86_64/bin/..."
```

它把 SDK 根目录**和 Windows 的 prebuilt 平台名**一起写死了。
整目录拷到另一台机器（或 Linux/mac runner）后，`--compile-only` 与设备测试都会直接失败，
而失败原因看起来像"NDK 没装"——正是 `AGENTS.md` 反复提醒的那类误导性报错。

复核结论（本轮实扫）：4 个校验器全部用 `Path(__file__).resolve().parent.parent` 定位仓库根，
**路径无关**；只有行为测试脚本这 2 行是真绑定。历史 `tools/*.py` 共 30 份、其中 5 份带硬编码路径，
按项目惯例**只统计、不判红**（豁免明示）。

---

## 二、规划设计

1. **SDK 解析三级优先级**：
   `ANDROID_HOME` / `ANDROID_SDK_ROOT` → `android/local.properties` 的 `sdk.dir` → PATH 里 `adb` 上溯两级。
   每一级都打印**来源**，换机器排错时能一眼看出用的是哪个。
2. **prebuilt 平台也参数化**：glob 用 `prebuilt/*/bin/...` 覆盖
   `windows-x86_64` / `linux-x86_64` / `darwin-x86_64`；wrapper 后缀同时兼容 `.cmd` 与无后缀。
3. **门禁只判 P16 新增脚本**：历史脚本的硬编码路径是既有事实，不改、不判红，只在报告里给数字。
   这不是放宽判据，是让判据有明确作用域。
4. **不进 CI 的部分如实说**：可移植性门禁是纯静态检查，进 CI；行为测试仍需真机，继续不进 CI。
5. **不碰他人主线**：`backend/CMakeLists.txt`、`docs/handoff/*`、`docs/evidence/*` 等一律不动。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/187-p16-p1b4-portability-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `181–186` 全文 |
| `tools/verify_tools_portability.py` | **新增** | 钉住 P16 脚本不得硬编码盘符 / SDK 路径，带变异自检 | 不改历史脚本、不判历史脚本红、不替代其它门禁 |
| `tools/run_p16_burst_behavior_test.py` | 改 | SDK 三级解析 + 跨平台 NDK glob + `--self-check` | 不改 T1–T3 断言、不改设备白名单 |
| `.github/workflows/p2-ci-build-test.yml` | 改 | `hard-gates` 增加该门禁一步 | 不动其它 job、不加 `continue-on-error` |

新增 2 文件、改 2 个既有文件，全部落在既有族；**无新目录**。

### 判据（P1–P3）

| 判据 | 内容 |
| --- | --- |
| P1 | P16 新增的 6 个 Python 脚本里不得出现硬编码盘符 / SDK / FFmpeg 绝对路径（精确匹配，避免把 `:\s` 之类正则转义误判） |
| P2 | 行为测试脚本通过 `resolve_sdk_root()` 从三个来源解析，含 `--self-check`，且 NDK glob 覆盖 `prebuilt/*` |
| P3 | CI `hard-gates` 含该门禁且该段无 `continue-on-error` |

变异自检：注入一行硬编码 SDK 路径、删掉 `resolve_sdk_root`、给 CI 加回 `continue-on-error`，各自必须被抓到。

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | `python tools/verify_tools_portability.py --self-test` + 正式跑 | 变异全 CAUGHT、正式 PASS |
| L1 | 五个门禁全量复跑 + 行为测试 `--compile-only` | 全 PASS；CI `hard-gates` 6 步 |
| L2 | `python tools/run_p16_burst_behavior_test.py --self-check` | 打印 SDK root 与来源；adb / compiler 都能找到 |
| L2 | **覆盖验证**：用环境变量把 `ANDROID_HOME` 指到同一个 SDK 再跑 `--self-check` | 来源变成 `env:ANDROID_HOME`，仍解析成功；证明不依赖写死的路径 |
| L3 | `python tools/run_p16_burst_behavior_test.py --device dd137cf7` | T1/T2/T3 仍全 PASS（`186` 的行为结论不变） |

---

## 五、收口（2026-11-06）

### 交付

新增 **2** 个文件（`187` 文档、`tools/verify_tools_portability.py` 156 行），
改 **2** 个既有文件（`run_p16_burst_behavior_test.py` 263 行、CI yml）。
无新目录、无项目包；CI `hard-gates` 现 **6 步**、仍无 `continue-on-error`。

### 实现

- 行为测试脚本删掉两行硬编码，改为 SDK 三级解析：
  `ANDROID_HOME` / `ANDROID_SDK_ROOT` → `android/local.properties` 的 `sdk.dir` → PATH 里 `adb` 上溯两级；
  NDK glob 改为 `prebuilt/*`（覆盖 `windows-x86_64` / `linux-x86_64` / `darwin-x86_64`），
  wrapper 后缀兼容 `.cmd` 与裸脚本；新增 `--self-check` 打印 SDK root / 来源 / adb / compiler。
  **T1–T3 断言与 `dd137cf7` 硬白名单逐字未改。**
- 可移植性门禁 P1–P3 + 3 条变异；判红范围只限 P16 的 6 份脚本，历史脚本只统计不判红。

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | 5 个门禁 self-test + 正式跑**全 PASS**；新门禁 baseline 0、3 变异全 CAUGHT；CI 6 步 |
| L2 | `--self-check` 默认 `source=android/local.properties`、`sdk=C:\Android\sdk`，adb/compiler 均找到，`RESULT: PASS` |
| L2 | **覆盖验证**：`ANDROID_HOME=C:/Android/sdk` 后 `source=env:ANDROID_HOME`，仍 PASS —— 证明不依赖写死路径 |
| L2 | `--compile-only`：9 个 TU 编译 OK |
| L3 | `--device dd137cf7`：六项断言全 PASS（`RESULT: PASS failures=0`）；白名单仍拒绝 `b6e950ee` |

### 口径说明

复核时用宽松正则（不限引号）统计"历史 `tools/*.py` 带硬编码路径"得 **5** 份，其中含
`:\s` 这类正则转义误报；正式门禁改成"引号内成形的路径字面量"精确匹配后为 **1** 份
（`sample_p12d3_mem.py`），只统计、不判红。两个数字口径不同，不是结论变化。

### 如实说明

- 行为测试仍**不进 CI**（ubuntu runner 无真机），静态可移植性门禁进 CI。
- 本卡**没有证明"在另一台机器上真的能跑"** —— 本机只有一套 SDK/NDK。
  证的是"路径来源不再写死、可被环境变量覆盖且 `--self-check` 能自证"；
  真正跨机复算仍需整目录拷到另一台机器执行 `--self-check` + `--compile-only`。
- `183` §七 评审门闸仍关着；A1 换控制律 / A2 ticket 排空未开工。

### 未做

历史脚本既有的硬编码路径不改；不实现 `183` §五；不改证据 schema、不调阈值、
不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不改历史 `tools/*.py` 里既有的 5 处硬编码路径（只统计）；
- 不把行为测试塞进 CI（无真机 runner）；
- 不实现 `183` §五 的实现卡（`183` §七 评审门闸仍关着）；
- 不改证据 schema、不调阈值、不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 不碰。
