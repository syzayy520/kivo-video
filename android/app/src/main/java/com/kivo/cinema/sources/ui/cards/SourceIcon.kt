package com.kivo.cinema.sources.ui.cards

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.kivo.cinema.R

internal enum class SourceIconKind { EMBY, PLEX, JELLYFIN, STORAGE, LOCAL, SMB, LOCK, WEBDAV }

/** Brand assets where available; neutral line icons for storage, never emoji. */
@Composable
internal fun SourceIcon(kind: SourceIconKind, modifier: Modifier = Modifier) {
    val asset = when (kind) {
        SourceIconKind.EMBY -> R.drawable.emby_logo
        SourceIconKind.PLEX -> R.drawable.source_plex
        SourceIconKind.JELLYFIN -> R.drawable.source_jellyfin
        else -> null
    }
    if (asset != null) {
        Image(painterResource(asset), null, modifier.size(30.dp))
        return
    }
    val ink = Color(0xFF66778D)
    Canvas(modifier.size(30.dp).background(Color(0xFFEEF1F6), RoundedCornerShape(8.dp)).padding(6.dp)) {
        val w = size.width; val h = size.height
        val stroke = Stroke(width = 1.6.dp.toPx())
        when (kind) {
            SourceIconKind.LOCK -> {
                drawRoundRect(ink, Offset(w*.15f,h*.43f), Size(w*.7f,h*.52f), CornerRadius(w*.1f), style = stroke)
                drawArc(ink, 180f, 180f, false, Offset(w*.28f,h*.03f), Size(w*.44f,h*.66f), style = stroke)
                drawCircle(ink, w*.055f, Offset(w*.5f,h*.68f))
            }
            SourceIconKind.LOCAL -> {
                drawRoundRect(ink, Offset(w*.02f,h*.1f), Size(w*.96f,h*.8f), CornerRadius(w*.1f), style = stroke)
                val p = Path().apply { moveTo(w*.4f,h*.3f); lineTo(w*.72f,h*.5f); lineTo(w*.4f,h*.7f); close() }
                drawPath(p, ink)
            }
            SourceIconKind.WEBDAV -> {
                drawArc(ink, 180f, 180f, false, Offset(w*.2f,h*.08f), Size(w*.6f,h*.75f), style = stroke)
                drawRoundRect(ink, Offset(w*.03f,h*.4f), Size(w*.94f,h*.5f), CornerRadius(w*.15f), style = stroke)
            }
            else -> {
                drawRoundRect(ink, Offset(w*.05f,h*.08f), Size(w*.9f,h*.62f), CornerRadius(w*.1f), style = stroke)
                drawLine(ink, Offset(w*.5f,h*.7f), Offset(w*.5f,h*.95f), stroke.width)
                drawLine(ink, Offset(w*.12f,h*.95f), Offset(w*.88f,h*.95f), stroke.width)
                drawCircle(ink, w*.045f, Offset(w*.75f,h*.4f))
            }
        }
    }
}
