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
class SourceManagerModel(app: Application) : AndroidViewModel(app) {
    private val repository = SourceRepository(SourceStore(app))
    var records by mutableStateOf<List<SourceRecord>>(emptyList()); private set
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
    private var original: SourceRecord? = null
    private var job: Job? = null
    private var generation = 0
    val busy get() = verifying || saving

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
        original = record
        name = record?.name.orEmpty(); address = record?.address.orEmpty()
        username = record?.username.orEmpty(); password = record?.password.orEmpty()
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
        val normalized = runCatching { SourceAddress.normalize(address) }.getOrElse {
            error = "请检查完整服务器地址"; return
        }
        if (username.isBlank()) { error = "请输入账号"; return }
        val previous = original
        val draft = SourceRecord(id = previous?.id ?: java.util.UUID.randomUUID().toString(),
            revision = (previous?.revision ?: 0) + 1,
            name = name.trim().ifEmpty { URI(normalized).host }, address = normalized,
            username = username.trim(), password = password)
        if (records.any { it.id != draft.id && it.sameAccount(draft) }) {
            error = "此服务器的该账号已添加"; return
        }
        error = null; verifying = true
        val ticket = ++generation
        job = viewModelScope.launch {
            val authentication = withContext(Dispatchers.IO) {
                runCatching { EmbyClient(draft.address).login(draft.username, draft.password) }
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
        ++generation
        saving = true; error = null
        viewModelScope.launch {
            val result = withContext(Dispatchers.IO) { repository.remove(record.id, record.revision) }
            saving = false
            result.fold({ records = it }, { error = safeError(it) })
        }
    }

    private fun clearDraft() { name = ""; address = ""; username = ""; password = ""; original = null; error = null }
    private fun safeError(e: Throwable): String = when (e) {
        is SourceFailure -> e.reason
        is EmbyClient.HttpFailure -> when (e.code) {
            401 -> "账号或密码未被接受，请检查后重试"
            403 -> "服务器拒绝访问，请检查账号权限或访问限制"
            404 -> "未找到Emby服务，请检查地址和服务路径"
            else -> "服务器暂时无法完成请求（${e.code}）"
        }
        is java.net.UnknownHostException -> "找不到服务器，请检查地址和网络"
        is java.net.SocketTimeoutException -> "连接超时，请稍后重试"
        is javax.net.ssl.SSLException -> "安全连接失败，请检查服务器证书"
        else -> "操作未完成，请检查网络或稍后重试"
    }
}
