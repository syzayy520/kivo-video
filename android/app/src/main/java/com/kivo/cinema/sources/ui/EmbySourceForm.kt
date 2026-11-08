package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.ui.workspace.SourceConnectionContent

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun EmbySourceForm(model: SourceManagerModel, onDismiss: () -> Unit = model::dismiss) {
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp,
        sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true,
            confirmValueChange = {
                if (it == SheetValue.Hidden && !model.saving && (model.draftDirty || model.verifying)) {
                    onDismiss(); false
                } else !model.saving
            }),
        containerColor = MaterialTheme.colorScheme.surfaceContainerLow) {
        Box(Modifier.fillMaxWidth().fillMaxHeight(.92f).imePadding()) {
            SourceConnectionContent(model, onDismiss)
        }
    }
}
