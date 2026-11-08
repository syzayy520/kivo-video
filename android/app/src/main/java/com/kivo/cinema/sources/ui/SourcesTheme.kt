package com.kivo.cinema.sources.ui

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Shapes
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.DesignTokens

/** Source controls inherit the app's neutral chrome; the host owns the ambient background. */
@Composable
internal fun SourcesTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = lightColorScheme(
            primary = DesignTokens.textMain, onPrimary = Color.White,
            primaryContainer = Color.White.copy(alpha = .76f), onPrimaryContainer = DesignTokens.textMain,
            secondaryContainer = Color.White.copy(alpha = .76f), onSecondaryContainer = DesignTokens.textMain,
            surface = DesignTokens.canvasFill, onSurface = DesignTokens.textMain,
            surfaceContainerLow = Color(0xFFFAF8FC), surfaceContainerHigh = Color(0xFFF6F3F8),
            surfaceVariant = DesignTokens.groupedFill, onSurfaceVariant = DesignTokens.textSub,
            outline = Color(0xFFD5D0DB), outlineVariant = DesignTokens.hairline,
            error = Color(0xFFBA303B),
        ),
        shapes = Shapes(
            small = RoundedCornerShape(10.dp),
            medium = RoundedCornerShape(DesignTokens.radiusCard.dp),
            large = RoundedCornerShape(20.dp), extraLarge = RoundedCornerShape(28.dp),
        ),
        content = content,
    )
}
