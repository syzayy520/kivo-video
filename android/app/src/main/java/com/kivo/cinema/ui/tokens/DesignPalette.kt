package com.kivo.cinema.ui.tokens

import androidx.compose.ui.graphics.Color

/**
 * 原始色板（Palette）：竞品五语义色 + 现有在用的原始色，各出明暗两套。
 *
 * 引用方向单向：`DesignPalette` ← `DesignTokens` ← 调用点；页面不得直接读本文件。
 * - Light 全部为既有 `DesignTokens` 的帧实测值，改一个数字就是改视觉，必须真机取证。
 * - Dark 的 `canvasFill/groupedFill/pageBackground/hairline/washFill` 取自竞品说明书 §3.1；
 *   其余为按同一灰阶规律推的暗态，标 `◐ 推断，待真机取色`。
 */
object DesignPalette {

    object Light {
        // ── 竞品五语义色（说明书 §3.1） ──
        val canvasFill = Color(0xFFFFFFFF)      // 卡片/内容画布
        val groupedFill = Color(0xFFF2F2F7)     // 分组列表底
        val pageBackground = Color(0xFFF2F2F7)  // 页面底色（首页 resting/资源库/我的/设置）
        val hairline = Color(0x0D000000)        // 分隔线/描边（黑 5%）
        val washFill = Color(0x80EDEDFB)        // 洗色叠加（P3 @50%）

        // ── 文本 ──
        val textMain = Color(0xFF1A1A1A)
        val textSub = Color(0xFF8A8A8E)

        // ── 控件绿（仅交互控件：toggle/主按钮） ──
        val accentGreen = Color(0xFF34C759)
        val dangerRed = Color(0xFFFF3B30)

        // ── 导航 chrome（269 v3：扁平灰 pill + 蓝选中） ──
        val navPill = Color(0xFFE8E9ED)
        val navIdle = Color(0xFF4B5563)
        val navActive = Color(0xFF0A84FF)
        val navPillDark = Color(0xFF2C2C2E)
        val navIdleDark = Color(0xFFEBEBF5)

        // ── 玻璃 ──
        val glassFallback = Color(0xEEFFFFFF)   // API31 降级 92% 白

        // ── 背景动画渐变三色（05 §二：淡紫→淡粉→米白） ──
        val bgLavender = Color(0xFFEAE4F5)
        val bgPink = Color(0xFFF4E3EA)
        val bgCream = Color(0xFFF7F1E8)
    }

    object Dark {
        // ── 竞品五语义色（说明书 §3.1，已实测） ──
        val canvasFill = Color(0xFF222222)      // 卡片/内容画布
        val groupedFill = Color(0xFF1C1C1E)     // 分组列表底（与私密区暗态实测同值）
        val pageBackground = Color(0xFF000000)  // 页面底色
        val hairline = Color(0x1AFFFFFF)        // 分隔线/描边（白 10%）
        val washFill = Color(0xFF3D3D41)        // 洗色叠加

        // ── 文本 ──
        val textMain = Color(0xFFF2F2F7)        // ◐ 推断，待真机取色
        val textSub = Color(0xFF98989E)         // ◐ 推断，待真机取色（与私密区 onSurfaceVariant 同值）

        // ── 控件绿 ──
        val accentGreen = Color(0xFF34C759)     // 品牌绿，明暗同值
        val dangerRed = Color(0xFFFF453A)       // ◐ 推断，待真机取色（暗态红提亮）

        // ── 导航 chrome ──
        val navPill = Color(0xFF2C2C2E)         // ◐ 推断，待真机取色
        val navIdle = Color(0xFF98989E)         // ◐ 推断，待真机取色（与 textSub 同值）
        val navActive = Color(0xFF0A84FF)       // 系统蓝，明暗同值
        val navPillDark = Color(0xFF2C2C2E)     // 与 navPill 同值（命名沿用旧令牌）
        val navIdleDark = Color(0xFFEBEBF5)     // 与浅色态同值远高于暗底，供暗态导航图标用

        // ── 玻璃 ──
        val glassFallback = Color(0xEE1C1C1E)   // ◐ 推断，待真机取色（暗态近黑 92%）

        // ── 背景动画渐变三色（暗态：同色相压暗） ──
        val bgLavender = Color(0xFF2E2A38)      // ◐ 推断，待真机取色
        val bgPink = Color(0xFF33262E)          // ◐ 推断，待真机取色
        val bgCream = Color(0xFF2E2A24)         // ◐ 推断，待真机取色
    }

    /**
     * 六页页面底色渐变端点（每页一组两色，垂直渐变）。
     *
     * Light 全部为 REX 真机帧实测（`docs/design/rex-dna-browse.md` §1.4 A 表，
     * 采样 y≈60 / y≈700；见 `kivo-play-baseline.md` §2「六页底色」）。
     * Dark 无实测帧，按同色相压暗到近黑推出，逐条标 `◐ 推断，待真机取色`。
     * 本对象只描述令牌；页面接入由 B-2/B-3/B-5 负责。
     */
    object PageGradient {

        object Light {
            // 发现（0332）：顶部 #F0ECF6 → 底部 #EFEEF5
            val discoverStart = Color(0xFFF0ECF6)
            val discoverEnd = Color(0xFFEFEEF5)

            // 资源库（0366）：#F8EBEE → #E5E7F6（粉 → 蓝紫）
            val libraryStart = Color(0xFFF8EBEE)
            val libraryEnd = Color(0xFFE5E7F6)

            // 我的（0357/0358）：#E5E4F4 → #F8F8FD（淡紫 → 近白）
            val profileStart = Color(0xFFE5E4F4)
            val profileEnd = Color(0xFFF8F8FD)

            // 设置（0374）：#F3EFF7 → #EFE5EC
            val settingsStart = Color(0xFFF3EFF7)
            val settingsEnd = Color(0xFFEFE5EC)

            // 详情：跟来源页，待 B-5 接入（暂取发现色）
            val detailStart = discoverStart
            val detailEnd = discoverEnd

            // 日历：跟「我的」，待 B-4 接入（暂取我的色）
            val calendarStart = profileStart
            val calendarEnd = profileEnd
        }

        object Dark {
            // 发现：◐ 推断，待真机取色
            val discoverStart = Color(0xFF12101A)
            val discoverEnd = Color(0xFF0E0E13)

            // 资源库：◐ 推断，待真机取色（冷蓝紫向，与浅态蓝紫终点同色相）
            val libraryStart = Color(0xFF1A1014)
            val libraryEnd = Color(0xFF101322)

            // 我的：◐ 推断，待真机取色（淡紫向）
            val profileStart = Color(0xFF121022)
            val profileEnd = Color(0xFF161620)

            // 设置：◐ 推断，待真机取色（粉紫向）
            val settingsStart = Color(0xFF17121A)
            val settingsEnd = Color(0xFF171017)

            // 详情：跟来源页，待 B-5 接入（暂取发现暗态）
            val detailStart = discoverStart
            val detailEnd = discoverEnd

            // 日历：跟「我的」，待 B-4 接入（暂取我的暗态）
            val calendarStart = profileStart
            val calendarEnd = profileEnd
        }
    }
}