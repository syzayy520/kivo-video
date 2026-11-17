# Golden 基线 · 竞品帧 ↔ kivo 页面映射

> 目的：为 B1–B5 的每一批 UI 对齐提供**可复算的对照基准**。竞品帧（Rex）已入库，kivo 侧截图待补。
> 竞品帧来源：`C:\kivo video\.tmp\rex_scaled\`（116 张缩略图）中挑出的 12 张关键帧，原样复制、未缩放、未改名。
> 生成时间：2026-11-16 · 归属任务：B0-T4 · 门禁脚本：`tools/ui_gate.py`

---

## 1. 归档原则

- 只归档**每批验收会直接比对**的帧，共 **12 张**，控制入库体积（当前 1.5 MB）。
- 全部保持原始文件名（`<日期>_<时间>_IMG_<编号>.jpg`），编号即审计报告里的引用编号，避免二次命名带来对不上的风险。
- `.tmp/rex_scaled/` 只是工作区临时目录；`docs/audit/golden/rex/` 才是入库副本，后续清理 `.tmp` 不影响基线。
- **禁止**在此目录增删竞品帧；需要更多帧时按同样规则追加并更新本表与数量。

---

## 2. 映射表（12 张）

kivo 截图状态一列：当前全部为「待补（adb 不在 PATH，B1 起补）」——本机 `adb` 未加入 PATH，且 kivo 侧截图必须真机、同页、同机位才有比对意义，模拟器与拼图一律无效。

| # | 竞品帧（`docs/audit/golden/rex/`） | 尺寸 / 方向 | 页面 | 修复批次 | kivo 对应文件 | kivo 截图状态 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | `2026_11_11_19_21_08_IMG_0159.jpg` | 1500×693 · 横屏 | 播放器 · 基础控制层 | B4 | `ui/PlayerActivity.kt`、`ui/compose/PlayerPanelRouter.kt`、`ui/compose/player/PlayerBarsHost.kt` | 待补（adb 不在 PATH，B1 起补） |
| 2 | `2026_11_11_19_21_42_IMG_0162.jpg` | 1500×693 · 横屏 | 播放器 · 底栏与进度轨 | B4 | `ui/compose/player/PlayerBottomBar.kt`、`ui/compose/player/PlayerEpisodeStrip.kt` | 待补（adb 不在 PATH，B1 起补） |
| 3 | `2026_11_11_19_22_08_IMG_0165.jpg` | 1500×693 · 横屏 | 播放器 · 玻璃菜单/面板 | B4 | `ui/compose/player/PlayerGlassMenu.kt`、`ui/compose/PlayerPanelRouter.kt` | 待补（adb 不在 PATH，B1 起补） |
| 4 | `2026_11_11_19_22_38_IMG_0170.jpg` | 1500×693 · 横屏 | 播放器 · 详情卡 | B4 | `ui/compose/player/PlayerDetailCard.kt` | 待补（adb 不在 PATH，B1 起补） |
| 5 | `2026_11_11_19_22_43_IMG_0171.jpg` | 1500×693 · 横屏 | 播放器 · 手势 HUD | B4 | `ui/compose/player/PlayerGestureHud.kt`、`ui/compose/PlayerComposeActivity.kt` | 待补（adb 不在 PATH，B1 起补） |
| 6 | `2026_11_11_19_23_01_IMG_0173.jpg` | 1500×693 · 横屏 | 播放器 · 选集态轨道 | B4 | `ui/compose/player/PlayerEpisodeStrip.kt`、`ui/compose/player/PlayerBottomBar.kt` | 待补（adb 不在 PATH，B1 起补） |
| 7 | `2026_11_16_21_16_58_IMG_0330.jpg` | 693×1500 · 竖屏 | 首页/发现 · 顶部 hero | B3 | `ui/design/DiscoverPage.kt`、`ui/design/home/HeroPage.kt` | 待补（adb 不在 PATH，B1 起补） |
| 8 | `2026_11_16_21_17_23_IMG_0333.jpg` | 693×1500 · 竖屏 | 首页/发现 · 模块流 | B3 | `ui/design/DiscoverPage.kt` | 待补（adb 不在 PATH，B1 起补） |
| 9 | `2026_11_16_21_17_30_IMG_0334.jpg` | 693×1500 · 竖屏 | 首页/发现 · 继续观看卡 | B3 | `ui/design/DiscoverPage.kt`、`ui/design/HeroCarousel.kt` | 待补（adb 不在 PATH，B1 起补） |
| 10 | `2026_11_16_21_17_41_IMG_0335.jpg` | 693×1500 · 竖屏 | 首页/发现 · 分页点与底部 | B3 | `ui/design/DiscoverPage.kt`、`ui/design/HeroCarousel.kt` | 待补（adb 不在 PATH，B1 起补） |
| 11 | `2026_11_16_21_21_07_IMG_0357.jpg` | 693×1500 · 竖屏 | 我的 · 看板首屏 | B2 | `ui/design/Pages.kt`（`ProfilePage`） | 待补（adb 不在 PATH，B1 起补） |
| 12 | `2026_11_16_21_22_14_IMG_0366.jpg` | 693×1500 · 竖屏 | 资源库 · 列表与导航选中态 | B1（导航选中态） | `sources/ui/SourcesScreen.kt`、`sources/ui/SourceCard.kt`、`ui/design/CapsuleNavBar.kt` | 待补（adb 不在 PATH，B1 起补） |

> 批次归属依据 `docs/audit/ui-parity-master-report.md` §10：B1 外壳（0366 导航选中态验收）、B2 我的（0357–0362）、B3 发现（0330–0335）、B4 播放器（0159/0162/0165/0170/0171/0173）。
> 竞品帧为 iPhone 实拍/截图：播放器为横屏 1500×693，页面为竖屏 693×1500；kivo 侧同为竖屏 1080×2400（播放器横屏），**只比布局与视觉参数，不做逐像素缩放比对**。

---

## 3. kivo 补图命令模板

设备要求（硬性）：

| 项 | 要求 |
| --- | --- |
| 设备 | Redmi K40，序列号 **`b6e950ee`**（唯一有效取证机；模拟器数字无效） |
| 分辨率 | 1080×2400（系统显示设置不缩放、不改 DPI） |
| 方向 | 页面类竖屏；播放器类横屏（先切到与竞品帧一致的横屏再截） |
| 状态 | 同一网络、同一片源、时间固定（如 `05:00`）、亮度与系统字体默认、关闭勿扰浮层 |
| 前置 | `adb devices` 必须显示 `b6e950ee  device`；HyperOS 首次安装会弹窗，需人工点「继续安装」 |

命令模板（在仓库根执行）：

```bash
# 0) 确认设备在线
adb devices

# 1) 固定设备状态（可选但建议，保证多轮可复算）
adb -s b6e950ee shell settings put system screen_brightness 128
adb -s b6e950ee shell settings put system accelerometer_rotation 0
adb -s b6e950ee shell settings put system user_rotation 0        # 0=竖屏；1=横屏（播放器）

# 2) 截图并落到 golden/kivo/（命名规则：kivo_<竞品编号>_<页面>.png）
mkdir -p docs/audit/golden/kivo
adb -s b6e950ee exec-out screencap -p > docs/audit/golden/kivo/kivo_0159_player_base.png
adb -s b6e950ee exec-out screencap -p > docs/audit/golden/kivo/kivo_0330_discover_hero.png
# …同一模式，编号与上表一一对应

# 3) 校验落盘（PNG 头部 + 尺寸）
python -c "import struct,sys;d=open(sys.argv[1],'rb').read(33);print(len(d) and struct.unpack('>II', d[16:24]))" docs/audit/golden/kivo/kivo_0159_player_base.png
```

补图完成后：把上表「kivo 截图状态」逐行改成实际路径（如 `docs/audit/golden/kivo/kivo_0159_player_base.png`），并注明截图时间与设备序列号。**未真机截图前不得把该列改为「已对齐」。**

---

## 4. 本目录自检

```bash
# 数量必须为 12（B0 基线约定）
ls docs/audit/golden/rex/*.jpg | wc -l
# 12 个编号齐全
ls docs/audit/golden/rex/ | grep -oE "IMG_[0-9]{4}" | sort | tr '\n' ' '
# 期望：IMG_0159 IMG_0162 IMG_0165 IMG_0170 IMG_0171 IMG_0173 IMG_0330 IMG_0333 IMG_0334 IMG_0335 IMG_0357 IMG_0366
```