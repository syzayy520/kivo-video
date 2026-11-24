package com.kivo.cinema.ui.search.query

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.clickable
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.search.SearchRepository
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.search.SearchTokens
import com.kivo.cinema.ui.search.home.SearchFieldEdit

/**
 * ② 输入中编排：聚焦搜索框 + 取消 → 最佳匹配 → 结果列表。
 * 边输边出结果（仓储 350ms 防抖）；加载/空/远端失败/本地未授权都有可见文案。
 */
@Composable
internal fun SearchQueryPage(
    repo: SearchRepository,
    onCancel: () -> Unit,
    onVoice: () -> Unit,
    onOpen: (SearchEntry) -> Unit,
    onSubmitQuery: () -> Unit,
) {
    Column(Modifier.fillMaxSize().imePadding()) {
        Row(
            Modifier.fillMaxWidth().padding(start = SearchTokens.pad, top = 6.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            SearchFieldEdit(
                query = repo.query, onQuery = repo::onQuery, onVoice = onVoice,
                modifier = Modifier.weight(1f), onSubmit = onSubmitQuery,
            )
            Text(
                "取消", fontSize = 16.sp, fontWeight = FontWeight.Medium, color = DesignTokens.textMain,
                modifier = Modifier.clickable(onClick = onCancel)
                    .padding(start = 12.dp, end = SearchTokens.pad, top = 14.dp, bottom = 14.dp),
            )
        }
        val best = repo.bestMatch()
        val rest = repo.restResults()
        LazyColumn(
            Modifier.fillMaxWidth().weight(1f),
            contentPadding = PaddingValues(bottom = 24.dp),
        ) {
            if (best != null) item { BestMatchCard(best, onOpen) }
            if (rest.isNotEmpty()) {
                item {
                    Row(
                        Modifier.fillMaxWidth()
                            .padding(start = SearchTokens.pad, end = SearchTokens.pad, top = 16.dp),
                        horizontalArrangement = androidx.compose.foundation.layout.Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Text("影片", fontSize = 17.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
                        Text(
                            "共 ${rest.size} 条", fontSize = 13.sp, color = DesignTokens.textSub,
                        )
                    }
                }
                item { SearchResultList(rest, repo.query, onOpen) }
            }
            item {
                QueryStatus(
                    loading = repo.loading,
                    query = repo.query,
                    empty = best == null,
                    error = repo.error,
                    localNotice = repo.localNotice,
                )
            }
        }
    }
}

@Composable
private fun QueryStatus(
    loading: Boolean,
    query: String,
    empty: Boolean,
    error: String?,
    localNotice: String?,
) {
    val text = when {
        loading && empty -> "正在搜索…"
        error != null -> error
        empty && query.isNotBlank() -> "没有找到与「${query.trim()}」相关的内容"
        else -> null
    }
    // 部分源失败/未授权提示与结果并存：有结果时也不许吞掉（不静默降级）。
    if (text == null && localNotice == null) return
    Column(Modifier.fillMaxWidth().padding(top = 36.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        if (text != null) Text(
            text, fontSize = 13.sp, color = DesignTokens.textSub,
            textAlign = TextAlign.Center, modifier = Modifier.padding(horizontal = 32.dp),
        )
        if (localNotice != null && !(loading && empty)) Text(
            localNotice, fontSize = 11.sp, color = DesignTokens.textSub,
            textAlign = TextAlign.Center, modifier = Modifier.padding(start = 32.dp, end = 32.dp, top = 8.dp),
        )
    }
}
/** 门禁 Preview：聚焦态搜索框（静态，不触网）。 */
@Preview
@Composable
private fun SearchQueryFieldPreview() {
    SearchFieldEdit(query = "黑帮", onQuery = {}, onVoice = {})
}