package com.kivo.cinema.smb

/** 出错的阶段。决定同一 NT status 该翻译成哪句人话（243 D7）。 */
enum class SmbStage(val label: String) {
    ADDRESS("地址"),
    CONNECT("连接"),
    AUTH("认证"),
    PERMISSION("权限"),
    SHARE("共享"),
    BROWSE("目录"),
    READ("读取"),
    UNKNOWN("未知"),
}

/** 带阶段的 SMB 失败。`reason` 给用户看；`detail`（NT status 与 libsmb2 原文）只进日志与证据。 */
class SmbFailure(
    val stage: SmbStage,
    val reason: String,
    val detail: String = "",
    cause: Throwable? = null,
) : Exception(reason, cause)

/**
 * libsmb2 错误 → 用户文案（243 D7）。纯映射，无 IO。输入是 `smb2_get_nterror()`（NT status）
 * 与 `smb2_get_error()` 原文：同一 `ACCESS_DENIED` 在"账号错"与"共享没授权"下含义不同。
 */
object SmbError {

    private const val ACCESS_DENIED = 0xC0000022L
    private const val LOGON_FAILURE = 0xC000006DL
    private const val BAD_NETWORK_NAME = 0xC00000CCL
    private const val OBJECT_NAME_NOT_FOUND = 0xC0000034L
    private const val OBJECT_PATH_NOT_FOUND = 0xC000003AL
    private const val PASSWORD_EXPIRED = 0xC0000071L
    private const val ACCOUNT_DISABLED = 0xC0000072L

    fun translate(stage: SmbStage, ntStatus: Int, raw: String, anonymous: Boolean): SmbFailure {
        val status = ntStatus.toLong() and 0xFFFFFFFFL
        val detail = "nt=0x${status.toString(16)} raw=$raw"
        // 未公开分步接口，但原文可定位失败点：negotiate → session setup → tree connect（libsmb2.c:703/894）。
        val where = if (stage != SmbStage.CONNECT) stage else when {
            raw.contains("Tree Connect failed") -> SmbStage.SHARE
            raw.contains("Session setup failed") -> SmbStage.AUTH
            else -> SmbStage.CONNECT
        }
        val reason = when (status) {
            0L -> fallback(where, raw)

            LOGON_FAILURE -> "账号或密码不正确"
            PASSWORD_EXPIRED -> "密码已过期，请在服务器上修改后重试"
            ACCOUNT_DISABLED -> "这个账号已被停用"

            ACCESS_DENIED -> when {
                where == SmbStage.AUTH -> "服务器拒绝了登录：账号密码不正确，或不允许匿名访问"
                where == SmbStage.SHARE -> "这个共享没有对本机开放，需要账号密码或服务器授权"
                anonymous -> "需要账号密码才能访问"
                else -> "当前账号没有访问权限"
            }

            BAD_NETWORK_NAME -> "找不到这个共享，可能已被删除或改名"
            OBJECT_PATH_NOT_FOUND, OBJECT_NAME_NOT_FOUND -> "路径不存在，请返回上一级重试"

            else -> fallback(where, raw)
        }
        return SmbFailure(where, reason, detail)
    }

    /** 非 NT 错误（DNS/连接被拒/超时/解析）走这里。 */
    fun stageFailure(stage: SmbStage, reason: String, detail: String = ""): SmbFailure =
        SmbFailure(stage, reason, detail)

    private fun fallback(stage: SmbStage, raw: String): String = when (stage) {
        SmbStage.CONNECT -> "无法连接服务器，请检查地址与网络"
        SmbStage.AUTH -> "登录失败，请检查账号和密码"
        SmbStage.SHARE -> "无法打开这个共享"
        SmbStage.BROWSE -> "无法读取文件夹"
        SmbStage.READ -> "读取中断，请检查网络后重试"
        SmbStage.ADDRESS -> "请输入有效的 SMB 地址"
        SmbStage.PERMISSION -> "当前账号没有访问权限"
        SmbStage.UNKNOWN -> raw.ifBlank { "SMB 操作失败" }
    }
}
