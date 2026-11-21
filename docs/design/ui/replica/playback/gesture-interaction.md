# 播放页手势交互落地

## 范围与门闸

2026-11-19 用户授权在 C:/kivo video 现有 Android 项目按③→①→②实施；不新建项目、不复制工程、不创建 worktree，保留其他助手工作。先明确下述自然家族树，再实现。每个新增/修改 Kotlin 文件不超过 300 行，单文件单职责。只动手势识别、展示及会话控制接线，不动 Surface/JNI/C++、解码和网络取数策略。

当前基线：PlayerGestureLayer 四个独立识别器竞争；PlayerSeekTrack 起手还要横向等待，拖动加粗；全屏横滑与进度条预览调度不同；双击同侧重复不能可靠重启动画，旧延时能清掉新反馈；长按先显示状态后检查写入成功。已有单击是控制条显隐（面板优先关闭），中间双击播放/暂停，左右双击按用户配置步长跳转，不擅自改为单击暂停。

## 自然家族树

```text
android/app/src/main/java/com/kivo/cinema/
  ui/compose/
    PlayerComposeActivity.kt       现有状态所有者/生命周期，薄接线
    PlayerComposeRoot.kt           现有播放层组合和手势回调接线（scrub settle/停留效果）
    PlayerGestureRouter.kt         竖滑系统值接线（连续累积量与量化读回分离）
    PlayerActionRouter.kt          顶栏/底栏/状态层点击动作（seek 与双击已按职责分出）
    PlayerLifecycle.kt             cancelPlayerGestures：失焦/后台/离场统一取消入口
    PlayerProbeHooks.kt            取证探针接 scrub（previewRequestedMs 去重，整毫秒对齐）
    PlayerProbeStress.kt           p215 压测 harness（按证据目标从 ProbeHooks 分出，守 300 行）
    gesture/
      PlayerTapActionRouter.kt     双击动作分发；flashEventId 守卫清除，同侧重播
      PlayerSeekGestureRouter.kt   横滑与进度条共同的 scrub/预览接线（边界从夹紧落点推进）
      PlayerSeekCommand.kt         统一 seek 入口：夹紧/落点预取/等待序号/四链路分发
      PlayerSpeedGestureRouter.kt  Activity/mpv 适配：Host 工厂与 MpvSpeedPort
      PlayerSpeedGestureController.kt 纯逻辑倍速事务：串行 worker/恢复屏障/身份守卫
    player/
      PlayerGestureLayer.kt        Compose输入宿主，唯一单指识别入口
      PlayerGestureFlash.kt        双击定向反馈
      PlayerGestureHud.kt          亮度/音量信息面板
      PlayerLongPressHud.kt        倍速状态与一次性箭头微动效
      PlayerSeekTrack.kt           进度条触摸与绘制编排
      PlayerSeekPreview.kt         画面/目标时间/总时长/位移量
      PlayerOverlay.kt             保持原层序与控件优先命中
      PlayerMotion.kt              统一动效参数
      gesture/
        PlayerGestureDecision.kt  无Android依赖的方向及点击判定规则
        PlayerGestureDetector.kt  单指状态机：待定/横滑/竖滑/长按/取消
        PlayerTapSequence.kt      单击延迟、第二击挂起及取消
      seek/
        PlayerSeekRail.kt          细轨/滑块/起点刻度/区间绘制
        PlayerScrubState.kt        一次拖动与松手信息停留状态
      feedback/
        PlayerFeedbackSurface.kt  手势HUD雾面材质，不改其他面板
        PlayerLevelMeter.kt       当前值填充/起点/边界反馈绘制
  thumbnail/
    PreviewRequest.kt              不可变请求身份 + 每 reader 独占代际闸门
    PreviewFrameReader.kt          回调带请求身份；invalidate/isCurrent
    ThumbnailReader.kt             本地解码：latest-wins 贯穿排队/解码/回调
    MpvThumbnailReader.kt          mpv 截帧：同构请求闸门
android/app/src/test/java/com/kivo/cinema/
  ui/compose/player/gesture/
    PlayerGestureDecisionTest.kt   方向、同侧双击、边界与取消判据
    PlayerGestureDetectorTest.kt   状态机：380ms 长按/方向锁定/取消派发
    PlayerTapSequenceTest.kt       首击等待/第二击挂起/滑动长按取消
  ui/compose/gesture/
    PlayerSpeedGestureControllerTest.kt 迟到 enter/恢复屏障/keep/锁定/身份切换
  thumbnail/
    PreviewRequestTest.kt          请求代际与身份校验
```

允许按实际代码收敛文件数量，不为单个调用方创建无语义空壳；不得为凑300行机械切片。现有文件优先原位修改，新增文件必须落到上述职责家族。

## ③ 点击与双击

单击保留控制条/面板语义。首击释放后等待240ms；第二击须满足时间窗、同分区、位移距离。第二击按住时挂起首击，拖动或长按成立时取消整个点击候选；取消/多指/离场也不补点击。同侧连续双击每次都重新触发动效，使用事件身份而非仅靠zone判断。左右反馈置于对应触点分区，以两道定向光痕/箭头接力衰减，不全屏闪光；中间播放暂停反馈使用实际播放态。

## ① 进度与竖滑

进度条热区上下各扩大12dp，视觉保持细轨；按下即独占至抬手，无方向等待。非控件区先经过12–15dp（Android逻辑像素）方向判定，方向锁定后不可互转。双指继续交给已有缩放，取消单指提交，不伪造单击。边界后反向立即生效。

滑块直接跟手，时间不插值不吸附到暗设锚点。起点刻度保持本次起点，高亮起点到目标区间。预览卡显示真实帧、目标时间/总时长、相对本次起点的方向与差值；未就绪/不可用明确显示状态原因，不使用伪帧。只展示最新请求的结果，包括主线程排队期间的过时结果。保留网络预取节流/落点等待能力，不用UI改造重写网络层。松手仅滑块外观临界阻尼收拢，落点信息保持400ms后淡出180ms。

竖滑按起手左右分区固定亮度/音量。窄雾面HUD靠对应侧，图标、真实百分比、名称、竖槽和淡起点刻度。窗口亮度不改系统全局，音量读回系统实际档位；计算的连续积累与读回值分离，避免小步拖动被整数档位吞掉。0音量显示静音。仅首次撞边轻提亮；松手400ms停留+180ms淡出。材质以半透明深底、细亮边和柔影实现，不宣称SurfaceView背后真实模糊。

## ② 长按倍速

非控件区380ms、位移未越方向阈值后成立；拖动一旦成立立即取消长按与点击候选。倍速只有内核成功写入并确认实际值才展示；读不到原速不能假定1倍。常规长按不把已有高倍速降到2倍。进入顶部安全区域小胶囊，2×/实际速度+倍速中，双箭头只推进一次，持续按住保持稳定。松手立即按既有恢复设置恢复；用户已明确开启的调速/锁定设置保留，但不允许切为进度/亮度/音量。触摸取消/失焦/后台/离场强制结束临时倍速，不能遗留锁定或待执行计时。恢复失败可见，不伪报成功。退出180ms收拢淡出，不继续保留倍速中。

## 验收

离线Kotlin编译、JVM方向/点击时序测试、所有触及Kt行数与diff核对。真实K40检查单击/左右同侧与跨侧双击，横竖斜滑、进度条边缘/纵偏/越界回拉，多指中断、长按与拖动竞争，松手/取消/后台恢复，起点与落点、预览迟到及不可用、静音/边界反馈、减弱动画。设备为共享资源，装机/切页前确认不打断其他助手。证据归入 docs/evidence/player-gestures/；未验项目明确保留，不以编译通过声称体验完成，不卸载/清数据、不改签名。

## 落地状态（2026-11-19）

- ③①② 代码已全部落位并通过离线 `:app:compileDebugKotlin`；全量 JVM 单测 24 套件 204 例 0 失败（含新增倍速控制器 18 例、检测器 18 例、请求闸门 9 例）。
- 接管修复：倍速控制器缺 `abort` 收敛与 `change` 返回值误用；Root `onScrub` 缺逗号；HUD 枚举残留 SEEK 项；`speedGestureLabel` 缺 java.math 导入；取证探针改接 scrub 并整毫秒对齐（否则新回调身份校验拒发探针位图）。
- **真机验证已做（K40 `b6e950ee`，2026-11-19）**：golden path 与边界取证见
  `docs/evidence/player-gestures/`（判读见该目录 README）。单击显隐、左右/中间双击与同侧重播、
  横滑、滑轨、竖滑 HUD（音量真实读回）、长按倍速、后台恢复、EOS 前向 toast 均真机确认。
- **预览卡落位修复**：用户报「进度信息跑到最右、不在进度条上方」。排查（证据 25–32）证伪五种
  轨道盒内落位机制——BoxWithConstraints 内 align/offset/padding/Popup/自布局都被祖先放置层
  恒定加偏（约 ±60dp）。修复：卡改挂 `PlayerOverlay` 根 Box（HUD/toast 同款 align+padding），
  轨道经 `onGloballyPositioned` 回传窗口坐标，几何唯一来源 `seek/PlayerSeekPreviewAnchor.kt`。
  真机判据全中（33–37）：中心跟滑块（960.5 vs 960）、底边贴轨道上沿 10dp（卡底 719.5）、
  左夹紧 259.5 / 右夹紧 1697.5、抬手 400ms 停留 + 180ms 淡出。
- **未验保留**：本地 mpv 无 thumbnailReader ⇒ 预览卡显示可见 reason「此片源暂不支持预览帧」，
  真实帧/thumbScan 未验；EOS 后回 seek 静默 no-op（k2）根因未定；片头 toast 未复验。
  不以编译通过或落位修复声称全部体验完成。
