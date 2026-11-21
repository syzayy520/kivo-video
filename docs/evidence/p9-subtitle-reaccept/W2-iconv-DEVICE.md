# W2 · iconv 预演构建 · 结论（2026-11-22）

目的：在**不动产品**的前提下（独立构建目录 `build-28iconv`、不改 minSdk、不覆盖 `prebuilt/`），
验证"抬到 API 28 + `-Diconv=enabled` 就能解 GBK"这个假设是否成立。

## 1. 结论：**不成立**（两条硬事实，来自 meson 真实日志）

```
Run-time dependency iconv found: NO  (tried builtin and system)
..\mpv-build\mpv\meson.build:647:8: ERROR: Dependency "iconv" not found (tried builtin and system)
```
链接测试原文：
```
.../aarch64-linux-android26-clang.cmd ... testfile.c ... -liconv ...
ld.lld: error: unable to find library -liconv
```

1. **Android 没有独立的 `libiconv`**：它的 iconv 在 **libc** 里（API 28+）。而 mpv 的 meson
   检测方式去找 `-liconv` ⇒ 必然失败。**所以 `-Diconv=enabled` 在 Android 上不可用**。
2. **cross file 里 API 是硬编码 26**（`aarch64-linux-android26-clang.cmd`，见 `W2-cross-file.txt` 第 20–21 行）。
   我在脚本里 export 的 `KIVO_API=28 / KIVO_TRIPLE=…28` **不生效** —— 要抬 API 必须改 cross 文件那两行。

⇒ 254 草案里"minSdk 28 即可用系统 iconv"这条**被证伪**；minSdk 与 GBK 并不是同一因果的两端，
而是"抬 minSdk" **还不够**，还得解决 libiconv。

## 2. 真实代价（更新 254 §6 的选项）

| 方案 | 要做的事 | 代价 |
| --- | --- | --- |
| A. 自带 libiconv | 交叉编译 GNU libiconv 静态库（arm64，API 28）→ 让 meson 找到（`-liconv` 或 pkg-config）→ 改 cross 文件 2 行 → 重编 libmpv → stage → 重跑 HDR/4K 回归 | 中高：多一条三方库依赖 + 一次完整重编 |
| B. 给 mpv 打补丁 | 改 `meson.build:647` 的 iconv 检测（把 bionic 的 libc iconv 当 builtin 接受）→ 其余同上 | 中：要维护第三个 patch（现有两个 patch 的邻居） |
| C. 不做 | GBK 字幕继续靠用户外部转 UTF-8 / 手选 codepage（而 codepage 同样依赖 iconv） | 零 |

**注意**：无论 A 还是 B，`W3-R1（sub-codepage）`都仍然**依赖这次重编**，不会绕过。

## 3. 未取证

1. 没有实际尝试 A/B（本轮只到"检测失败"这一步，未编 libiconv、未改 meson.build）。
2. 没验证"抬到 API 28 后现有 HDR/Surface patch 是否仍能编过"（被 iconv 检测挡在前面，尚未到达该阶段）。
3. 没验证 API 28 的 `libmpv.so` 在 K40 上能否正常出画面（同上）。

## 4. 留档资产

- `android/app/buildscripts/mpv/build-libmpv-28iconv.sh`（预演脚本，头部已写"已试、失败原因"）
- `W2-iconv-probe.log`（meson 检测原文）、`W2-cross-file.txt`（API 硬编码证据）
- 预演目录 `build-28iconv/` **已删除**（仅 120 K 中间产物），现有 `build/` 与 `prebuilt/` **未改动**。

## 5. 收尾

未 commit / push；APK 未改动（仍是上一版）。
