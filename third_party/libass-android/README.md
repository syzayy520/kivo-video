# libass-android（212 字幕进渲染的依赖链交叉编译）

## 这是什么

把字幕渲染链交叉编译成 Android **arm64-v8a 静态库**：
**freetype 2.13.3**（栅格化，FTL 许可）→ **fribidi 1.0.16**（双向文本，LGPLv2.1）→
**libass 0.17.3**（ASS/SSA/SRT 解析+光栅化，ISC 许可）。

- 源码：各上游 GitHub tag，tarball 下载到 `/c/re-tools/libass-src/`（走 v2rayN 代理 10808）。
- 运行：`/c/msys64/usr/bin/bash -lc 'bash third_party/libass-android/build.sh'`
  （MSYS2 bash——make/sh 同运行时，32K 教训同 ffmpeg-android）。
- 产物：`/c/re-tools/libass-android/dist/`（**无空格路径**，同款理由）。
- **不编 harfbuzz**：libass `--disable-harfbuzz` 降级为简单布局——拉丁/中文正常，
  阿拉伯/印度等复杂文字系统留后续卡（诚实清单）。fontconfig/expat 不编，
  字体走 `ass_add_font` 内存注入（设备 `/system/fonts/` 运行时读取）。

## 依赖顺序（强约束）

freetype 必须先装（libass 经 pkg-config 发现它）；PKG_CONFIG_PATH/LIBDIR 锁死在
dist 内，防止宿主 pkg-config 污染交叉编译。

## 许可证合规

libass ISC、freetype FTL、fribidi LGPLv2.1（静态链接 → 提供 relink 手段：
本目录 build.sh 即完整重建脚本 + 上游源码链接，与 ffmpeg-android README 同款声明）。
