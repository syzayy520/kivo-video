package com.kivo.cinema.sources.ui.workspace

import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.listSaver
import com.kivo.cinema.sources.SourceManagerModel

/** Navigation stores IDs only; pending navigation never captures or persists credentials. */
@Stable
internal class SourceWorkspaceState(id: String? = null, types: Boolean = false) {
    var sourceId by mutableStateOf(id)
    var choosingType by mutableStateOf(types)
    var pending by mutableStateOf<(() -> Unit)?>(null)
        private set

    fun navigate(model: SourceManagerModel, action: () -> Unit) {
        if (model.saving) return
        if (model.editing && (model.draftDirty || model.verifying)) pending = action
        else { if (model.editing) model.dismiss(); action() }
    }
    fun keepEditing() { pending = null }
    fun discard(model: SourceManagerModel) {
        if (model.saving) return
        val action = pending
        pending = null
        model.dismiss()
        action?.invoke()
    }
    fun clear() { sourceId = null; choosingType = false; pending = null }

    companion object {
        val Saver = listSaver<SourceWorkspaceState, String>(
            save = { listOf(it.sourceId.orEmpty(), it.choosingType.toString()) },
            restore = { SourceWorkspaceState(it[0].ifEmpty { null }, it[1].toBoolean()) })
    }
}
