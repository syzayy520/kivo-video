# 夜间自主推进收口（2026-11-21 22:00 → 11-22 06:0x）

> 你睡前说「按建议推进，10 点前自己安排」。以下是我做完的、没做的、以及需要你拍板的。
> 所有改动**未 commit / push**；设备 `b6e950ee` 已装成本轮最终包；未对 `dd137cf7` 发过任何 adb。

## 0. 一句话

面板这条线（材质 v3 → 低项 → 拆分 → 投屏卡 → 字幕次延迟 → 关闭闪现 → 背景磨砂 → 设备行取证）**全部闭环**；
门禁只剩 **1 项裸色**且**不是本轮引入**；**退场动画**查了 12 轮仍无解，已如实登记不做。

## 1. 本轮成果（按时间）

| 项 | 结果 | 证据 |
| --- | --- | --- |
| O1 次字幕面板锚点 | 补 `subtitleSecondary → captions`；顶边 82.18dp / 左缘 608.73dp 与字幕卡一致 | `O1-DEVICE.md` |
| KivoSub 埋点迁移 | 删 `Log.i("KivoSub")`，改 smbTrace（真机可读） | `O2-DEVICE.md` |
| O2 M4 场景 | 主 A+次 B 删 A ⇒ 内核自选到 B 且 `sec=no`，**不扩展兜底** | `O2-DEVICE.md` |
| 面板低项 4 项 | 描边真机 1.09dp、内容/滚动条裁进圆角、滚动监听不每帧重启、CastSheet token 化 | `R0-DEVICE.md` |
| StepperRow 拆分 | 450 行 → `player/row/` 5 文件（167/185/76/47/46），零行为 | `S0-DEVICE.md` |
| 投屏卡 v2 | 全宽贴底 → 248dp 浮卡锚投屏图标（368.7dp/82dp），行结构按 SenPlayer | `C0-DEVICE.md` |
| **关闭面板中间闪一下** | 根因 `anchorFor("")→Center`；60fps 逐帧中间区跳变 **+37.15 → 0.2~0.34** | `BUG-DEVICE.md` |
| 面板背景磨砂 | mpv 截图抓帧 → 480×270 → 盒式模糊；暗场 114（SenPlayer 117–120） | `BD-DEVICE.md` |
| 投屏设备行取证 | 真实设备「小爱音箱-8245 / 192.168.5.145」，卡片顶 82.2dp | `DLNA-DEVICE.md` |

## 2. 门禁（`python tools/ui_gate.py`）

- **行数回归：已修**。我的 backdrop 参数把 `PlayerGlassMenu` 顶到 302 行 → 把 `MenuHeader` 移入
  `PlayerGlassMenuParts`（现 257 / 138），门禁复跑「行数无新增超限文件」。
- **裸色仍 359 > baseline 326（+33）**，`RESULT: FAIL`。归属已查：
  - `DesignPalette.kt` / `PlayerTokens.kt` 的未提交改动**只 +2** 个 `Color(0x…)`；
  - 缺口在**未跟踪**的 `PlayerPanelStyle.kt`（35 处，材质单一真值表）。
  - 我的判断：这是**颜色令牌/材质表**，裸色是它的内容，不应算违规；但把门禁改成豁免会改变判定口径、
    还要刷 baseline，**属于治理决策，我没擅自做**。等你一句话：
    a) 加 `BARE_COLOR_EXEMPT = {PlayerPanelStyle.kt, DesignPalette.kt, PlayerTokens.kt}` 后刷 baseline；
    b) 先把这些色改成引用 DesignTokens（工作量大、可能撞色）；
    c) 就让它报着，等 B5 治理批。

## 3. 没做的事（及原因）

| 项 | 为什么没做 |
| --- | --- |
| **面板退场动画** | 12 轮探针（800ms、Animatable、`if(true)/if(false)`、延迟清 kind、`key()`）全部仍是 1 帧硬切 ⇒ **组件内无解**，只能独立窗口或 Activity 层自绘。已全部回滚，交接第 11 条记着 |
| **亮场自适应材质** | 会让白字可读，但要按背景亮度在白/深玻璃间切 —— **推翻你拍板的 v3**，必须你点头 |
| 投屏点选 → 真正推流 | 会真的把视频推到你的音箱，不在无人看护时执行 |
| 夹具 `kivo-test-renderer` 未进列表 | 现象已记录；原因（描述字段/去重顺序）未深究 |

## 4. 已知边界（都在证据里写明）

- 亮场白字**仍不可读**（合成 234.7），与 SenPlayer 同量级 —— **持平不是解决**。
- 背景磨砂**只抓一次**（打开那一刻），面板开着不随画面更新。
- 抓帧失败的四类分支（`no-session/command-rejected/no-file/decode-null`）写了 trace，**没在真机人为触发**。
- StepperRow 拆分只做了**视觉核对**，8 类行的交互没逐一点按。
- 退场硬切：面板区 165→106.7 一帧完成。

## 5. 建议的下一批（你来排）

1. **裸色治理决策**（上面 §2 的 a/b/c）—— 这个卡着门禁全绿
2. **亮场自适应材质**（需重拍 v3 口径）
3. **投屏点选推流联调**（需要你在场，怕吵到音箱）
4. **已登记大项**：AI 字幕第二阶段 / Wave2 minSdk 28 + libmpv 重编（GBK）/ Wave3

## 6. 证据索引（`docs/evidence/p9-subtitle-reaccept/`）

`O1-DEVICE.md`、`O2-DEVICE.md`、`R0-DEVICE.md`、`S0-DEVICE.md`、`C0-DEVICE.md`、`BUG-DEVICE.md`、
`BD-DEVICE.md`、`DLNA-DEVICE.md`、`CARD-SENPLAYER-MEASURE.md`，以及各自的截图/录屏。