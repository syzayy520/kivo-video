package com.kivo.cinema.library

import android.content.Context
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbCredential
import com.kivo.cinema.smb.SmbSession
import com.kivo.cinema.smb.SmbShareLister
import com.kivo.cinema.smb.scan.SmbLibraryScanner
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore

/**
 * NAS 片库扫描的总入口：已保存源 → 共享 → 影视目录 → 索引条目。
 *
 * 单一职责：串起"源/共享/目录/扫描器"，产出 [NasVideo] 列表；不写索引（交给 [NasIndexStore]）、
 * 不碰 UI、不抽海报。阻塞（大量 SMB 列目录）：必须在 IO 线程执行。
 *
 * 只扫影视共享与影视根目录，**不扫家目录/下载/系统共享**——真机实测 `homes` `Download`
 * `PlexMediaServer` 之类的共享里混着非影视内容，全扫既慢又会把首页弄脏。
 */
object NasScanService {

    /** 影视内容所在的共享（真机实测 12T/1T/4T）；其余共享一律不进片库。 */
    private val LIBRARY_SHARES = setOf("12T", "1T", "4T")

    /** 影视根目录候选（分类由目录名决定）；共享里不存在的会因 list 失败被扫描器静默跳过。 */
    private val ROOT_DIRS = listOf(
        "电影", "REMUX", "原盘", "剧集", "电视剧", "综艺", "动漫", "动画片", "番剧", "演唱会", "音乐", "纪录片",
    )

    fun scanAll(context: Context, maxFilesPerShare: Int = 2000): List<NasVideo> {
        val records = runCatching {
            SourceRepository(SourceStore(context)).read().getOrElse { emptyList() }
        }.getOrElse { emptyList() }

        val out = ArrayList<NasVideo>(512)
        records.filter { it.isSmb && !it.hidden }.forEach { rec ->
            runCatching {
                val addr = SmbAddress.parse(rec.address)
                val credential = SmbCredential(username = rec.username, password = rec.password)
                val shares = if (addr.share.isNotBlank()) listOf(addr.share) else {
                    SmbShareLister(addr.host, addr.port, credential).list()
                        .filter { it.isDisk }.map { it.name }
                }
                shares.filter { it in LIBRARY_SHARES }.forEach { share ->
                    SmbSession(SmbAddress(addr.host, addr.port, share), credential).use { session ->
                        session.connect()
                        SmbLibraryScanner(session)
                            .scan(
                                ROOT_DIRS.mapNotNull { name ->
                                    NasCategory.fromDirName(name)?.let { SmbLibraryScanner.Root(name, it) }
                                },
                                maxFiles = maxFilesPerShare,
                            )
                            .forEach { out.add(NasVideo(rec.id, share, it.path, it.size, 0L, it.category, it.posterPath)) }
                    }
                }
            }
        }
        return out
    }
}
