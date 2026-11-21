# W2 · iconv 落位（libmpv 重编 + minSdk 28）· 2026-11-22

> **状态：已落位、二进制已验证；真机验证未完成**（测试机 `b6e950ee` 在构建结束时掉线，
> `adb devices` 列表为空；另一台 `dd137cf7` 属其他助手，按铁律未做任何操作）。
> 下面把"已验证"与"待验证"分开写清楚，**不把未验证当完成**。

## 1. 路线决策（为什么不用 GNU libiconv）

| 选项 | 判定 |
| --- | --- |
| GNU libiconv | **不采用**：LGPL，静态链进闭源 APK 有重新链接义务 |
| **bionic libc iconv（BSD/Apache）** | **采用**：API 28+ 完整支持 GBK/GB18030/BIG5/Shift-JIS/EUC-* |

上游 mpv 的探测方式是 `dependency('iconv')` → 最后一招链 `-liconv` → Android 必然失败
（`ld.lld: error: unable to find library -liconv`，见 `W2-iconv-probe.log`）。

## 2. 落位清单

| 文件 | 改动 |
| --- | --- |
| `buildscripts/mpv/patch-mpv-iconv-android.py`（**新增**） | 只在 `android` 上追加一次**真实转换**的链接测试：GBK「中」(0xD6 0xD0) → UTF-8；通过即 `declare_dependency()`；`-Diconv=enabled` 但测试失败时 `error()` 明确失败（不是静默降级） |
| `buildscripts/mpv/build-libmpv.sh` | MESON_OPTS 加 `-Diconv=enabled`；打第三个 patch；接入 python3 shim |
| `buildscripts/mpv/toolchain.env` | `KIVO_API=28`、`KIVO_BUILD=…/build-28`（与 API 26 的旧 `build/` 分离，避免混编） |
| `buildscripts/mpv/cross-android-arm64.txt` | clang wrapper `…android26-clang` → `…android28-clang` |
| `android/app/build.gradle.kts` | `minSdk = 28` |
| `cpp/mpv/prebuilt/arm64-v8a/libmpv.so` + `jniLibs/arm64-v8a/libmpv.so` | 换成本次产物（32 818 904 B）；旧包备份为 `libmpv.so.orig-26` |

## 3. 已验证（二进制层，硬证据）

```
llvm-readelf -s libmpv.so  |  UND iconv_open@LIBC_P
                                UND iconv@LIBC_P
                                UND iconv_close@LIBC_P
                             LOCAL mp_iconv_to_utf8
llvm-readelf -d libmpv.so  |  NEEDED 里 **没有 libiconv.so**
llvm-readelf -n libmpv.so  |  Android note: r30 / 16248370（NDK 30）
```

⇒ 产物**真的**通过 bionic libc 的 API-28 iconv 符号做转码，且没有引入任何新 .so。
APK `assembleDebug` = `BUILD SUCCESSFUL in 4m 49s`。

## 4. 环境坑（已修，留档）

1. **Windows Store 的 `python3.exe` 是坏存根**（指向不存在的 `pythoncore-3.14-64`），
   meson 的 `project()` 调它 ⇒ `status 3`。修法：项目内 `.codeartsdoer/toolchain-shim/python3.exe`
   （真正的 Python310 副本），脚本里前置到 PATH。**不动系统**。
2. `KIVO_BUILD` 定义在 `toolchain.env`（不是 `build-libmpv.sh`）——第一次改错了文件。

## 5. 待验证（设备回来后必须逐条做）

| # | 项 | 判据 |
| --- | --- | --- |
| 1 | **装机**（minSdk 28） | `adb install -r` 成功；`dumpsys package` 的 minSdk 显示 28 |
| 2 | **GBK 字幕正确** | 设备上已有 `/sdcard/Movies/Kivo/GBK-test.srt`（GBK 编码，内容「中文字幕 GBK 转码测试」）；`--es kivo.subWrite "sub-codepage=GBK"` 后截图，文字必须正确、不是乱码 |
| 3 | **反证**（最强验证） | 把 `libmpv.so.orig-26` 换回、重编一份旧包，同样设 `sub-codepage=GBK` ⇒ 应当乱码/失败。两者对比才证明差异来自本次改动 |
| 4 | **能力不回归** | 重跑 P13-A（4K60 HEVC10 帧率）与 P12-E2/E3（HDR 标记 / dataspace），与既有证据比对 |
| 5 | **HDR 补丁仍在** | 新产物已含 `patch-mpv-es3.py` + `patch-mpv-hdr-surface.py`（脚本顺序保证），但需真机确认 E2/E3 输出不为空 |

## 5.1 编码注入通道：三条实测约束（W3-R1 的设计输入，**本轮新增**）

| 尝试 | 命令 | 结果 |
| --- | --- | --- |
| ① 当属性写 | `setProperty("sub-codepage", "GBK")` | **ok=false** —— 它是 demuxer 局部选项（`demux_lavf.c:105 OPT_STRING(sub_cp)`），不在属性表 |
| ② 当 per-file option | `loadfile <url> replace 0 "sub-codepage=GBK,start=2"` | **ok=true 但无效**：只作用于**主文件** demuxer；`sub-auto=exact` 加载的**外部字幕**用独立 demuxer，不继承 ⇒ 截图仍是乱码（字幕区亮像素 4587，与改前逐像素相同） |
| ③ 运行时选项命名空间 | `setProperty("options/sub-codepage", "GBK")` | **ok=true**，但已加载字幕**不会重解**；且 mpv **没有 `sub-reload` 命令**（`input/cmd_list.c` 里查无此命令） |

**结论**：`sub-codepage` 必须在 **demuxer 创建之前**生效。可行路径只有两条：
- **A**（初始化选项）：把 `sub-codepage` 作为 `mpv_set_option_string` 传进 `apply_mpv_options`
  → 需要打通 JNI 6 层（hpp/cpp/session/jni/Kotlin create/StartSequence）；
- **B**（重建 demuxer）：先写 `options/sub-codepage`，再 `removeExternalSubtitle` + `addExternalSubtitle`
  让外挂字幕重建 —— 不用改 native，但会打断当前字幕轨（需重新选中）。

两条都属 **W3-R1（sub-codepage UI）**；届时"改编码"在产品上必然表现为一次"重载字幕"（B）或"重开会话"（A）。

## 6. 回滚（一句话）

```bash
cp android/app/src/main/cpp/mpv/prebuilt/arm64-v8a/libmpv.so.orig-26 \
   android/app/src/main/cpp/mpv/prebuilt/arm64-v8a/libmpv.so
cp android/app/src/main/cpp/mpv/prebuilt/arm64-v8a/libmpv.so.orig-26 \
   android/app/src/main/jniLibs/arm64-v8a/libmpv.so
# 再把 minSdk 改回 26、KIVO_API/cross 改回 26（本次改动都在工作区，未提交）
```

## 7. 收尾

- 仓库未 commit / push；改动全在工作区（可 `git checkout` 回滚）。
- `build-28/` 产物 32 818 904 B（与落位文件同一份）。
- 预演临时资产（`build-libmpv-28iconv.sh`、`cross-android-arm64-28.txt`、`build-28iconv/`）已清理。
