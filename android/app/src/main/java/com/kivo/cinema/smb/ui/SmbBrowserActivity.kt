package com.kivo.cinema.smb.ui

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.smb.*

/**
 * 已保存 SMB 源的浏览页（243 S4）：列出该源的目录、点进去、点开播；地址没写共享名时先枚举让用户选。
 * 协议细节全在 [SmbSession]；凭据从加密存储按 `sourceId` 取，不经过 Intent（匿名账号可能只看到少数共享，见 D1）。
 */
class SmbBrowserActivity : ComponentActivity() {

    /**
     * 跨线程可见性：`record`/`session` 由后台线程写入、UI 线程读取。
     * 不用 `@Volatile` 的话，UI 可能读到 null 而报"会话未就绪"。
     */
    @Volatile private var record: SourceRecord? = null
    @Volatile private var session: SmbSession? = null

    /** 当前连着的共享名：源里可能没写（枚举形态），启动播放器必须显式带过去，否则播放器判成"未选择共享"。 */
    @Volatile private var activeShare: String = ""
    private var path by mutableStateOf("")
    private var entries by mutableStateOf<List<SmbEntry>>(emptyList())
    private var shares by mutableStateOf<List<SmbShare>>(emptyList())
    private var status by mutableStateOf("正在连接…")
    private var loading by mutableStateOf(true)
    private var error by mutableStateOf<String?>(null)

    /**
     * 目录请求序号。
     *
     * 每点一次目录/上一级就 +1；结果回来时只有"最新一次"才允许写进 UI。
     * 没有这个的话，快速连点会让先发的慢请求后到，把当前目录覆盖成上一个目录的内容 ——
     * 标题显示 /视频/电影，列表却是 /视频 的。
     */
    private val loadSeq = java.util.concurrent.atomic.AtomicInteger()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { BrowserScreen() }
        val sourceId = intent.getStringExtra(EXTRA_SOURCE_ID)
        if (sourceId.isNullOrBlank()) {
            error = "缺少源标识，请从资源库重新进入"
            loading = false
            return
        }
        Thread {
            val loaded = SourceStore(this).read().getOrNull()
                ?.firstOrNull { it.id == sourceId }
            if (loaded == null) {
                runOnUiThread { error = "找不到这个资源库，可能已被移除"; loading = false }
                return@Thread
            }
            record = loaded
            val address = runCatching { SmbAddress.parse(loaded.address) }.getOrNull()
            if (address == null) {
                runOnUiThread { error = "保存的地址无法解析"; loading = false }
                return@Thread
            }
            val credential = SmbCredential(username = loaded.username, password = loaded.password)
            if (address.needsShare) {
                loadShares(address, credential)
            } else {
                connect(address, credential)
            }
        }.start()
    }

    private fun loadShares(address: SmbAddress, credential: SmbCredential) {
        runOnUiThread { status = "正在读取共享列表…" }
        try {
            val list = SmbShareLister(address.host, address.port, credential).list()
            runOnUiThread {
                shares = list
                loading = false
                status = if (list.isEmpty()) "服务器没有列出共享，请编辑这个源并写明共享名" else "选择一个共享"
            }
        } catch (e: Exception) {
            runOnUiThread { error = e.message ?: "无法读取共享列表"; loading = false }
        }
    }

    private fun connect(address: SmbAddress, credential: SmbCredential) {
        try {
            val opened = SmbSession(address, credential)
            opened.connect()
            activeShare = address.share
            session = opened
            load(address.path)
        } catch (e: Exception) {
            runOnUiThread { error = e.message ?: "无法连接共享"; loading = false }
        }
    }

    private fun onSharePicked(share: SmbShare) {
        val current = record ?: return
        val base = runCatching { SmbAddress.parse(current.address) }.getOrNull() ?: return
        val target = base.copy(share = share.name, path = "")
        loading = true
        error = null
        shares = emptyList()
        status = "正在连接 ${share.name}…"
        Thread {
            runCatching { session?.close() }
            session = null
            connect(target, SmbCredential(username = current.username, password = current.password))
        }.start()
    }

    /** 列目录。工作线程调用，UI 状态回主线程；只有最新一次请求的结果会被采用。 */
    private fun load(subPath: String) {
        val active = session
        if (active == null) {
            runOnUiThread { error = "会话未就绪"; loading = false }
            return
        }
        val seq = loadSeq.incrementAndGet()
        Thread {
            try {
                val listing = active.list(subPath)
                runOnUiThread {
                    if (isFinishing || isDestroyed || seq != loadSeq.get()) return@runOnUiThread
                    path = subPath
                    entries = listing
                    loading = false
                    error = null
                    status = if (listing.isEmpty()) "这个目录中没有子目录或视频文件" else "/$subPath"
                }
            } catch (e: Exception) {
                runOnUiThread {
                    if (isFinishing || isDestroyed || seq != loadSeq.get()) return@runOnUiThread
                    error = e.message ?: "无法读取文件夹"
                    loading = false
                }
            }
        }.start()
    }

    private fun open(entry: SmbEntry) {
        if (isFinishing || isDestroyed) return
        val current = record ?: return
        val full = if (path.isEmpty()) entry.name else "$path/${entry.name}"
        startActivity(
            Intent(this, com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
                .putExtra("kivo.sourceId", current.id)
                .putExtra("kivo.smbShare", activeShare)
                .putExtra("kivo.smbPath", full)
                .putExtra("kivo.label", entry.name)
        )
    }

    private fun goUp() {
        if (path.isEmpty()) return
        loading = true
        load(path.substringBeforeLast('/', missingDelimiterValue = ""))
    }

    override fun onDestroy() {
        // 不能在主线程 close：锁可能被一次目录列举（网络往返）持有，join 就是 UI 线程等网络。
        // 先摘引用让在途结果作废，再丢后台线程收尾。
        val active = session
        session = null
        loadSeq.incrementAndGet()
        if (active != null) Thread { runCatching { active.close() } }.start()
        super.onDestroy()
    }

    @Composable
    private fun BrowserScreen() {
        Surface(Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                Column(Modifier.fillMaxSize().padding(horizontal = 16.dp)) {
                    Row(Modifier.fillMaxWidth().padding(vertical = 12.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        TextButton(onClick = { finish() }) { Text("返回") }
                        Column(Modifier.weight(1f)) {
                            Text(record?.name ?: "SMB", fontSize = 18.sp, fontWeight = FontWeight.Medium)
                            Text(status, fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                                maxLines = 1)
                        }
                        TextButton(onClick = { goUp() }, enabled = path.isNotEmpty() && !loading) {
                            Text("上一级")
                        }
                    }
                    if (loading) LinearProgressIndicator(Modifier.fillMaxWidth())
                    error?.let {
                        Text(it, color = MaterialTheme.colorScheme.error, fontSize = 13.sp,
                            modifier = Modifier.padding(vertical = 8.dp))
                    }
                    if (shares.isNotEmpty()) {
                        ShareList(shares) { onSharePicked(it) }
                    } else {
                        EntryList(entries) { entry ->
                            if (entry.isDirectory) {
                                loading = true
                                load(if (path.isEmpty()) entry.name else "$path/${entry.name}")
                            } else {
                                open(entry)
                            }
                        }
                    }
                }
        }
    }

    @Composable
    private fun ShareList(items: List<SmbShare>, onPick: (SmbShare) -> Unit) {
        LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            items(items) { share ->
                Row(Modifier.fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(12.dp))
                    .clickable { onPick(share) }
                    .padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
                    Column(Modifier.weight(1f)) {
                        Text(share.name, fontSize = 15.sp)
                        if (share.remark.isNotBlank()) {
                            Text(share.remark, fontSize = 12.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    }
                    Text("›", color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
            }
        }
    }

    @Composable
    private fun EntryList(items: List<SmbEntry>, onOpen: (SmbEntry) -> Unit) {
        LazyColumn(verticalArrangement = Arrangement.spacedBy(6.dp)) {
            items(items) { entry ->
                Row(Modifier.fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(10.dp))
                    .clickable { onOpen(entry) }
                    .padding(horizontal = 14.dp, vertical = 12.dp),
                    verticalAlignment = Alignment.CenterVertically) {
                    Text(if (entry.isDirectory) "目录" else "文件", fontSize = 11.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.width(36.dp))
                    Column(Modifier.weight(1f)) {
                        Text(entry.name, fontSize = 14.sp, maxLines = 2)
                        if (!entry.isDirectory) {
                            Text(formatSize(entry.size), fontSize = 11.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    }
                    if (entry.isDirectory) Text("›", color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
            }
        }
    }

    private fun formatSize(bytes: Long): String = when {
        bytes >= 1L shl 30 -> "%.1f GB".format(bytes.toDouble() / (1L shl 30))
        bytes >= 1L shl 20 -> "%.1f MB".format(bytes.toDouble() / (1L shl 20))
        bytes >= 1L shl 10 -> "%.1f KB".format(bytes.toDouble() / (1L shl 10))
        else -> "$bytes B"
    }

    companion object {
        const val EXTRA_SOURCE_ID = "kivo.sourceId"
    }
}
