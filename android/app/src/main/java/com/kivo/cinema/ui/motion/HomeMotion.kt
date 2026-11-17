package com.kivo.cinema.ui.motion

/**
 * 首页动效常量的**唯一来源**（只放首页，不含播放器；播放器见 PlayerMotion）。
 *
 * 出处等级沿用竞品拆解口径：✅ 确定 / ◐ 推断。
 * 这里没有 ✅ 的首页精确时长——竞品主二进制 FairPlay 加密，
 * SwiftUI 的 duration 常量不可静态读取，逐条依据见各常量注释。
 */
object HomeMotion {

    /**
     * 卡片按压时的缩放下限（按压 → 0.97 → 松手回 1.0）。
     * ◐ 推断：竞品动效参数表 M04-S01/S02 只给了轮播与拖拽抬升（1.03），
     * 首页卡片无按压数值；按 iOS 按钮 press spring 惯例取 0.97。
     */
    const val pressureScale = 0.97f

    /**
     * 分页点（hero 指示点）尺寸/选中态切换时长。
     * ◐ 推断：竞品 M04-S01 仅确定"分页点随选中态变化"，未给时长；
     * 推断文档另记 d=0.2s（同样是 ◐），此处取更克制的 150ms。
     */
    const val dotSizeMs = 150

    /**
     * hero 视差位移时长；0 = **当前不做视差**。
     * 出处：竞品 M04-S01 的 hero 只有整页 Δx 平移 + 自动轮播，无视差参数，
     * 故显式写 0 —— 防止后续被当成"漏了"而随手补一个值。
     */
    const val heroParallaxMs = 0f

    /**
     * 首页分区揭示（进入视口/首次布局）的单区时长。
     * ◐ 推断：竞品 M04-S03 展开/收起为 0.3s、P12 清单揭示单项 0.3s + stagger，
     * 首页整区揭示取 200ms（比单项略快，避免与轮播节奏抢注意力）。
     */
    const val sectionRevealMs = 200

}