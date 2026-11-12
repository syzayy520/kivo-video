#!/usr/bin/env python3
"""D6 长期代码参考产品名清理：精确改写映射 + 前后快照 + 语义判据复算。

在仓库根 C:/kivo video 下运行：
  python docs/evidence/p260/code-derex/derex_apply.py snapshot before
  python docs/evidence/p260/code-derex/derex_apply.py apply
  python docs/evidence/p260/code-derex/derex_apply.py snapshot after
  python docs/evidence/p260/code-derex/derex_apply.py verify

判据（用户 2026-11-13 拍板：语义判据 + 目录全量）：
  * 产品名 / 出处署名在长期代码中零残留；
  * 子串误报（ShaderExecution / barExit / Execution）与冻结参考路径列入白名单；
  * 扫描范围按目录全量，含 CMakeLists.txt 与 .cxx，排除冻结归档目录。
"""
from __future__ import annotations

import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[4]
EVID = ROOT / "docs/evidence/p260/code-derex"

SCAN_ROOTS = ["backend", "include", "android/app/src/main", "tools"]
SCAN_EXTS = {".kt", ".cpp", ".cxx", ".hpp", ".h", ".py", ".txt", ".json"}
EXCLUDE_PREFIXES = ("tools/reference/", "docs/")
# 清理前基线固定为 D6 开工时的提交；不随 HEAD 漂移（审计发现并发提交会污染基线）。
BASELINE_COMMIT = "f97dd2ea"

HIT = re.compile(r"rex|senplayer", re.I)
# 排除路径中的 rex 子串：\bRex\b 后不得紧跟 - 或 /（冻结参考目录/归档文件名）。
PRODUCT = re.compile(
    r"REX[ -]?\d|REX-EMBY|RexDesign|SenPlayer|"
    r"\bRex\b(?![-/])|rex_browse|measure_rex|rex-replication|rex-playback|rexTextWeight",
    re.I,
)
WHITELIST = re.compile(
    r"ShaderExecution|barExit|Execution|docs/reference/rex-teardown|"
    r"artifacts/rex-measurements\.txt|tools/reference/rex|rex-measurements|"
    r"236-ui-align-rex-design\.md",
    re.I,
)
# 宽帧号口径：0160–0179 整段（审计指出原集合漏 0170）。
FRAME_IDS = re.compile(
    r"M0[38]-S\d+|\bsk_\d+\b|IMG_0161|frames2/|\b0(?:16\d|17\d)\b",
    re.I,
)

# (相对路径, 原文片段, 新文片段, 期望命中次数)。全部为单行片段，逐条唯一命中断言。
RULES: list[tuple[str, str, str, int]] = [
    # ---- android/app/src/main/cpp（D6 字面扩展名会漏掉的两个文件，按目录全量补上）----
    ("android/app/src/main/cpp/CMakeLists.txt", "；SenPlayer 同款库）", "）", 1),
    ("android/app/src/main/cpp/jni/jni_smb2_bridge.cxx",
     "（220 定案：与 SenPlayer 同款库同形态；250 做健壮性收口）", "（220 定案；250 做健壮性收口）", 1),
    ("android/app/src/main/cpp/mpv/mpv_commands.cpp",
     "（与 Rex 的 `seek.accurate` 开关同一思路）", "（沿用 `seek.accurate` 开关思路）", 1),

    # ---- app：下载 / Emby / 内核 / 网络 ----
    ("android/app/src/main/java/com/kivo/cinema/download/DownloadService.kt",
     " * SenPlayer 同款 DownloadService 对位：", " * 下载服务对位：", 1),
    ("android/app/src/main/java/com/kivo/cinema/emby/EmbySessionModel.kt",
     "（REX 首页横排）", "（首页横排）", 1),
    ("android/app/src/main/java/com/kivo/cinema/emby/EmbyCatalog.kt",
     "。REX 首页把每个 View", "。首页把每个 View", 1),
    ("android/app/src/main/java/com/kivo/cinema/emby/EmbyShows.kt",
     " * REX 剧详情", " * 剧详情", 1),
    ("android/app/src/main/java/com/kivo/cinema/emby/ui/EmbyBrowseScreen.kt",
     " * 255：REX 首页一比一——", " * 255：首页按规格一比一——", 1),
    ("android/app/src/main/java/com/kivo/cinema/mpv/LocalRangeServer.kt",
     "mpv（也就是 SenPlayer / Rex 用的那颗内核）", "mpv 内核", 1),
    ("android/app/src/main/java/com/kivo/cinema/playback/MpvPlaybackSession.kt",
     "mpv（SenPlayer / Rex 用的那颗）", "mpv", 1),
    ("android/app/src/main/java/com/kivo/cinema/playback/PlaybackEngine.kt",
     "全 FFmpeg 解码覆盖，", "全 FFmpeg 解码覆盖。", 1),
    ("android/app/src/main/java/com/kivo/cinema/playback/PlaybackEngine.kt",
     " *    与 SenPlayer / Rex 同路。", " *    单一 demux 线程，单一读路径。", 1),
    ("android/app/src/main/java/com/kivo/cinema/net/LocalRangeProxy.kt",
     "）——SenPlayer 架构对位", "）", 1),
    ("android/app/src/main/java/com/kivo/cinema/smb/discovery/SmbDiscovery.kt",
     "Rex 的\"可用地址\"里也只有 N1", "实测\"可用地址\"里也只有 N1", 1),

    # ---- app：详情 / 设计 token ----
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailCalendarGrid.kt",
     "（REX 帧复刻）：", "：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonPicker.kt",
     "（REX-EMBY 0178/0179）：", "：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonPicker.kt",
     "播出日历条（REX）：", "播出日历条：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonMenu.kt",
     "（REX-EMBY 0178/0179 帧实测）", "（帧实测）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonMenu.kt",
     "（REX 不压暗背景）", "（不压暗背景）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailPage.kt",
     "the captured Rex scroll sequence", "the captured scroll sequence", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailCalendarSheet.kt",
     "（REX 帧复刻）：", "：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/actions/DetailMorePopup.kt",
     "（REX 帧复刻，**不是** TMDB/重新加载）：", "（**不是** TMDB/重新加载）：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/design/DesignTokens.kt",
     "（RexDesign Assets.car 硬参数，浅色态；深色态 v2）", "（浅色态实测硬参数；深色态 v2）", 1),

    # ---- app：播放页 compose 外层 ----
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "（REX 的 \"4K\" 位置）", "（\"4K\" 位置）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "（REX 0172 保留 4K 标）", "（实测保留 4K 标）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "（形态4，REX 0162 锚点；不是均衡器）", "（形态4；不是均衡器）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "（形态13，REX 0172）：", "（形态13）：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "（REX 入口未录到，见设计稿 §六）", "（入口未录到，见设计稿 §六）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "双击/长按通道保留（M03-S07）。", "双击/长按通道保留。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "/** 轻提示：1.8s 后自动消失（REX M08-03 停留 1.0–2.0s）。 */",
     "/** 轻提示：1.8s 后自动消失（落在 1.0–2.0s 停留区间）。 */", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerComposeRoot.kt",
     "：REX 的弹幕只压画面", "：弹幕只压画面", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerSubtitleImport.kt",
     "导入即启用（REX 行为）", "导入即启用", 1),

    # ---- app：播放页 player 家族 ----
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerAudioOnly.kt",
     "，REX 0172 复测）", "，真机复测）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerBottomBar.kt",
     "vs REX 304.8pt", "vs 基线 304.8pt", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerBottomBar.kt",
     "（◐ 入口，REX 未录到；见设计稿 §六）", "（◐ 入口，未录到；见设计稿 §六）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureFlash.kt",
     "（REX M03-S01：\"图标闪现 0.5\"，0→1→0）", "（\"图标闪现 0.5\"，0→1→0）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureFlash.kt",
     " * REX 用一个居中的图标", " * 交互反馈用一个居中的图标", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureFlash.kt",
     "（REX 的 easeOut 族）", "（easeOut 族）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "（REX 交互 M03-S01 / M03-S02）", "（单击 / 双击 / 上下滑）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "## 交互链（对齐 REX）", "## 交互链", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "REX\"点屏唤出/再点收起\"", "\"点屏唤出/再点收起\"", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "REX M03-S01 双击热区", "双击热区", 3),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "这正是 REX 文档里\"识别窗 0.3s\"那一条", "这正是\"识别窗 0.3s\"那一条", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "双击热区（REX M03-S01：左 / 中 / 右）", "双击热区（左 / 中 / 右）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphChrome.kt",
     "（REX 0160 实测 ink", "（实测 ink", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyph.kt",
     "## 形状语言（与 REX 一致）", "## 形状语言", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyph.kt",
     "（REX 掩码实测，24 画布）", "（掩码实测，24 画布）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyph.kt",
     "，REX 0160 掩码 1:1；", "，掩码 1:1；", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（对应 REX 顶栏那一排）", "（顶栏那一排）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "## 为什么按 REX 的形状重画", "## 为什么按这套形状重画", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "用户明确要求：\"图标你要对比下 REX 的图标，就知道问题出在哪里了\"。",
     "用户明确要求图标按实测形状对照重画。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "而 REX 用的是一套", "而目标用的是一套", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "| REX 图标 |", "| 图标 |", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（AirPlay 形，0160 掩码 1:1）", "（AirPlay 形，掩码 1:1）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（0166 掩码 1:1）", "（掩码 1:1）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "的混搭，与 REX 一致；", "的混搭；", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "坐标系 24×24，与 REX 的符号比例接近", "坐标系 24×24，与实测符号比例接近", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（音轨，REX 0162）", "（音轨）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（听视频，REX 0172）", "（听视频）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（REX 0160 掩码 1:1）", "（掩码 1:1）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "// 用户严格要求\"和 REX 一模一样\"：REX 的符号是 AirPlay 形",
     "// 用户严格要求形状完全一致：这里的符号是 AirPlay 形", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "符号跟参考，能力跟平台", "符号按形状规范，能力跟平台", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphFeature.kt",
     "（REX 0166 掩码 1:1）", "（掩码 1:1）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphCore.kt",
     "（X，REX 0160 实测 ink", "（X，实测 ink", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphCore.kt",
     "，REX sk_16 实测）", "，实测）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerOverlay.kt",
     "**不替换控制条**（REX 0172）：", "**不替换控制条**：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerOverlay.kt",
     "（REX 0172）⇒", " ⇒", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerMotion.kt",
     "// REX §三：面板弹入", "// 面板弹入", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerMotion.kt",
     "（REX M03-S06：", "（", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerMotion.kt",
     "（REX M03-S03：", "（", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerLockLayer.kt",
     "，REX sk_16 实测）", "，真机实测）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerLockLayer.kt",
     "【入口是 ◐ 推断】REX 的加锁入口没被录到", "【入口是 ◐ 推断】加锁入口没被录到", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelHost.kt",
     "（REX 0171 面板右缘≈倍速右缘）", "（实测面板右缘≈倍速右缘）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelHost.kt",
     "：REX 是原地换内容。", "：原地换内容。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelOcclusion.kt",
     "（v3 §1.4，对照 REX IMG_0161）", "（v3 §1.4）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelOcclusion.kt",
     "）。REX 的磨砂把盖住的字遮死、", "）。目标效果是把盖住的字遮死、", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelOcclusion.kt",
     "，与 REX 相反。", "，与目标行为相反。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelOcclusion.kt",
     " * REX 的真实行为是", " * 真实行为是", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerPanelData.kt",
     "REX 的 13 个面板", "13 个面板", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerSeekTrack.kt",
     "// REX 质感＝", "// 玻璃质感＝", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerSourceOrigin.kt",
     "（v3.1f；REX 0169 源卡显示的是", "（v3.1f；源卡显示的是", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerSeekPreview.kt",
     "（v3 §1.5，REX frames2/sk_8）。", "（v3 §1.5）。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerStatusLayer.kt",
     "（REX 的加载指示）", "（加载指示）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerStatusRow.kt",
     "（REX 的录屏里它们一直在）", "（录屏里它们一直在）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerStepperRow.kt",
     "（iOS UIStepper 语义，REX 同源）", "（iOS UIStepper 语义）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerToast.kt",
     "播放页的**轻提示**（REX 交互 M08-03）。", "播放页的**轻提示**。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "播放页视觉 token（player-ui-v3-rex-replication.md §一，唯一常量源）",
     "播放页视觉 token（唯一常量源，见播放页视觉规格）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "**数值来源**：REX 1.0.2 真机横屏播放页逐像素测量", "**数值来源**：真机横屏播放页逐像素测量", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "python tools/measure_rex_playback.py", "python tools/measure_baseline_playback.py", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "REX 是**半透明白", "目标是**半透明白", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "底部渐变：REX 底栏有黑渐变", "底部渐变：底栏有黑渐变", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "vs REX 55pt", "vs 基线 55pt", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "—— REX 投屏/PiP 掩码 ink", "—— 实测投屏/PiP 掩码 ink", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "（REX 的反馈族＝", "（反馈族＝", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "（REX 实测 17pt）", "（实测 17pt）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "（REX 实测 21×18.3pt）", "（实测 21×18.3pt）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "**非 REX 实测**", "**非实测值**", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTokens.kt",
     "（sk_16 逐像素复测", "（逐像素复测", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTopBar.kt",
     "实测 REX 顶栏白图标", "实测顶栏白图标", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTopBar.kt",
     "REX 0172 也隐掉它", "实测也隐掉它", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTopBar.kt",
     "（REX 0172 的反白圆）", "（反白圆）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerWindow.kt",
     " * REX 播放时**也是全屏** —— 给我们的录屏里能看到状态栏，",
     " * 播放时**必须全屏** —— 参考录屏里能看到状态栏，", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerWindow.kt",
     " * 是因为录屏为了展示功能才把系统栏显出来，不是 REX 的播放态。",
     " * 是因为录屏为了展示功能才把系统栏显出来，不是常态播放态。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerWindow.kt",
     " * REX（iOS）的 chrome 全部按", " * iOS 基线的 chrome 全部按", 1),

    # ---- backend ----
    ("backend/include_private/video/video_plane/core/audio/pcm_channel_downmix.hpp",
     "（mpv / libmpv / SenPlayer）", "（mpv / libmpv）", 1),
    ("backend/include_private/video/video_plane/backends/mediacodec/session/audio_decode_feeder.hpp",
     "（mpv/libmpv/SenPlayer）", "（mpv/libmpv）", 1),
    ("backend/src/video/video_plane/backends/avsync/session/audio_playback.cxx",
     "mpv / libmpv / SenPlayer 都是", "mpv / libmpv 都是", 1),

    # ---- 参考录屏帧号 / 动效编号出处（D6 ①类，grep 不命中，一并清除）----
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerAudioOnly.kt",
     " * 0172 推翻旧稿", " * 实测推翻旧稿", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerBottomBar.kt",
     "（0169 实测）", "（实测）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerCaptureButton.kt",
     "（0162/0172 播放态无此钮）", "（播放态无此钮）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureHud.kt",
     " * 上下滑手势的 **HUD**（参考 M03-S03）。", " * 上下滑手势的 **HUD**。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureHud.kt",
     "## 动效（M03-S03：淡入 0.15 / 停止 1s / 淡出 0.3）",
     "## 动效（淡入 0.15 / 停止 1s / 淡出 0.3）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "// 【竖向拖动：亮度/音量】参考 M03-S03。", "// 【竖向拖动：亮度/音量】左半屏调亮度，右半屏调音量。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "（§二 M03-S07）", "（§二）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "参考 M03-S02：longPress≥0.5s", "longPress≥0.5s", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyph.kt",
     "（圈内波形条，0162 锚定）", "（圈内波形条）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyph.kt",
     "（耳机，0172 白圆亮）", "（耳机，白圆亮）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGlyphChrome.kt",
     "（0169：胶片盒 / 码率菱形叠）", "（胶片盒 / 码率菱形叠）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerLongPressHud.kt",
     "（参考 M03-S02：\"倍速标签淡入\"", "（\"倍速标签淡入\"", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerOverlay.kt",
     "（M03-S07：锁定不是全禁）", "（锁定不是全禁）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerOverlay.kt",
     "而不是被 if 直接摘掉（M03-S03）。", "而不是被 if 直接摘掉。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerResourcePanel.kt",
     "，帧 0169）", "）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerResourcePanel.kt",
     " * 实测（0169 逐像素）：", " * 实测（逐像素）：", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerResourcePanel.kt",
     "【0169 推翻设计稿的一处】", "【实测推翻设计稿的一处】", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerGestureRouter.kt",
     "/** 竖滑起点：左半屏亮度、右半屏音量（M03-S03）。 */",
     "/** 竖滑起点：左半屏亮度、右半屏音量。 */", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerGestureRouter.kt",
     "（M03-S03\"停止 1s→淡出\"）", "（\"停止 1s→淡出\"）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerGestureRouter.kt",
     "（M03-S02：longPress≥0.5s → 标签淡入）", "（longPress≥0.5s → 标签淡入）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerGestureRouter.kt",
     "但调速/上滑锁定 ❌（M03-S07）。", "但调速/上滑锁定 ❌。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonMenu.kt",
     "（**无胶囊底**，0178）", "（**无胶囊底**）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonMenu.kt",
     "（0179 缩略图在玻璃后糊成色块）", "（缩略图在玻璃后糊成色块）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonPicker.kt",
     "两行简介（0179），", "两行简介，", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/detail/DetailSeasonPicker.kt",
     "（0178 新状态 E01 带边）", "（新状态 E01 带边）", 1),

    # ---- tools（活跃脚本；冻结参考目录 tools/reference/ 不在范围）----
    ("tools/rewrite_discover.py", "rex_browse_a 帧实测", "浏览页帧实测", 1),
    ("tools/measure_kivo_playback.py",
     "（dp = px / density，与 REX 的 pt 同纲）", "（dp = px / density，与基线 pt 同纲）", 1),
    ("tools/measure_kivo_playback.py",
     "y0/y1 缺省按 REX 基线帧(1170 高)的比例换算", "y0/y1 缺省按基线帧(1170 高)的比例换算", 1),
    ("tools/measure_kivo_playback.py",
     "聚类/最长_run 复用 measure_rex_playback，保证两边算法逐位一致。",
     "聚类/最长_run 复用 measure_baseline_playback，保证两边算法逐位一致。", 1),
    ("tools/measure_kivo_playback.py",
     "import measure_rex_playback as rex", "import measure_baseline_playback as baseline", 1),
    ("tools/measure_kivo_playback.py", "rex.", "baseline.", 10),
    # 更名后的基线测量脚本（apply 前需先 git mv）
    ("tools/measure_baseline_playback.py",
     "\"\"\"REX 播放页像素基线测量", "\"\"\"播放页像素基线测量", 1),
    ("tools/measure_baseline_playback.py",
     "python tools/measure_rex_playback.py", "python tools/measure_baseline_playback.py", 4),

    # ---- 独立审计 FAIL 后的修复（2026-11-13）：漏清帧号 + 改写质量 ----
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerBottomBar.kt",
     "（0170 实测）", "（实测）", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/design/DesignTokens.kt",
     "（浅色态语义色板硬参数；深色态 v2）", "（浅色态实测硬参数；深色态 v2）", 1),
    # PlaybackEngine 的中间态「链路独立…」已由首条规则直接收敛到最终文本，不单列规则。
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureLayer.kt",
     "// 【竖向拖动：亮度/音量】。",
     "// 【竖向拖动：亮度/音量】左半屏调亮度，右半屏调音量。", 1),
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerGestureFlash.kt",
     " * 用一个居中的图标闪 0.5 秒来确认", " * 交互反馈用一个居中的图标闪 0.5 秒来确认", 1),
]

# 并发的 262 卡提交二次改写了该注释（现文本不含产品名/出处编号，满足语义判据）。
# 对这些规则只校验「原文已消失」，不要求「本卡拟定的新文仍在」。
SOFT_RULES = {
    ("android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt",
     "/** 轻提示：1.8s 后自动消失（REX M08-03 停留 1.0–2.0s）。 */"),
}


def read(path: pathlib.Path) -> str:
    with open(path, "r", encoding="utf-8", newline="") as f:
        return f.read()


def write(path: pathlib.Path, text: str) -> None:
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


# 清理前工作区 = HEAD + 主助手两个未提交 WIP（本卡不碰，保持工作区版本）。
WIP_FILES = {
    "android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerBarText.kt",
    "android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerTopBar.kt",
}


def git_out(*args: str) -> str:
    return subprocess.check_output(
        ["git", "-c", f"safe.directory={ROOT.as_posix()}", *args],
        cwd=ROOT, text=True, encoding="utf-8", errors="replace",
    )


def eligible(rel: str) -> bool:
    if not any(rel == r or rel.startswith(r + "/") for r in SCAN_ROOTS):
        return False
    # 顶层 artifacts/ 为冻结归档；backend 下治理闸门生成的 artifacts/ 证据纳入扫描并列入白名单。
    if rel.startswith(EXCLUDE_PREFIXES) or rel.startswith("artifacts/"):
        return False
    p = pathlib.Path(rel)
    return p.suffix in SCAN_EXTS or p.name == "CMakeLists.txt"


def scan_files() -> list[pathlib.Path]:
    out: list[pathlib.Path] = []
    for root in SCAN_ROOTS:
        for p in (ROOT / root).rglob("*"):
            if not p.is_file():
                continue
            rel = p.relative_to(ROOT).as_posix()
            if eligible(rel):
                out.append(p)
    return sorted(out)


def head_snapshot() -> list[tuple[str, str]]:
    """从固定基线提交一次导出整棵树，重建「清理前」文本（纯提交态，不混入工作区 WIP）。

    审计披露：清理前工作区另有主助手 2 个未提交 WIP，已自行清掉 7 行；
    故纯提交基线 = 144 HIT / 127 product，清理前工作区 = 137 HIT / 120 product。
    两个数字都在 README 披露，本函数只负责可复算的纯提交基线。
    """
    import io
    import tarfile

    raw = subprocess.check_output(
        ["git", "-c", f"safe.directory={ROOT.as_posix()}",
         "archive", "--format=tar", BASELINE_COMMIT],
        cwd=ROOT,
    )
    out: list[tuple[str, str]] = []
    with tarfile.open(fileobj=io.BytesIO(raw)) as tf:
        members = sorted(
            (m for m in tf.getmembers() if m.isfile() and eligible(m.name)),
            key=lambda m: m.name,
        )
        for m in members:
            f = tf.extractfile(m)
            if f is None:
                continue
            out.append((m.name, f.read().decode("utf-8", errors="replace")))
    return out


def classify(line: str) -> str:
    """product = 违规；whitelist = 已批准白名单；unknown = 未分类命中（按违规处理）。"""
    if PRODUCT.search(line):
        return "product"
    if WHITELIST.search(line):
        return "whitelist"
    return "unknown"


def snapshot(label: str) -> dict:
    lines_out: list[str] = []
    counts = {"product": 0, "whitelist": 0, "unknown": 0}
    files = {k: set() for k in counts}
    frame_hits: list[str] = []
    if label == "before":
        pairs = [(rel, text.splitlines()) for rel, text in head_snapshot()]
    else:
        pairs = []
        for p in scan_files():
            try:
                pairs.append((p.relative_to(ROOT).as_posix(), read(p).splitlines()))
            except Exception:
                continue
    scanned = len(pairs)
    for rel, lines in pairs:
        for n, line in enumerate(lines, 1):
            if HIT.search(line):
                bucket = classify(line)
                counts[bucket] += 1
                files[bucket].add(rel)
                lines_out.append(f"{rel}:{n}: [{bucket}] {line.strip()}")
            if FRAME_IDS.search(line):
                frame_hits.append(f"{rel}:{n}: {line.strip()}")
    target = EVID / f"grep_{label}.txt"
    write(target, "\n".join(lines_out) + "\n")
    write(EVID / f"frame_ids_{label}.txt", "\n".join(frame_hits) + "\n")
    result = {
        "label": label,
        "source": f"{BASELINE_COMMIT} (pure commit)" if label == "before" else "working tree",
        "scanned_files": scanned,
        "hit_lines": sum(counts.values()),
        "counts": counts,
        "counts_files": {k: len(v) for k, v in files.items()},
        "frame_id_lines": len(frame_hits),
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return result


def apply() -> None:
    pending: dict[pathlib.Path, str] = {}
    for rel, old, new, expected in RULES:
        p = ROOT / rel
        text = pending.get(p)
        if text is None:
            if not p.exists():
                raise SystemExit(f"[FAIL] 文件不存在（是否已执行 git mv？）: {rel}")
            text = read(p)
        found = text.count(old)
        if found != expected:
            # 幂等：已应用过（原文消失且新文在场），或 SOFT 规则被并发合法改写 → 跳过。
            if found == 0 and (text.count(new) >= expected or (rel, old) in SOFT_RULES):
                continue
            raise SystemExit(
                f"[FAIL] {rel}: 期望 {expected} 次，实际 {found} 次 -> {old!r}\n"
                "全部改动已回滚（未写任何文件）"
            )
        pending[p] = text.replace(old, new)
    for p, text in pending.items():
        write(p, text)
    print(f"[OK] {len(RULES)} 条改写规则全部唯一命中，已写入 {len(pending)} 个文件")


def verify() -> None:
    problems: list[str] = []
    for rel, old, new, expected in RULES:
        p = ROOT / rel
        if not p.exists():
            problems.append(f"文件缺失: {rel}")
            continue
        text = read(p)
        if old in text:
            problems.append(f"原文仍残留: {rel} -> {old[:50]!r}")
        if (rel, old) not in SOFT_RULES and new not in text:
            problems.append(f"新文未落地: {rel} -> {new[:50]!r}")
    counts = {"product": 0, "whitelist": 0, "unknown": 0}
    details: list[str] = []
    frame_lines: list[str] = []
    for p in scan_files():
        rel = p.relative_to(ROOT).as_posix()
        try:
            lines = read(p).splitlines()
        except Exception:
            continue
        for n, line in enumerate(lines, 1):
            if HIT.search(line):
                bucket = classify(line)
                counts[bucket] += 1
                if bucket != "whitelist":
                    details.append(f"{rel}:{n}: [{bucket}] {line.strip()}")
            if FRAME_IDS.search(line):
                frame_lines.append(f"{rel}:{n}: {line.strip()}")
    result = {
        "rule_count": len(RULES),
        "soft_rules_relaxed": [f"{r}: {o[:60]}" for r, o in sorted(SOFT_RULES)],
        "rule_problems": problems,
        "hit_lines": sum(counts.values()),
        "counts": counts,
        "violations": details,
        "frame_id_lines": frame_lines,
        "verdict": "PASS" if not problems and counts["product"] == 0 and counts["unknown"] == 0 else "FAIL",
    }
    write(EVID / "verify_result.json", json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "frame_id_lines"},
                     ensure_ascii=False, indent=2))
    if frame_lines:
        print(f"\n[INFO] 录屏帧号/动效编号残留 {len(frame_lines)} 行（非 grep 闸门，供披露）：")
        for x in frame_lines:
            print("  " + x)


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "verify"
    if cmd == "snapshot":
        snapshot(sys.argv[2])
    elif cmd == "apply":
        apply()
    elif cmd == "verify":
        verify()
    else:
        raise SystemExit("usage: derex_apply.py {snapshot before|apply|snapshot after|verify}")
