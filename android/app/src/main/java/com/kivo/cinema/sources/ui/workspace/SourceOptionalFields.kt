package com.kivo.cinema.sources.ui.workspace

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.EaseInOut
import androidx.compose.animation.core.tween
import androidx.compose.animation.expandVertically
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.sources.SourceAddress
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.SourceRecord

/**
 * 来源信息 · 选填（254 D13）：协议/端口/备注渐进展开（243 M04-S03，0.3s easeInOut）。
 * 协议与端口**派生自唯一地址态** [SourceManagerModel.address]，编辑后写回同一串，
 * 不另立互相矛盾的地址状态（library-rebuild/plan.md:40）。SMB 协议固定 smb://，只显端口。
 */
@Composable
internal fun SourceOptionalFields(model: SourceManagerModel) {
    var expanded by remember { mutableStateOf(false) }
    val smb = model.protocol == SourceRecord.PROTOCOL_SMB
    Column(Modifier.fillMaxWidth()) {
        Row(Modifier.fillMaxWidth().clickable(enabled = !model.busy) { expanded = !expanded }
            .padding(vertical = 12.dp), verticalAlignment = Alignment.CenterVertically) {
            Text("来源信息 · 选填", fontSize = 14.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.weight(1f))
            Text(if (expanded) "⌃" else "⌄", fontSize = 14.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
        AnimatedVisibility(visible = expanded,
            enter = expandVertically(tween(300, easing = EaseInOut)) + fadeIn(tween(300, easing = EaseInOut)),
            exit = shrinkVertically(tween(300, easing = EaseInOut)) + fadeOut(tween(300, easing = EaseInOut))) {
            Surface(shape = MaterialTheme.shapes.medium) {
                Column(Modifier.padding(vertical = 4.dp)) {
                    if (!smb) {
                        val scheme = SourceAddress.schemeOf(model.address)
                        Row(Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 10.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            Text("协议", fontSize = 14.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.weight(1f))
                            listOf("https", "http").forEach { s ->
                                FilterChip(selected = scheme == s, enabled = !model.busy,
                                    onClick = { model.address = SourceAddress.withScheme(model.address, s) },
                                    label = { Text(s) })
                            }
                        }
                        HorizontalDivider(Modifier.padding(horizontal = 16.dp),
                            color = MaterialTheme.colorScheme.outlineVariant)
                    }
                    TextField(value = SourceAddress.portOf(model.address),
                        onValueChange = { model.address = SourceAddress.withPort(model.address, it.filter(Char::isDigit).take(5)) },
                        label = { Text("端口") }, placeholder = { Text(if (smb) "默认 445" else "https 默认 443") },
                        enabled = !model.busy, singleLine = true,
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                        colors = sourceInputColors(), modifier = Modifier.fillMaxWidth())
                    HorizontalDivider(Modifier.padding(horizontal = 16.dp),
                        color = MaterialTheme.colorScheme.outlineVariant)
                    TextField(value = model.note, onValueChange = { model.note = it },
                        label = { Text("备注") }, placeholder = { Text("选填，给自己看的说明") },
                        enabled = !model.busy, singleLine = true,
                        colors = sourceInputColors(), modifier = Modifier.fillMaxWidth())
                }
            }
        }
    }
}