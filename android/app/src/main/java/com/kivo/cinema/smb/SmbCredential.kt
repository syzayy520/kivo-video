package com.kivo.cinema.smb

/**
 * SMB 凭据（243 D2）。不可变值对象。
 *
 * 空用户名 = 匿名（libsmb2 的匿名分支要求 password 为 NULL，不是空串），
 * 所以"是否匿名"必须由这里显式回答，不能靠调用方各自判断空串。
 *
 * 安全：不参与 toString / data class / 日志。密码只交给 JNI。
 */
class SmbCredential(
    val domain: String = "",
    val username: String = "",
    val password: String = "",
) {
    val anonymous: Boolean get() = username.isEmpty()

    override fun toString(): String =
        "SmbCredential(host-user=${if (anonymous) "<anonymous>" else username})"

    companion object {
        val ANONYMOUS = SmbCredential()
    }
}
