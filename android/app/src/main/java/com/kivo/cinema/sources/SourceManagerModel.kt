package com.kivo.cinema.sources

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.sources.storage.SourceStore
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.net.URI

/** Draft lifetime and connection/save orchestration; never stores credentials in saved state. */
class SourceManagerModel internal constructor(app: Application,
    private val repository: SourceRepository,
    private val authenticate: (SourceRecord) -> Unit,
) : AndroidViewModel(app) {
    constructor(app: Application) : this(app, SourceRepository(SourceStore(app)), { record ->
        if (record.protocol == SourceRecord.PROTOCOL_SMB) verifySmb(record)
        else EmbyClient(record.address).login(record.username, record.password)
    })
    var records by mutableStateOf<List<SourceRecord>>(emptyList()); private set
    val visibleRecords get() = records.filter { com.kivo.cinema.sources.access.SourceEligibility.visible(it,
        com.kivo.cinema.sources.access.SourceAccessSession.unlocked) }
    val summaries = com.kivo.cinema.sources.overview.SourceSummaries(viewModelScope) { visibleRecords }
    var ready by mutableStateOf(false); private set
    var loadError by mutableStateOf<String?>(null); private set
    var error by mutableStateOf<String?>(null); private set
    var editing by mutableStateOf(false); private set
    var verifying by mutableStateOf(false); private set
    var saving by mutableStateOf(false); private set
    var name by mutableStateOf("")
    var address by mutableStateOf("")
    var username by mutableStateOf("")
    var password by mutableStateOf("")
    var note by mutableStateOf("")
    /** 当前草稿的协议；编辑已有源时由记录带入，新建时由"添加"入口选定。 */
    var protocol by mutableStateOf(SourceRecord.PROTOCOL_EMBY); private set
    private var original: SourceRecord? = null
    private var job: Job? = null
    private var generation = 0
    val busy get() = verifying || saving
    val editingExisting get() = original != null
    val editingId get() = original?.id
    val draftDirty get() = editing && (name != original?.name.orEmpty() ||
        address != original?.address.orEmpty() || username != original?.username.orEmpty() ||
        password != original?.password.orEmpty() || note != original?.note.orEmpty())
    val draftVisible get() = original?.hidden != true || com.kivo.cinema.sources.access.SourceAccessSession.unlocked

    init { refresh() }
    fun refresh() {
        if (busy) return
        val ticket = generation
        viewModelScope.launch {
            val result = withContext(Dispatchers.IO) { repository.read() }
            if (ticket != generation || busy) return@launch
            result.fold({ records = it; loadError = null; ready = true },
                { loadError = safeError(it); ready = false })
        }
    }

    fun edit(record: SourceRecord? = null) {
        if (busy || !ready) return
        if (record?.hidden == true && !com.kivo.cinema.sources.access.SourceAccessSession.unlocked) return
        original = record
        protocol = record?.protocol ?: SourceRecord.PROTOCOL_EMBY
        name = record?.name.orEmpty(); address = record?.address.orEmpty()
        username = record?.username.orEmpty(); password = record?.password.orEmpty()
        note = record?.note.orEmpty()
        error = null; editing = true
    }

    /** 新建源时选定协议（表单字段随协议变化）。 */
    fun startNew(kind: String) {
        if (busy || !ready) return
        original = null
        protocol = if (kind in SourceRecord.PROTOCOLS) kind else SourceRecord.PROTOCOL_EMBY
        name = ""; address = ""; username = ""; password = ""; note = ""
        error = null; editing = true
    }

    fun dismiss() {
        if (saving) return
        ++generation; job?.cancel(); verifying = false; editing = false
        clearDraft()
    }

    fun applyPreview(p: SourceShareParser.Preview, selected: String) {
        if (busy || selected !in p.addresses) return
        address = selected
        p.name?.let { name = it }; p.username?.let { username = it }; p.password?.let { password = it }
        error = null
    }

    fun save() {
        if (busy || !ready) return
        if (!draftVisible) { error = "请先解锁资源库"; return }
        val normalized = runCatching { SourceAddress.normalize(address, protocol) }.getOrElse {
            error = it.message?.takeIf { m -> m.isNotBlank() } ?: "请检查完整服务器地址"; return
        }
        // Emby 必须有账号；SMB 允许匿名（用户名留空 = 匿名访问）。
        if (protocol == SourceRecord.PROTOCOL_EMBY && username.isBlank()) { error = "请输入账号"; return }
        val previous = original
        val draft = SourceRecord(id = previous?.id ?: java.util.UUID.randomUUID().toString(),
            revision = (previous?.revision ?: 0) + 1,
            name = name.trim().ifEmpty { defaultName(normalized) }, address = normalized,
            username = username.trim(), password = password, protocol = protocol, note = note.trim())
        if (records.any { it.id != draft.id && it.sameAccount(draft) }) {
            error = "此服务器的该账号已添加"; return
        }
        error = null; verifying = true
        val ticket = ++generation
        job = viewModelScope.launch {
            val authentication = withContext(Dispatchers.IO) {
                runCatching { authenticate(draft) }
            }
            if (ticket != generation) return@launch
            verifying = false
            if (authentication.isFailure) { error = safeError(authentication.exceptionOrNull()!!); return@launch }
            saving = true
            val result = withContext(Dispatchers.IO) { repository.saveVerified(draft, previous?.revision) }
            saving = false
            result.fold({ records = it; editing = false; clearDraft() },
                { error = "连接成功，但未保存。${safeError(it)}" })
        }
    }

    fun remove(record: SourceRecord) {
        if (busy) return
        if (record.hidden && !com.kivo.cinema.sources.access.SourceAccessSession.unlocked) return
        ++generation
        saving = true; error = null
        viewModelScope.launch {
            val result = withContext(Dispatchers.IO) { repository.remove(record.id, record.revision) }
            saving = false
            result.fold({ records = it }, { error = safeError(it) })
        }
    }

    fun reportError(reason: String) { error = reason }

    fun setControl(record: SourceRecord, control: SourceRepository.Control, enabled: Boolean) {
        if (busy || !ready) return
        if ((record.hidden || control == SourceRepository.Control.HIDDEN) &&
            !com.kivo.cinema.sources.access.SourceAccessSession.unlocked) {
            error = "请先解锁资源库"; return
        }
        ++generation; saving = true; error = null
        viewModelScope.launch {
            val result = withContext(Dispatchers.IO) { repository.setControl(record.id, record.revision, control, enabled) }
            saving = false
            result.fold({
                records = it
                if (control == SourceRepository.Control.HIDDEN && enabled)
                    com.kivo.cinema.sources.access.SourceAccessSession.lock()
            }, { error = safeError(it) })
        }
    }

    /** 名称留空时的兜底：Emby 用主机名，SMB 用共享名（没有共享就用主机名）。 */
    private fun defaultName(normalized: String): String = runCatching {
        if (protocol == SourceRecord.PROTOCOL_SMB) {
            val smb = com.kivo.cinema.smb.SmbAddress.parse(normalized)
            smb.share.ifEmpty { smb.host }
        } else {
            URI(normalized).host
        }
    }.getOrNull()?.takeIf { it.isNotBlank() } ?: "未命名"

    private fun clearDraft() {
        name = ""; address = ""; username = ""; password = ""; note = ""; original = null; error = null
        protocol = SourceRecord.PROTOCOL_EMBY
    }
    private fun safeError(e: Throwable): String = when (e) {
        is SourceFailure -> e.reason
        is EmbyClient.HttpFailure -> when (e.code) {
            401 -> "账号或密码未被接受，请检查后重试"
            403 -> "服务器拒绝访问，请检查账号权限或访问限制"
            404 -> "未找到Emby服务，请检查地址和服务路径"
            else -> "服务器暂时无法完成请求（${e.code}）"
        }
        is com.kivo.cinema.smb.SmbFailure -> e.reason
        is java.net.UnknownHostException -> "找不到服务器，请检查地址和网络"
        is java.net.SocketTimeoutException -> "连接超时，请稍后重试"
        is javax.net.ssl.SSLException -> "安全连接失败，请检查服务器证书"
        else -> "操作未完成，请检查网络或稍后重试"
    }
}

/**
 * SMB 的"验证"就是真连一次（243 D5）：
 * - 地址带共享 → 连到该共享（验证账号与权限）；
 * - 地址只写主机 → 连 IPC$ 枚举共享，至少要能看到一个，否则明确告诉用户写共享名。
 *
 * 放在文件级而不是 model 里，是因为它只依赖协议层，不需要 Android 上下文。
 */
private fun verifySmb(record: SourceRecord) {
    val address = com.kivo.cinema.smb.SmbAddress.parse(record.address)
    val credential = com.kivo.cinema.smb.SmbCredential(
        username = record.username,
        password = record.password,
    )
    if (address.needsShare) {
        val shares = com.kivo.cinema.smb.SmbShareLister(address.host, address.port, credential).list()
        if (shares.isEmpty()) {
            throw com.kivo.cinema.smb.SmbFailure(
                com.kivo.cinema.smb.SmbStage.SHARE,
                "服务器没有列出共享，请在地址里写明共享名（例如 smb://主机/视频）",
            )
        }
    } else {
        com.kivo.cinema.smb.SmbSession(address, credential).use { it.connect() }
    }
}
