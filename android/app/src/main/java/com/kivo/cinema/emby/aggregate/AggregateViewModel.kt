package com.kivo.cinema.emby.aggregate

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope

/** 256 跨源宿主 ViewModel：让连接会话与搜索态跨配置变更存活；不持 UI。 */
class AggregateViewModel : ViewModel() {
    val session = AggregateSession(viewModelScope)
    val search = AggregateSearchModel(viewModelScope)

    fun start(context: Context) = session.connect(context)

    override fun onCleared() { session.close(); search.clear() }
}