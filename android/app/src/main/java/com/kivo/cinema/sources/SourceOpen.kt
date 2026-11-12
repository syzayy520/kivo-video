package com.kivo.cinema.sources

import android.content.Context
import android.content.Intent
import com.kivo.cinema.emby.EmbyActivity
import com.kivo.cinema.smb.ui.SmbBrowserActivity

/** 协议→浏览页的单一入口：defaultHome 与 onOpenSource 共用，避免两处再分叉。 */
object SourceOpen {
    fun intentFor(context: Context, source: SourceRecord): Intent =
        Intent(context, if (source.isSmb) SmbBrowserActivity::class.java else EmbyActivity::class.java)
            .putExtra("kivo.sourceId", source.id)
}