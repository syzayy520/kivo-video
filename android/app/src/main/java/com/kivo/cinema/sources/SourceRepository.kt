package com.kivo.cinema.sources

import com.kivo.cinema.sources.storage.SourceStore

/** Each mutation rereads disk under the store lock, preserving other writers' changes. */
class SourceRepository(private val store: SourceStore) {
    fun read(): Result<List<SourceRecord>> = store.read()

    /** L2 must authenticate this draft before calling; expectedRevision=null means new source. */
    fun saveVerified(draft: SourceRecord, expectedRevision: Long? = null): Result<List<SourceRecord>> = store.update { old ->
        val previous = old.firstOrNull { it.id == draft.id }
        if (expectedRevision == null) {
            if (previous != null || draft.revision != 1L) throw SourceFailure("该源已存在，请刷新后重试")
        } else if (previous == null || previous.revision != expectedRevision || draft.revision != expectedRevision + 1) {
            throw SourceFailure("该源已被修改，请重新打开编辑")
        }
        if (old.any { it.id != draft.id && it.sameAccount(draft) }) throw SourceFailure("此服务器的该账号已添加")
        if (previous == null) old + draft else old.map { if (it.id == draft.id) draft else it }
    }

    fun remove(id: String, expectedRevision: Long): Result<List<SourceRecord>> = store.update { old ->
        val previous = old.firstOrNull { it.id == id } ?: throw SourceFailure("该源已不存在，请刷新列表")
        if (previous.revision != expectedRevision) throw SourceFailure("该源已被修改，请刷新后再删除")
        old.filterNot { it.id == id }
    }
}
