package com.kivo.cinema.ui.search.home

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.history.glassSurface

import com.kivo.cinema.ui.search.SearchGlyph
import com.kivo.cinema.ui.search.SearchIcon
import com.kivo.cinema.ui.search.SearchMagnifier
import com.kivo.cinema.ui.search.SearchTokens

private val FieldShape = RoundedCornerShape(23.dp)

/** ① 只读入口态：玻璃框 + 占位文案 + 语音钮；整框点击进输入态。 */
@Composable
internal fun SearchFieldEntry(onOpen: () -> Unit, onVoice: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().padding(horizontal = SearchTokens.pad)
            .height(SearchTokens.fieldHeight)
            .glassSurface(FieldShape)
            .clip(FieldShape)
            .clickable(onClick = onOpen)
            .padding(start = 16.dp, end = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        SearchMagnifier(DesignTokens.textSub, Modifier.size(19.dp))
        Spacer(Modifier.size(9.dp))
        Text("片名、演员、导演", fontSize = 16.sp, color = DesignTokens.textSub, modifier = Modifier.weight(1f))
        VoiceButton(onVoice)
    }
}

/** ② 编辑态：自动聚焦、蓝焦点环与光标、清除圆钮、语音钮。 */
@Composable
internal fun SearchFieldEdit(
    query: String,
    onQuery: (String) -> Unit,
    onVoice: () -> Unit,
    modifier: Modifier = Modifier,
    onSubmit: () -> Unit = {},
) {
    val focus = remember { FocusRequester() }
    val keyboard = androidx.compose.ui.platform.LocalSoftwareKeyboardController.current
    var focused by remember { androidx.compose.runtime.mutableStateOf(false) }
    LaunchedEffect(Unit) {
        // AnimatedContent 过渡期间的 requestFocus 会被吞（真机②键盘不出），延迟到过渡落定后再请求。
        kotlinx.coroutines.delay(180)
        focus.requestFocus()
    }
    LaunchedEffect(focused) {
        if (focused) keyboard?.show()
    }
    Row(
        modifier.height(SearchTokens.fieldHeight)
            .glassSurface(FieldShape)
            .border(1.5.dp, SearchTokens.focusBlue.copy(alpha = .42f), FieldShape)
            .clip(FieldShape)
            .padding(start = 16.dp, end = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        SearchMagnifier(DesignTokens.textSub, Modifier.size(19.dp))
        Spacer(Modifier.size(9.dp))
        BasicTextField(
            value = query,
            onValueChange = onQuery,
            singleLine = true,
            // 部分第三方输入法（微信输入法）对没有 imeAction 的字段键盘形态异常，补标准搜索键。
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Text, imeAction = ImeAction.Search),
            keyboardActions = KeyboardActions(onSearch = { onSubmit(); keyboard?.hide() }),
            textStyle = TextStyle(fontSize = 16.sp, color = DesignTokens.textMain),
            cursorBrush = SolidColor(SearchTokens.focusBlue),
            decorationBox = { inner ->
                if (query.isEmpty()) Text("片名、演员、导演", fontSize = 16.sp, color = DesignTokens.textSub)
                inner()
            },
            modifier = Modifier.weight(1f).focusRequester(focus)
                .onFocusChanged { focused = it.isFocused },
        )
        if (query.isNotEmpty()) Box(
            Modifier.size(18.dp).clip(CircleShape).background(SearchTokens.clearFill)
                .clickable { onQuery("") },
            contentAlignment = Alignment.Center,
        ) {
            SearchGlyph(SearchIcon.Close, DesignTokens.canvasFill, Modifier.size(10.dp))
        }
        Spacer(Modifier.size(9.dp))
        VoiceButton(onVoice)
    }
}

@Composable
private fun VoiceButton(onVoice: () -> Unit) {
    Box(
        Modifier.size(36.dp).clip(CircleShape).clickable(onClick = onVoice),
        contentAlignment = Alignment.Center,
    ) {
        SearchGlyph(SearchIcon.Mic, DesignTokens.textSub, Modifier.size(19.dp))
    }
}