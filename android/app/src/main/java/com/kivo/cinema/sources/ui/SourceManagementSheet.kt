package com.kivo.cinema.sources.ui

import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository.Control
import com.kivo.cinema.sources.ui.workspace.SourceManagementContent

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SourceManagementSheet(record: SourceRecord, onDismiss: () -> Unit, onEdit: () -> Unit,
    onDelete: () -> Unit, onControl: (Control, Boolean) -> Unit, busy: Boolean = false, error: String? = null) {
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp,
        sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true),
        containerColor = MaterialTheme.colorScheme.surfaceContainerLow) {
        SourceManagementContent(record, onDismiss, onEdit, onDelete, onControl, busy, error)
    }
}
