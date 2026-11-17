package com.kivo.cinema.ui.shell

/**
 * 主 Tab：与竞品实拍一致（发现/我的/资源库/设置），[page] 对应既有 Shell 路由值。
 *
 * 声明顺序 = 路由值分组，**不是**胶囊显示顺序：`entries` 为 Library/Discover/Profile/Settings，
 * 而胶囊条从左到右是 发现/我的/资源库/设置。故显示顺序单独由 [displayOrder] 给出，
 * 且 `entries.ordinal` 不能当作胶囊下标（两者不同构）。
 */
enum class ShellTab(val label: String, val page: Int) {
    Library("资源库", 0),
    Discover("发现", 1),
    Profile("我的", 3),
    Settings("设置", 4);

    companion object {
        /** 胶囊条显示顺序。✅ 实拍 `docs/audit/golden/rex/2026_11_16_21_22_14_IMG_0366.jpg` */
        val displayOrder = listOf(Discover, Profile, Library, Settings)

        /**
         * 「在看」旧路由值（page=2）。B2 已把「在看」迁到「我的」模块的二级覆盖层
         * （`MainActivityShell.showWatching`），**不再参与 Tab 路由**；保留常量仅为兼容
         * 历史证据与外部调用方，新增代码不得使用。
         */
        const val WatchingPageCode = 2
    }
}