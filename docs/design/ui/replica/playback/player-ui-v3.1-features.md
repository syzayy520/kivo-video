# player-ui-v3.1 · 功能调校设计（画中画 / 弹幕 / 投屏 / 进度条 / 丝滑度）

> 状态：**实现中**（2026-11-12）。上游：`player-ui-v3-rex-replication.md`（像素基线不动）。
> 用户指令（原话，三次重申）：
> 「画中画功能要设计适配，弹幕功能要是设计适配，投屏既然不支持app就把Apple那个去掉，
> 进度条选中的时候最好有选中效果，比如进度条会变粗，体验设计你要设计下，
> REX有设计画面帧的设计，就是拖动进度条的时候能看见当前进度的画面帧。
> 你把播放页的功能调校到完美为止，交互体验上做到极致丝滑」

本稿只覆盖**功能与交互**，不改 v3 的像素几何（顶栏 pitch 50 / 轨道 6pt / 面板锚点表）。
唯一新增几何：轨道按下态 10pt（用户指定的选中增强，REX 无此形态，注明非实测）。

---

## 一、投屏：去 AirPlay，留 DLNA（#12）

**决策**：AirPlay 是 Apple 专有协议，Android 做不了 ⇒ 图标、面板行、文案全部移除。
DLNA 已实现（230：CastController + CastSheet）⇒ 投屏入口保留，但：

1. 顶栏首枚图标从 `airplay`（屏幕+实心三角，Apple 符号）换成 `cast`
   （屏幕框 + 左下信号弧 + 圆点，通用投屏符号）。**槽位不动**：右排仍 8 枚，
   其余图标的「从右数 idx」不变 ⇒ 面板锚点表零改动。
2. 点 `cast` = **直接开 DLNA 设备 sheet**（`openCastSheet()`），不再经过只剩一行的面板。
3. `cast` 面板形态整体删除：Forms.cast() / panelTitle / enumerate / Host 锚点 /
   Tokens.castMenuWidth / PanelRouter 的 "dlna" 分支，全链路清掉，不留死代码。

**不许静默降级核对**：SMB 片源投屏在 `onCastSelect` 已有可见原因
（"SMB 片源投屏暂未开放"），保留。

## 二、进度条：选中变粗 + 拖动实时画面帧（#13）

### 2.1 选中态
- 静止 6pt（v3 实测）；**按下或拖动 → 10pt**，150ms FastOutSlowIn；松手回 6pt。
- 无滑块（REX 没有）；拖动中未播段提亮保留（v3 已有）。
- 新增 token `trackActiveHeight = 10f`，注释标明「用户指定增强，非 REX 实测」。

### 2.2 画面帧供给（关键缺口）
现状：`ThumbnailReader` 只在**本地**起播链路创建（PlayerSessionStart）；
SMB/mpv 链路 `thumbnailReader == null` ⇒ 拖动浮窗永远是空框。

**方案**：给 SMB 开**第二条独立读路径**做缩略图（与主链路零干扰，215 设计 D1 的延伸）：
```
SmbByteSource#2（预读关，随机读） → ByteSourceMediaDataSource → MediaExtractor → MediaCodec → 320px 位图
```
- 新文件 `thumbnail/ThumbnailSource.kt`：`ByteSourceMediaDataSource`（ByteSource→MediaDataSource）
  + `ThumbInput`（Path / Source 二选一）。
- `ThumbnailReader` 双构造（path / MediaDataSource）；**latest-wins 合并**：
  快速拖动时新请求顶掉排队中的旧请求（只留最新一个），防解码队列堆积。
- `yuvToBitmap` 拆去新文件 `thumbnail/ThumbnailYuv.kt`（Reader 守 ≤300 行）。
- `PlayerSmbStart` 准备线程里开第二条源 → UI 线程 `mountSmbThumbnails()`；
  Activity 新增 `smbThumbSource` 字段，`closeNetworkPlayback` 后台关闭。
- Root `onScrub` 按**秒**节流（同秒不重复请求；Reader 内还有秒级 LRU）。
- 本地链路不动。

**诚实边界**：若 extractor 打不开（无视频轨/容器不支持），浮窗保持形状、内容留空
（v3 已有设计），取证 JSON 记 `decode_failures`；不编帧、不静默。
 shelter.mkv「无索引」风险（252 注）：MediaExtractor 走 MKV Cues，与 ffprobe 全片线性扫不同；
真机实测 `cold_first_latency_ms` 后再下结论。

### 2.3 真机定论与换核（v3.1b，2026-11-12）

§2.2 留的「实测后再下结论」已有答案，**结论是否定的**：

| 证据 | 内容 |
| --- | --- |
| v31b-d2/d4.png、v31c-d6.png | 8s 长拖 +2/+4/+6.5s 浮窗全空框（几何正确） |
| v31-thumb-smb-drag.json | T+60 dump = `handler busy`：ThumbnailReader 单 HandlerThread 被首请求卡死 |

根因：无 Cues 的 MKV 让 `MediaExtractor.setDataSource(MediaDataSource)` 线性扫全片
（与 252 注的 ffprobe 13 分钟同一类），首请求阻塞 ⇒ 后续请求全部排队 ⇒ 整段拖动无帧。
**MediaExtractor 只配索引齐全的本地源**；网络/无索引源换 **mpv 内核第二实例**取预览帧
（matroska demux 无索引按字节插值定位，主播放 seek 真机 28–49ms 已证）：

```
SmbByteSource#2 → LocalRangeServer 代理 → MpvSession(vo=libmpv, ao=null, pause=yes)
  → attachSurface(ImageReader 480×270 离屏面) → seek(exact) → 取 RGBA 帧 → Bitmap
```

- 离屏面用 **ImageReader** 而非 SurfaceTexture：后者无 GL 消费端会堵死 BufferQueue，
  渲染线程停摆 ⇒  seek 后不出新帧。ImageReader 的 listener 只记帧序号，
  worker `acquireLatestImage()` 消费（顺带丢旧帧）。
- 新 `thumbnail/PreviewFrameReader.kt`：接口（seekTo / onThumbnailReady /
  **onUnavailable(reason)** / evidenceJson / release），两实现共用接线。
- 新 `thumbnail/MpvThumbnailReader.kt`：懒起内核、latest-wins、秒级 LRU、5s 闲释放、
  取证计数与 ThumbnailReader 同构（另加 `init_ms`）。失败走 onUnavailable ⇒
  **浮窗空框内画原因小字**（不许静默降级）。
- `ThumbnailReader` 实现同接口（本地链路不动，215 证据继续有效），解码失败也报原因。
- 接线：`mountSmbThumbnails` 换 MpvThumbnailReader；Activity 字段改接口类型 +
  `thumbnailUnavailableReason` 态；Root→Overlay→BarsSlot→BottomBar→SeekTrack→Preview
  传 reason；Preview 在 bitmap==null 且 reason 非空时画 11sp 居中小字。

### 2.4 延迟定论与预热（v3.1c，2026-11-12）

v3.1b 真机仍空框：`cache/mpv-thumb.log` 时间线证明管线**能出帧但太晚** ——
内核冷启 ~6.8s（主实例同代码仅 2ms，差异待 .diag 时间戳定位）+ 加载 1.5s +
**exact 解码每帧 1.6–2.2s**（seek done → first frame shown），首帧落在 8s 拖动结束之后。
三处改：

1. **预热**：`PreviewFrameReader.prewarm()`（默认空操作）；挂载即延后 1.2s 起内核
   （让主内核先完成起播读盘），init 成功后用 0s 关键帧暖 VO/解码器（首帧冷启动 ~1.7s）。
   预热期空框画「预览帧准备中…」（mount 时置 reason，首帧到达清除）。
2. **关键帧口径**：预览 seek 改 `exact=false`，与主内核 seek 同口径
   （MpvPlaybackSession 本就 exact=false）。偏差 ≤1 GOP 是预览浮窗行业口径；
   exact 的 1.6–2.2s/帧在拖动手感下不可用。
3. **常驻**：去掉 5s 闲释放（重初始化 ~8s，闲释放=每次隔段拖动重新冷启）；
   内存闸改 `demuxer-max-bytes=8MiB / back=4MiB`（预览只要单帧，64MiB 预读纯浪费）。
   内核随会话 onDestroy 释放。

诊断增量：`mpv_session.cpp mark_step` 加单调毫秒时间戳（.diag），
下一轮真机跑完即可定位 6.8s 花在 options 环还是 mpv_initialize 内。

### 2.5 出帧等待与预读（v3.1c 第二轮，2026-11-12）

v3.1c 首轮真机：预热生效（.diag 时间戳 initialize=4ms、init_ms=3057 含 SMB 加载），
但 **requests=3 / decode_failures=3 / cache=0** —— mpv 日志每 seek 都出帧，
ImageReader 里也有帧，worker 却空等超时。根因：暂停态每次 seek **只出一帧**，
旧写法等 `seeking` 结束才快照帧序号，那一帧已在快照之前进面 ⇒ 永远等不到"新"帧。
**快照必须在 seek 之前**，随后单一等待环（6s 上限）+ acquireLatestImage。

同轮第二延迟源：seek done → 出帧 1.0–2.9s，随目标距离涨 —— demux 清缓存后
按 64–256KiB 逐次 SMB 往返回填（~95ms/次）。thumbSource 预读改 **开**：
seek 后是顺序读簇，预读一次往返喂满窗口；该源独立于主读，不互顶（252 的教训
只适用于**共用**源）。

## 三、画中画（#10）

现状：`enterPictureInPictureMode(空参数)` + manifest **没有** `supportsPictureInPicture`
⇒ 永远失败，toast「当前设备不支持画中画」（其实是自己没声明）。

1. Manifest：PlayerComposeActivity 加 `android:supportsPictureInPicture="true"`。
2. 新文件 `compose/PlayerPip.kt`（单一职责：PiP 参数 / 进入 / 退出态）：
   - `updatePipParams()`：宽高比取 mpv `video-params/w/h`（读不到退 16:9），
     夹在系统允许区间 [100:239, 239:100]；`sourceRectHint` = decorView 全幅；
     **API31+ `setAutoEnterEnabled(true)`** ⇒ 播放中按 Home 自动进 PiP（K40=API33 ✓）。
   - `enterPictureInPicture()`：先更新参数再进入；失败给可见 toast。
   - `onPipModeChanged(inPip)`：进 → 关面板/sheet、收 chrome；出 → 恢复控制条。
3. Activity：`pipActive` 态 + `onPictureInPictureModeChanged` 覆写；
   三条起播链路 ready 后各调一次 `updatePipParams()`。
4. `PlayerOverlay(pipActive)`：PiP 内**不画任何 chrome、不挂手势层**
   （小窗点击交给系统 PiP 控件）；Root 的弹幕层 / CastSheet 同步隐。
5. 会话已有 Foreground Service 托管（214）⇒ PiP 期间播放不死，无需新机制。
6. **v3.1c 静默拒绝看门狗**：真机实测 manifest/appops 全对、tap 命中图标、
   `enterPictureInPictureMode` 不抛异常，但系统**不进小窗**（MIUI 静默拒绝）。
   请求后 1.5s 核对 `onPictureInPictureModeChanged` 是否来过（`pipEnterConfirmed`），
   没来 ⇒ toast「系统未进入画中画：请在系统设置里允许本应用使用画中画」+
   `smbTrace` 落痕（pip enter requested / watchdog / mode-changed 三条）。
   铁律：不许静默降级 —— 系统拒绝也必须用户可见。

## 四、弹幕（#11）

现状盘点（先修断线，再做设计）：
- `DanmakuOverlay` 已挂载，但 Root **没传 speedSec** ⇒「滚动时长」stepper 是死的；
- `onPanelStep` 把 `danmaku-speed` 当 **mpv 属性**写 ⇒ 必然失败（它根本不是 mpv 属性）；
- `loadDanmaku` 只在 SMB 链路调，本地片源不加载；
- 解析器认 mode=4/5（底部/顶部固定弹幕），渲染层却一律滚动；
- 无弹幕文件时开关照样「已开启」⇒ 违反不许静默降级。

**设计**：
1. 新文件 `danmaku/DanmakuSettings.kt`：`data class DanmakuSettings(speedSec=8, fontScale=1,
   opacity=1, area=FULL, hasItems=false)` + `enum DanmakuArea { FULL, TOP_HALF, BOTTOM_HALF }`。
   Activity 用 `danmakuSettings` 取代散落的 `danmakuSpeedSec`。
2. `DanmakuOverlay` 真接全部设置：字号倍率、不透明度（描边+填充同 alpha）、
   显示区域（全屏/上半/下半，轨道数按区域算）、滚动时长；
   **mode 4/5 画固定弹幕**（居中、停留 4s、按 hash 分槽）。
3. 面板：
   - `danmakuOps(on, hasItems)`：显示设置> / 过滤关键词（置灰：词表未接入）/
     开启弹幕开关 —— **无文件时置灰 + 原因**「未找到弹幕文件（danmaku/<片名>.xml）」。
   - `danmakuSettings(settings)`：滚动时长 4–16s（0.5 步长）/ 字号 0.6–1.6×（0.1）/
     不透明度 30–100%（10）三个 stepper + 全屏/上半屏/下半屏三行单选。全部真接渲染层。
   - stepper 走**本地写**（新 LOCAL_STEP_UNITS），不再碰 mpv；重置也本地。
4. 顶栏弹幕图标：无文件时点击 → toast 说明原因，不开空开关。
5. 本地起播也 `loadDanmaku`。
6. 取证：推一份 B 站 XML 样例到设备 `files/danmaku/<片名>.xml`，截渲染/设置/区域三态。

## 五、丝滑度总调校（#14）

新文件 `player/PlayerMotion.kt`：**全页动效常量唯一来源**，替换散落字面量：

| 部件 | 进 | 出 | 曲线 |
| --- | --- | --- | --- |
| 控制条 | 180ms fade+位移 | 140ms | FastOutSlowIn |
| 浮层面板 | 200ms fade+0.94 缩放 | 150ms | FastOutSlowIn |
| 面板钻入换内容 | 150ms crossfade | 120ms | 同上 |
| 轻提示 | 200ms fade | 250ms | 同上 |
| 轨道变粗 | 150ms | 150ms | 同上 |
| 预览浮窗 | 120ms fade+0.92 缩放 | 100ms | 同上 |

- G1 降级保留：系统「移除动画」⇒ 位移/缩放归零，只留 alpha（150ms）。
- 面板关闭淡出期间**内容保持**（记住 lastKind），修「壳在淡出、内容先没」的闪断。
- 验收：K40 拖动进度条期间 `dumpsys gfxinfo` 帧统计 + 逐态截图。

## 六、合规拆分

`PlayerBottomBar.kt` 319 行 > 300 ⇒ 拆出 `player/PlayerBarText.kt`
（TimeText / BarTextButton / formatPlayerTime，≈100 行），底栏本体降到 ≈225 行。

## 七、文件家族树（新增 ★ / 修改 △）

```
android/app/src/main/
├── AndroidManifest.xml                       △ supportsPictureInPicture
└── java/com/kivo/cinema/
    ├── thumbnail/
    │   ├── ThumbnailReader.kt                △ 双输入源 + latest-wins（≤300）
    │   ├── ThumbnailSource.kt                ★ ByteSource→MediaDataSource + ThumbInput
    │   └── ThumbnailYuv.kt                   ★ YUV→Bitmap（自 Reader 拆出）
    ├── danmaku/
    │   ├── DanmakuSettings.kt                ★ 设置数据类 + 区域枚举
    │   └── DanmakuOverlay.kt                 △ 字号/透明度/区域/固定弹幕
    └── ui/compose/
        ├── PlayerPip.kt                      ★ PiP 参数/进入/退出
        ├── PlayerComposeActivity.kt          △ pipActive / danmakuSettings / smbThumbSource
        ├── PlayerComposeRoot.kt              △ 接线（settings、pipActive、scrub 节流）
        ├── PlayerSmbStart.kt                 △ 缩略图第二读路径
        ├── PlayerSessionStart.kt             △ 本地弹幕加载 + PiP 参数
        ├── PlayerSessionTeardown.kt          △ 关缩略图源
        ├── PlayerActionRouter.kt             △ cast 入口 / 弹幕诚实开关 / PiP 迁出
        ├── PlayerPanelRouter.kt              △ 弹幕本地 stepper / 区域选择
        └── player/
            ├── PlayerBarText.kt              ★ 底栏文字件（自 BottomBar 拆出）
            ├── PlayerMotion.kt               ★ 动效常量唯一来源
            ├── PlayerBottomBar.kt            △ 瘦身
            ├── PlayerSeekTrack.kt            △ 变粗动画
            ├── PlayerSeekPreview.kt          △ 入场动画
            ├── PlayerTopBar.kt               △ cast 图标
            ├── PlayerGlyph.kt                △ cast key
            ├── PlayerGlyphFeature.kt         △ cast 形状（airplay 删除）
            ├── PlayerPanelForms.kt           △ cast 删除 / 弹幕诚实行
            ├── PlayerPanelItems.kt           △ settings 传参 / cast 删除
            ├── PlayerPanelData.kt            △ danmakuSettings(settings)
            ├── PlayerPanelHost.kt            △ cast 锚点删除
            ├── PlayerTokens.kt               △ trackActiveHeight / castMenuWidth 删除
            ├── PlayerOverlay.kt              △ pipActive 门 + 钻入 crossfade + lastKind
            ├── PlayerBarsHost.kt             △ 动效常量引用
            └── PlayerToast.kt                △ 动效常量引用
```

## 八、证据计划（docs/evidence/p240/player-ui/v3.1/）

| 文件 | 判据 |
| --- | --- |
| cast-icon.png | 顶栏首枚为通用投屏符，无 Apple 三角；点开 = DLNA sheet |
| track-rest.png / track-drag.png | 6pt vs 10pt（ ink 行探针复算） |
| preview-frame.png + thumbnail-evidence.json | SMB 拖动浮窗**有画面帧**；cold/warm 延迟可复算 |
| pip-window.png + pip-logcat.txt | 小窗在播、chrome 全收；Home 自动进 PiP |
| danmaku-on.png / danmaku-settings.png / danmaku-area.png | 渲染可见、设置真生效 |
| gfxinfo.txt | 拖动期间 jank 率 |
| metrics.txt | 探针命令 + 安装协议（force-stop 重启） |

## 九、BACK 层级（v3.1b 新增）

真机实测缺陷：全 app **没有任何 BACK 处理** ⇒ 面板开着按 BACK 直接退出播放
（测试序列里一次 BACK 就 finish 了 Activity）。REX 同序习惯：先收浮层再退播放。

层级：**面板 → 投屏 sheet → 控制条 → 退出**。
`PlayerActionRouter.handleBack(): Boolean`（消费=true）+ Activity
`onBackPressedDispatcher.addCallback`：未消费时禁用回调再转发默认 finish。
关面板/sheet 刷新 lastTouchMs（与 onDismissPanel 同语义：关浮层也是一次交互）。

## 十、v3.1b 家族树增量（★ 新 / △ 改）

```
java/com/kivo/cinema/
├── thumbnail/
│   ├── PreviewFrameReader.kt      ★ 两实现共同契约（含 onUnavailable）
│   ├── MpvThumbnailReader.kt      ★ mpv 第二实例预览帧（网络/无索引源）
│   └── ThumbnailReader.kt         △ 实现接口 + 失败报原因
└── ui/compose/
    ├── PlayerComposeActivity.kt   △ 字段接口化 + reason 态 + BACK 回调 + 收尾顺序
    ├── PlayerActionRouter.kt      △ handleBack 层级
    ├── PlayerSmbStart.kt          △ mount 换 MpvThumbnailReader
    ├── PlayerComposeRoot.kt       △ reason 接线
    └── player/
        ├── PlayerOverlay.kt       △ reason 参数
        ├── PlayerBarsHost.kt      △ reason 参数
        ├── PlayerBottomBar.kt     △ reason 参数
        ├── PlayerSeekTrack.kt     △ reason 参数
        └── PlayerSeekPreview.kt   △ 空框原因小字
```

证据增量（同目录）：`v31-thumb-smb-drag.json`（换核前 handler busy 定论）、
`preview-frame-smb.png`（换核后浮窗有帧）、`thumb-evidence-after.json`（cold/warm 可复算）、
`back-layers.png`（面板开→BACK 收面板不退播放）。