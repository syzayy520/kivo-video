# 261 设置页 · REX 1:1 复刻 · 真机取证

> 卡：设置页 UI（主助手自留）。规格：[`docs/design/ui/replica/settings-page.md`](../../../design/ui/replica/settings-page.md)。
> 设备：Redmi K40 `b6e950ee`（1080×2400 @440，真机，非模拟器）。
> APK md5：`31a8f8f63ff2b747da0be2a0a329e85d`（最终取证构建）。
> 构建：`cd android && JAVA_HOME=C:/Android/jdk-17.0.13+11 ANDROID_HOME=C:/Android/sdk ./gradlew :app:assembleDebug --offline --no-configuration-cache`。
> 家族：`ui/design/settings/` 8 文件（Store/Rows/Page/Nav/PlaybackSub/DataSub/CacheSub/AboutSub）；退役 `Pages.kt` 骨架。

## 一、闸门判定（S1–S6）

| # | 闸门 | 判定 | 证据 |
| --- | --- | --- | --- |
| S1 | 视觉 1:1（inset 分组卡 59pt 行/节头/多行脚注/浅色） | PASS | `shots/s1-root.png`（6 组 inset 白卡 + 节头小字灰 + 大标题 28pt）；`s2/s4/s6/s9` 子页含多行「敢解释」脚注 |
| S2 | 无假状态（每行绑真实持久化/导航；开关重启保持） | PASS | 开关：`s10-toggle-on.png` 绿 ON + `logs/s10-autoplay-persist.log`（true→重启仍 true→复原 false）；只读真实值：`s4`（API Key 已配置/代理 直连）、`s9`（版本 0.1.0-p12a (1) 取自 PackageManager）、`s6`（图片缓存 9.1 MB 实测）；无后端行不放 |
| S3 | 导航正确（push/pop 返回栈；跨 tab；外部 Activity） | PASS | 系统返回回根页：`s3-back-root.png` + `logs/s3-back-root-dump.xml`（六组全在、无子页内容）；跨 tab：`s5-sources-nav.png` + dump（媒体服务器 chevron→资源库页）；外部：`logs/s12-privacy-nav.log`（PrivateActivity resumed→back→MainActivity） |
| S4 | 家族/行数（单文件单职责 ≤300；骨架迁出不留重复） | PASS | `logs/s4-line-counts.txt`：新包最大 126 行；Pages.kt 138→82；MainActivity 357 未增（既有 #37 债）；GroupSection/GroupRow 仅存于注释 |
| S5 | 动效（push/pop/按压/开关对照 REX 候选） | PASS（功能）/ 披露 D6 | push/pop 功能成立（子页滑入出现、返回回根）；AnimatedContent slide+fade 350ms EaseOut = REX 候选 M05-S01(0.35s)/M04-S01；按压=Material ripple（#15 口径）；开关弹簧+accentGreen。**真_mid 帧未捕获**（screencap 延迟>过渡），丝滑度未帧证 → 候选值/运行时未测 |
| S6 | 证据/红线（真机 K40；凭据不入证据；含密码态不截图） | PASS | 全部帧真机 screencap；privacy 仅 dumpsys 断言不截图（D7）；证据无 TMDB key 值/无服务器密码/无 token（`s4` 仅显示「已配置」状态） |

## 二、帧索引

| 帧 | 内容 |
| --- | --- |
| s1-root.png | 根页：6 组 inset 卡；首页开关 OFF（iOS 灰轨白 thumb）；行标题与节头不重复 |
| s2-playback.png | 播放与字幕子页：明文=已开启（真实）、解码/字幕只读策略 + 脚注 |
| s3-back-root.png | 系统返回后回根页（返回栈断言） |
| s4-data.png | 数据与资源子页：媒体服务器 chevron；API Key 已配置 / 代理 直连（真实） |
| s5-sources-nav.png | 媒体服务器 chevron 跨 tab 落到资源库页 |
| s6-cache.png | 存储与缓存子页：图片缓存 9.1 MB / 评分 0 B（实测） |
| s7-cache-confirm.png | 清理确认弹窗（取消/清理，说明影响范围） |
| s8-cache-cleared.png | 清理后 0 B + 绿字「已释放 9.1 MB」 |
| s9-about.png | 关于子页：版本 0.1.0-p12a (1)（PackageManager 真实值）+ 私密入口 |
| s10-toggle-on.png | 开关绿 ON，且为 force-stop 重启后（持久化断言） |
| s11-toggle-off.png | 复原 OFF（默认 false，设备留默认态） |

## 三、日志索引

- `logs/s3-back-root-dump.xml`：返回后 UI dump（根页六组文本齐全）。
- `logs/s5-sources-nav-dump.xml`：跨 tab 后 UI dump（资源库页文本）。
- `logs/s6-cache-bytes.log`：清理前 `du -sb`=9558473 / 119 文件；清理后 24576（仅目录 inode）/ 0 文件；`tmdb_cache.json` absent。
- `logs/s10-autoplay-persist.log`：`shared_prefs/home_display.xml` 三次采样 true→true(重启后)→false(复原)。
- `logs/s12-privacy-nav.log`：dumpsys `topResumedActivity` = PrivateActivity → back → MainActivity。
- `logs/s4-line-counts.txt`：家族/ touched 文件行数。

## 四、披露（取舍与未测，明示不隐藏）

- **D1 背景**：设置页沿用全应用 `AnimatedGradientBg`（淡紫渐变）而非 REX 扁平 `groupedFill #F2F2F7`；白卡 inset 为 REX 设置的决定性特征，渐变壳与其余 tab 统一（用户已验收discover/资源库同壳）。属刻意适配，非缺陷。
- **D2 行标题**：根页行标题改为「目的地描述」（媒体服务器与 TMDB/清理缓存/版本与 UI 对齐…），与节头类别名不重复，避免「头=行」的廉价重复感；子页标题仍为类别名。
- **D3 开关复用**：唯一真实开关复用 `HomePreviewSetting`（不新建重复 SwitchRow）；其 Switch 颜色设为 `accentGreen` 轨+白 thumb、未选 iOS 灰轨白 thumb 无边框——修掉 Material 默认紫轨（与 DesignTokens「toggle 用 accentGreen」及 REX 绿开关对齐）。
- **D4 返回栈深度**：子页互不嵌套 ⇒ 用可空当前路由（深度≤1）而非多级栈；`rememberSaveable` 保配置变更/进程重建，**不**保 tab 切换（切 tab 回设置为根页）。
- **D5 只读策略行**：播放/字幕/按钮/手势/倍速的逐项控制在播放页面板（PlayerComposeActivity，主设置不可达）且当前不持久化全局默认 ⇒ 本页只读说明 + 脚注指明真实控制位置，**不放假开关、不伪造导航**。
- **D6 动效未帧证**：push/pop 为 AnimatedContent slide+fade 350ms EaseOut（REX 候选 M05-S01 0.35s / M04-S01 整页平移），功能成立；screencap 延迟大于过渡时长，未拿到真 mid 帧，丝滑度/弹簧拟合**运行时未测**，参数为候选值非实测常量。
- **D7 密码态不截图**：privacy 入口仅以 dumpsys 断言 Activity 切换；含密码表单态一律不 screencap/不入证据。
- **D8 缓存清理真实**：清理同时删磁盘图缓存 + 清进程内图缓存（`TmdbImageCache.clearMemory`，本卡新增）+ 删评分 json；on-device du/ls 复算证实 119→0 文件。不触碰资源库/观看记录/密码。
- **D9 设备留默认**：autoplay 取证后复原 false；缓存清理为真实副作用（海报下次浏览重新加载），已明示。
- **D10 行数债**：MainActivity 357 行仍超 300 闸，为既有 #37 债，本卡未增（净 0）；Pages.kt 由 138 减到 82。

## 五、复算命令

```bash
export PATH="$PATH:/c/Android/sdk/platform-tools"; export MSYS_NO_PATHCONV=1
# 构建 + 装机（HyperOS 需同时点继续安装）
cd "C:/kivo video/android" && JAVA_HOME="C:/Android/jdk-17.0.13+11" ANDROID_HOME="C:/Android/sdk" \
  ./gradlew :app:assembleDebug --offline --no-configuration-cache
(adb -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk & sleep 4; adb -s b6e950ee shell input tap 347 2111; wait)
md5sum app/build/outputs/apk/debug/app-debug.apk   # 期望 31a8f8f63ff2b747da0be2a0a329e85d
# 行数闸
wc -l app/src/main/java/com/kivo/cinema/ui/design/settings/*.kt
# 缓存清理复算（清理前后各跑一次）
adb -s b6e950ee shell "run-as com.kivo.cinema du -sb cache/tmdb-images-v2; run-as com.kivo.cinema ls cache/tmdb-images-v2 | wc -l"
# 开关持久化复算
adb -s b6e950ee shell "run-as com.kivo.cinema cat shared_prefs/home_display.xml"
# 导航复算
adb -s b6e950ee shell "dumpsys activity activities | grep topResumedActivity"
```