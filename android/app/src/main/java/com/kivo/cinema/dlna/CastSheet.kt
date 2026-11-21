package com.kivo.cinema.dlna

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.MenuSeparator
import com.kivo.cinema.ui.compose.player.PlayerGlyph
import com.kivo.cinema.ui.compose.player.PlayerPanelStyle
import com.kivo.cinema.ui.compose.player.PlayerTokens
import com.kivo.cinema.ui.compose.player.panelPressable
import com.kivo.cinema.ui.compose.player.playerPanelSurface

/**
 * 投屏设备选择卡（230 设计稿 D5；v2 按 P10 §17 重做为「面板同族浮动卡」）。
 *
 * 【为什么不再是贴底 sheet】SenPlayer 没有自绘投屏面板（点投屏直接走系统），
 * 它的面板语言是浮动卡片：窄宽、13.5dp 圆角、左对齐行、右侧 chevron、没有多余按钮。
 * 旧版全宽贴底在无设备时就是一大片空玻璃，且设备行拿一个「投」字当图标 —— 用户点名的丑。
 *
 * 【定位不在这里】卡片挂在哪由调用方给 `modifier` 决定（`PlayerComposeRoot` 传
 * `align(TopStart) + offset(x = 投屏图标下方, y = 82dp)`），本组件只管画自己。
 * @param onDismiss 关闭：footer 右侧的「关闭」。再点顶栏投屏图标也会关（`cast` 动作 toggle）。
 */
@Composable
fun CastSheet(
    visible: Boolean,
    scanning: Boolean,
    devices: List<DlnaDevice>,
    message: String,
    onSelect: (DlnaDevice) -> Unit,
    onRescan: () -> Unit,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
) {
    if (!visible) return
    Column(
        modifier = modifier
            .width(PlayerTokens.panelWidth.dp)
            .heightIn(max = 460.dp)
            // 吃掉卡片内点击：手势层在下面兜底，不消费就会"点卡片＝关面板"。
            .pointerInput(Unit) { detectTapGestures(onTap = {}) }
            .playerPanelSurface(),
    ) {
        // ---- 头：标题 + 状态（状态就是扫描结果，不再占一枚胶囊按钮）----
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier
                .fillMaxWidth()
                .padding(
                    start = PlayerPanelStyle.hPadStart,
                    end = PlayerPanelStyle.rowTrailingPad,
                    top = 13.dp,
                    bottom = 13.dp,
                ),
        ) {
            Text(
                "投屏",
                color = PlayerPanelStyle.titleColor,
                fontSize = PlayerPanelStyle.titleSize.sp,
                fontWeight = FontWeight.SemiBold,
            )
            Spacer(Modifier.weight(1f))
            Text(
                when {
                    // 扫描中的"进行态"只出现在 footer 的按钮上，标题右侧留空 ——
                    // 两处同时写「扫描中…」是重复（真机截图 C0-02 指出的唯一瑕疵）。
                    scanning -> ""
                    devices.isNotEmpty() -> "发现 ${devices.size} 台"
                    else -> "未发现"
                },
                color = PlayerPanelStyle.subtitleColor,
                fontSize = PlayerPanelStyle.subtitleSize.sp,
            )
        }
        MenuSeparator()

        if (devices.isEmpty()) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(
                        start = PlayerPanelStyle.hPadStart,
                        end = PlayerPanelStyle.rowTrailingPad,
                        top = 12.dp,
                        bottom = 12.dp,
                    ),
            ) {
                // 空态用同一枚 cast 图标但压灰：不造新符号，也让"没有设备"一眼分清。
                PlayerGlyph("cast", PlayerPanelStyle.closeColor, Modifier.size(20.dp))
                Spacer(Modifier.width(12.dp))
                Column {
                    Text(
                        if (scanning) "正在搜索…" else "未发现设备",
                        color = PlayerPanelStyle.rowTitleColor,
                        fontSize = PlayerPanelStyle.rowTitleSize.sp,
                    )
                    Text(
                        "确认电视与手机在同一网络",
                        color = PlayerPanelStyle.subtitleColor,
                        fontSize = PlayerPanelStyle.subtitleSize.sp,
                    )
                }
            }
        } else {
            Column(Modifier.heightIn(max = 300.dp).verticalScroll(rememberScrollState())) {
                devices.forEach { d ->
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier
                            .fillMaxWidth()
                            .panelPressable(enabled = true) { onSelect(d) }
                            .padding(
                                start = PlayerPanelStyle.hPadStart,
                                end = PlayerPanelStyle.rowTrailingPad,
                                top = 12.dp,
                                bottom = 12.dp,
                            ),
                    ) {
                        PlayerGlyph("cast", PlayerPanelStyle.rowTitleColor, Modifier.size(20.dp))
                        Spacer(Modifier.width(12.dp))
                        Column(Modifier.weight(1f)) {
                            Text(
                                d.friendlyName.ifBlank { "未命名设备" },
                                color = PlayerPanelStyle.rowTitleColor,
                                fontSize = PlayerPanelStyle.rowTitleSize.sp,
                                maxLines = 1,
                            )
                            Text(
                                hostOf(d),
                                color = PlayerPanelStyle.subtitleColor,
                                fontSize = PlayerPanelStyle.groupHeadSize.sp,
                                maxLines = 1,
                            )
                        }
                        PlayerGlyph("chevron-right", PlayerPanelStyle.chevronColor, Modifier.size(16.dp))
                    }
                }
            }
        }

        if (message.isNotBlank()) {
            Text(
                message,
                color = PlayerPanelStyle.subtitleColor,
                fontSize = PlayerPanelStyle.subtitleSize.sp,
                modifier = Modifier.padding(
                    start = PlayerPanelStyle.hPadStart,
                    end = PlayerPanelStyle.rowTrailingPad,
                    bottom = 10.dp,
                ),
            )
        }

        // ---- footer：重新扫描（左）+ 关闭（右，12sp 次要）----
        MenuSeparator()
        Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .weight(1f)
                    .panelPressable(enabled = !scanning) { onRescan() }
                    .padding(
                        start = PlayerPanelStyle.hPadStart,
                        top = 11.dp,
                        bottom = 11.dp,
                    ),
            ) {
                PlayerGlyph(
                    "reset",
                    if (scanning) PlayerPanelStyle.closeColor else PlayerPanelStyle.checkColor,
                    Modifier.size(16.dp),
                )
                Spacer(Modifier.width(10.dp))
                Text(
                    "重新扫描",
                    color = if (scanning) PlayerPanelStyle.closeColor else PlayerPanelStyle.checkColor,
                    fontSize = PlayerPanelStyle.resetSize.sp,
                )
            }
            Text(
                "关闭",
                color = PlayerPanelStyle.subtitleColor,
                fontSize = PlayerPanelStyle.subtitleSize.sp,
                modifier = Modifier
                    .panelPressable(enabled = true) { onDismiss() }
                    .padding(
                        start = 12.dp,
                        end = PlayerPanelStyle.rowTrailingPad,
                        top = 11.dp,
                        bottom = 11.dp,
                    ),
            )
        }
    }
}

private fun hostOf(d: DlnaDevice): String =
    Regex("://([^/:]+)").find(d.location)?.groupValues?.get(1) ?: d.location