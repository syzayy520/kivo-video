package com.kivo.cinema.sources.ui.cards

import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.unit.sp

@Composable
internal fun StorageEntryCard(title: String, subtitle: String, kind: SourceIconKind, onOpen: () -> Unit) {
    SourceTile(title, subtitle, onOpen, icon = { SourceIcon(kind) },
        accessory = { Text("›", fontSize = 22.sp) })
}
