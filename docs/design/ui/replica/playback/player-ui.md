# 播放页 UI · 参考取证与实施边界

2026-11-09：用户调整优先级，播放故障由另一位助手跟进，本助手先实现播放页 UI。原 R5 后置安排由此覆盖。

## 构思与取证

以用户提供的 REX 原始截图/录像、仓库商店截图及拆解包为依据，先建立播放画面、控制栏和弹出面板的可见状态清单。截图可证明几何与颜色；字符串只能证明功能文案存在，不能据此捏造布局或动画参数。没有参考的状态明确记录，不宣称像素验收。

先实现横屏控制层：顶部返回/片名/动作、中心传输控制、底部时间轴与工具栏、单击显隐及进度拖动反馈。按实际参考修订具体布局。沿用首页/详情页的克制材质与统一图标。功能入口必须有反馈，未接入能力明确提示。

## 自然家族树与边界

```text
android/app/src/main/java/com/kivo/cinema/ui/compose/
  player/                    # 播放展示组件
    PlayerOverlay.kt         # 显隐、状态、顶部/中心/底部编排
    PlayerGlyph.kt           # 统一矢量图标
    PlayerTimeline.kt        # 时间轴与拖动预览
    PlayerOptions.kt         # 播放选项与明确的能力反馈
  PlayerControls.kt          # 保持旧调用兼容的展示适配
  PlayerComposeActivity.kt   # 仅 UI 接线，保留 Surface / 会话生命周期
docs/evidence/p240/player-ui/
  reference/                 # 参考索引和接触表
  device/                    # 手机/平板截图与操作证据
```

不改 backend、bridge、NetworkPlaybackSession、HttpRangeFile 或解码策略。与另一助手共享宿主文件时，UI 改动限 setContent/展示回调；不重写生产会话。独立展示验收可使用测试宿主与静态状态，不把它称为真实播放成功。

## 本阶段验收

构建通过；手机/平板横屏可见且不侵入切口/手势区；按钮和小字号风格统一；单击显隐、拖动反馈及面板关闭可用；准备/失败状态可读。逐项保存同状态截图，记录参考尺寸与差异。后台解码是否成功单独交接，禁止混为 UI 验收。
