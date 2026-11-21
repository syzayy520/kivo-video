# Wave2 · minSdk 26→28 + libmpv 重编（iconv / uchardet）· 家族树草案

> 状态：**仅设计，未实施**（2026-11-22）。需要用户拍板两件事（见 §6）才能进入实现。
> 本文件回答三件事：为什么两件事是同一件事、要动哪些文件、动完怎么验。

## 1. 结论先行：这不是两件事

交接里一直并列写着「minSdk 26→28」与「libmpv 重编 `-Diconv/-Duchardet`（GBK 乱码）」。
实测 NDK 头文件后可以确认：**它们是同一因果链的两端**。

**证据（原文，非记忆）**：`C:/Android/sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/iconv.h`

```
61: * Available since API level 28.
64: #if __ANDROID_API__ >= 28
65: iconv_t iconv_open(...) __INTRODUCED_IN(28);
76: size_t  iconv(...)       __INTRODUCED_IN(28);
86: int     iconv_close(...) __INTRODUCED_IN(28);
87: #endif /* __ANDROID_API__ >= 28 */
```

也就是说：**Android 的 bionic 从 API 28 才有 `iconv`**。libmpv 要解 GBK 字幕必须能用 iconv；
因此编译期 `__ANDROID_API__` 必须 ≥ 28；因此 app 的 `minSdk` 必须 ≥ 28。

> ⚠️【2026-11-22 预演构建更正】**抬 minSdk 还不够**。真实 meson 日志（`docs/evidence/p9-subtitle-reaccept/W2-iconv-DEVICE.md`）：
> `-Diconv=enabled` 直接失败于 `Dependency "iconv" not found` / `ld.lld: error: unable to find library -liconv`
> —— Android 的 iconv 在 **libc** 里、**没有独立 libiconv**，而 mpv 的检测只认 `-liconv`。
> 另外 `cross-android-arm64.txt` 里 API 是**硬编码 26**（`aarch64-linux-android26-clang.cmd`），
> 脚本里的 `KIVO_API`/`KIVO_TRIPLE` 覆盖不生效。
> ⇒ 只有两条路：① 自带 GNU libiconv 静态库；② 给 mpv 打补丁改 iconv 检测。见 254 §6（已更新）。

> 反面推论（同样重要）：**只要坚持 minSdk 26，就不可能在 libmpv 里用系统 iconv**，
> 唯一替代是自带一份 libiconv 静态库 —— 那会把"抬 minSdk"换成"多背一个三方编码库 + 包体"。

## 2. 现状（实测）

| 项 | 现状 | 位置 |
| --- | --- | --- |
| `minSdk` | **26**（compileSdk/targetSdk 37） | `android/app/build.gradle.kts:15` |
| libmpv 形态 | **预编译 `.so`，不进版本库** | `android/app/src/main/cpp/mpv/prebuilt/arm64-v8a/`（libmpv + libav*.so + include） |
| 重建脚本 | 存在，可重建 | `android/app/buildscripts/mpv/{build-libmpv.sh, stage-libmpv.sh, toolchain.env, cross-android-arm64.txt}` |
| 源码改动 | 两个 patch 脚本（ES3 / HDR-Surface） | `patch-mpv-es3.py`、`patch-mpv-hdr-surface.py` |
| meson 开关 | **`MESON_OPTS` 里没有 iconv / uchardet** | `build-libmpv.sh:76-88` |
| 28 的现有用法 | 只 1 处且有版本判断 | `ui/compose/player/PlayerWindow.kt:60`（`VERSION_CODES.P`） |

## 3. 文件家族（要动的落点，按变化原因分层）

```
android/app/buildscripts/mpv/
├── build-libmpv.sh          ← R1/R2 的唯一改动点（meson 开关）
├── stage-libmpv.sh          ← 不变（把产物 stage 到 prebuilt/ + jniLibs/）
└── patch-mpv-*.py           ← 不变，但**必须重新应用**（见 §5 风险 2）

android/app/
├── build.gradle.kts         ← R3（minSdk 26→28）
└── src/main/cpp/mpv/prebuilt/arm64-v8a/   ← 重建产物落位（不入库）

android/app/src/main/jniLibs/arm64-v8a/    ← stage 的副本（随构建）
```

**没有新增代码文件**：本项是"改 1 个开关 + 改 1 个数字 + 重建产物"，不产生新模块。
因此**不需要新的家族树分支**，只需在 `253-mpv-kernel-family-tree.md` 的「构建入口」一节补一行链接到本文件。

## 4. 需求（EARS）

- **R1**：When libmpv 需要解非 UTF-8 字幕（GBK/GB18030/BIG5），the 内核 shall 能用系统 `iconv` 完成编码转换。
  - 实现：`MESON_OPTS` 增 `-Diconv=enabled`（显式，不依赖 auto 探测）。
- **R2（可选）**：When 字幕文件编码未声明，the 内核 shall 能自动探测（否则只能靠用户手选 codepage）。
  - 实现：`-Duchardet=enabled`。**代价**：需要 uchardet 源码并入交叉编译 → 包体增加、构建链多一条依赖。
- **R3**：the app shall 以 `minSdk = 28` 构建（compileSdk 37 / targetSdk 37 不变）。
- **R4**：When 重建产物落到 `prebuilt/`，the 现有能力 shall 不回归（HDR 标记、Surface 呈现、1080p10/4K 帧率）。

## 5. 风险（都来自实测到的结构，不是猜测）

1. **放弃 Android 8（API 26/27）机型**。这是产品决策，不是技术决策 —— 需要你拍板。
2. **两个 patch 必须重新应用**：`patch-mpv-es3.py` / `patch-mpv-hdr-surface.py` 改的是 mpv 源码；
   重建若忘了打，P12-E2/E3（HDR 标记下发、dataspace）会静默回退 —— 而这两项的验收证据是现成的，
   重跑一遍即可发现（这也是为什么必须先跑 §7 的回归）。
3. **包体**：iconv 走系统 **不增**；uchardet 会增至多 ~几百 KB（按并入的实现而定）。
4. **预编译产物不入库**：换机器/清 `build/` 后必须重跑 `build-libmpv.sh`，否则链接失败
   （与 `libsmb2-android` 同一类坑，AGENTS.md §0.3 已登记）。

## 6. 需要你拍板的两件事

| # | 问题 | 选项 |
| --- | --- | --- |
| A | 接受 minSdk 28（放弃 Android 8）吗？ | ① 接受，只上 `-Diconv`（推荐：代价最小、直接解 GBK）<br>② 不接受，改自带 libiconv 静态库（要评估许可与包体）<br>③ 先不做，GBK 字幕继续让用户手选 codepage |
| B | 要不要 `-Duchardet`（自动探测编码）？ | ① 不要（先解 GBK 即可）<br>② 要（多一条依赖 + 包体） |

## 7. 验收判据（实现后必须全过）

1. **GBK 字幕**：用一份 GBK 编码的 `.srt`（设备侧可直接用 `iconv -f utf-8 -t gbk` 造）挂载，字幕文字**无乱码**。
   反证：同一文件在旧内核上必须复现乱码（证明差异来自本次改动）。
2. **编码标示**：`--es kivo.subWrite` 探针读回 `options/sub-codepage` 或 track 的 `codec`/charset 字段，与预期一致。
3. **能力不回归**：重跑 P12-E2/E3（HDR 标记 + dataspace）与 P13-A（4K60 HEVC10 帧率），
   与现有证据比对（帧率落在原区间、`delivered_color_*` 仍 6/6/2）。
4. **包体**：`assembleRelease` 的 APK 体积增量 ≤ 1%（记录改前改后数字）。
5. **构建可复现**：删 `prebuilt/` 后按脚本重建一次，能编过并装机。

## 8. 不做

- 不做 sub-codepage 的 UI（Wave3 范围）。
- 不碰 `playback_graph` / SSA 字体（Wave3）。
- 不动 `PlayerPanelStyle` / 面板材质（已完成 v3 + backdrop，见 p10 §16/§18）。