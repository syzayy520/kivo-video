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
        if (previous == null) old + draft.withControls(hidden = false, defaultHome = false,
            aggregateEnabled = true, aggregateSearchEnabled = true)
        else old.map { if (it.id == draft.id) draft.withControls(hidden = previous.hidden,
            defaultHome = previous.defaultHome, aggregateEnabled = previous.aggregateEnabled,
            aggregateSearchEnabled = previous.aggregateSearchEnabled) else it }
    }

    enum class Control { HIDDEN, DEFAULT_HOME, AGGREGATE, AGGREGATE_SEARCH }

    fun setControl(id: String, expectedRevision: Long, control: Control, enabled: Boolean): Result<List<SourceRecord>> = store.update { old ->
        val record = old.firstOrNull { it.id == id } ?: throw SourceFailure("该源已不存在，请刷新列表")
        if (record.revision != expectedRevision) throw SourceFailure("该源已被修改，请刷新后重试")
        old.map { source ->
            when {
                source.id == id -> source.withControls(revision = source.revision + 1,
                    hidden = if (control == Control.HIDDEN) enabled else source.hidden,
                    defaultHome = if (control == Control.DEFAULT_HOME) enabled else source.defaultHome,
                    aggregateEnabled = if (control == Control.AGGREGATE) enabled else source.aggregateEnabled,
                    aggregateSearchEnabled = if (control == Control.AGGREGATE_SEARCH) enabled else source.aggregateSearchEnabled)
                control == Control.DEFAULT_HOME && enabled && source.defaultHome ->
                    source.withControls(defaultHome = false, revision = source.revision + 1)
                else -> source
            }
        }
    }

    fun remove(id: String, expectedRevision: Long): Result<List<SourceRecord>> = store.update { old ->
        val previous = old.firstOrNull { it.id == id } ?: throw SourceFailure("该源已不存在，请刷新列表")
        if (previous.revision != expectedRevision) throw SourceFailure("该源已被修改，请刷新后再删除")
        old.filterNot { it.id == id }
    }
}
