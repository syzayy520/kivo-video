# 223 · 弹幕 —— 设计稿

> 状态：**设计完成，实现开工**。M2 第四卡。
> 卡片原文：B 站 XML/protobuf 解析 + Compose Canvas 渲染层。
> 参照：SenPlayer `danmu.js`（OfficialModules JS Widget）；Rex 弹幕开关 `弹`（播放器顶栏）。
> 220 v1 节拍曲线仪表已建——弹幕渲染不影响视频节拍（Canvas 层叠加）。

---

## 一、一句话目标

**B 站 XML 弹幕文件解析 → Compose Canvas 滚动弹幕层叠加到播放页——
弹/关切换、滚动速度实时、不阻塞解码线程。**

## 二、设计决策

### D1 解析：B 站 XML 格式（离线最简）

- 格式：`<d p="time,mode,size,color,date,pool,uid,rowhash">text</d>`
- time=秒、mode=1滚动/4底部/5顶部、size=字号、color=RRGGBB 十进制
- protobuf（B 站 API 二进制格式）后续卡——v1 只做 XML（离线文件/本地弹幕）

### D2 渲染：Compose Canvas overlay

- `DanmakuOverlay` Composable：Box + Canvas，覆盖在 SurfaceView 上层；
- 滚动弹幕：从右到左平移，速度 = (屏宽 + 文字宽) / 显示时长（默认 8s）；
- 行分配：简单按取模分配轨道（防碰撞 v1，不用碰撞检测）；
- 颜色/字号来自 XML 属性；
- 不阻塞解码线程（纯 Compose 绘制）。

### D3 开关：顶栏"弹"圈（Rex 同款）

- PlayerControls 加弹幕开关圆钮（"弹"字图标）；
- 开→ Canvas 层显示 + 弹幕计时；关→ 隐藏。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `danmaku/DanmakuItem.kt` | **新** | 数据类（text/time/mode/color/size） |
| `danmaku/DanmakuParser.kt` | **新** | B 站 XML 解析 → List<DanmakuItem> |
| `danmaku/DanmakuOverlay.kt` | **新** | Compose Canvas 滚动弹幕层（轨道分配 + 平移） |
| `ui/compose/PlayerComposeActivity.kt` | 旧（小改） | 弹幕开关 + XML 文件加载 |

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 解析 | B 站 XML → 100 条弹幕正确解析（text/color/mode） |
| AC2 | 滚动 | 弹幕从右到左滚动、速度一致 |
| AC3 | 开关 | 弹/关切换生效 |
| AC4 | 不阻塞 | 弹幕层不影响视频解码/节拍 |

## 五、明确不做

protobuf（B 站 API 二进制）· 碰撞检测（v1 取模分配轨道）· 弹幕发送/上传 ·
高级模式（高级弹幕/定位弹幕）。
