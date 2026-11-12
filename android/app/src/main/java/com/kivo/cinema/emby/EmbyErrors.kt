package com.kivo.cinema.emby

import java.net.ConnectException
import java.net.SocketTimeoutException
import java.net.UnknownHostException
import javax.net.ssl.SSLException

/** Emby 出网失败 → 用户可见中文 reason（单源会话与聚合会话共用，避免两套相同映射）。 */
object EmbyErrors {
    fun reason(e: Throwable): String = when (e) {
        is EmbyClient.HttpFailure -> when (e.code) {
            401 -> "账号或密码未被接受，请检查后重试。"
            403 -> "服务器拒绝了访问，请检查账号权限或服务器的访问限制。"
            404 -> "没有找到 Emby 服务，请检查地址、端口和路径。"
            else -> "服务器暂时无法完成请求（${e.code}），请稍后重试。"
        }
        is UnknownHostException -> "找不到服务器，请检查地址和网络连接。"
        is SocketTimeoutException -> "连接超时，请检查网络或稍后重试。"
        is ConnectException -> "无法连接服务器，请检查端口与服务是否在线。"
        is SSLException -> "无法建立安全连接，请检查服务器证书。"
        is IllegalArgumentException, is java.net.URISyntaxException -> "服务器地址格式不正确，请检查后重试。"
        else -> "连接未完成，请检查网络与服务器地址后重试。"
    }
}