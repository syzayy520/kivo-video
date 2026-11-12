package com.kivo.cinema.privacy

import android.content.Context
import android.util.AtomicFile
import android.util.Base64
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.security.SecureRandom
import javax.crypto.SecretKeyFactory
import javax.crypto.spec.PBEKeySpec

/**
 * 应用内独立数字密码（39 卡）：盐化 PBKDF2 派生 + 原子存储 + 常数时间校验。
 * 单一职责：密码的派生 / 存储 / 校验。无 UI、不持有解锁会话（会话在 SourceAccessSession）。
 *
 * 威胁模型：这是访问门，不是文件加密——防手机已解锁时的随手翻看，不防离线取证。
 * 哈希落 noBackupFilesDir（应用私有、不备份），与 SourceStore 同口径；明文密码永不落盘。
 */
class PrivacyPassword(context: Context) {

    private val file = File(context.noBackupFilesDir, "privacy/lock.bin")
    private var salt: ByteArray? = null
    private var hash: ByteArray? = null
    private var iterations = DEFAULT_ITERATIONS

    val isSet: Boolean get() = hash != null

    init { load() }

    private fun load() {
        runCatching {
            if (!file.exists()) return
            val json = JSONObject(AtomicFile(file).readFully().toString(Charsets.UTF_8))
            if (json.optInt("schema") != SCHEMA) return
            salt = Base64.decode(json.getString("salt"), Base64.NO_WRAP)
            hash = Base64.decode(json.getString("hash"), Base64.NO_WRAP)
            iterations = json.optInt("iterations", DEFAULT_ITERATIONS)
        }.onFailure { salt = null; hash = null }
    }

    /** 设置 / 重设密码。4–6 位纯数字；失败带非空 reason（G9）。 */
    fun set(pin: String): Result<Unit> {
        validate(pin)?.let { return Result.failure(IllegalArgumentException(it)) }
        return try {
            val newSalt = ByteArray(16).also { SecureRandom().nextBytes(it) }
            val newHash = derive(pin, newSalt, DEFAULT_ITERATIONS)
            persist(newSalt, newHash, DEFAULT_ITERATIONS)
            salt = newSalt; hash = newHash; iterations = DEFAULT_ITERATIONS
            Result.success(Unit)
        } catch (_: Exception) {
            Result.failure(IllegalStateException("无法保存数字密码，请重试"))
        }
    }

    fun verify(pin: String): Boolean {
        val s = salt ?: return false
        val h = hash ?: return false
        if (pin.isEmpty()) return false
        return MessageDigest.isEqual(derive(pin, s, iterations), h)
    }

    fun change(old: String, new: String): Result<Unit> {
        if (!verify(old)) return Result.failure(IllegalArgumentException("原密码不正确"))
        return set(new)
    }

    private fun persist(s: ByteArray, h: ByteArray, iter: Int) {
        file.parentFile?.mkdirs()
        val json = JSONObject().put("schema", SCHEMA)
            .put("salt", Base64.encodeToString(s, Base64.NO_WRAP))
            .put("hash", Base64.encodeToString(h, Base64.NO_WRAP))
            .put("iterations", iter)
        val atomic = AtomicFile(file)
        var stream: FileOutputStream? = null
        try {
            stream = atomic.startWrite()
            stream.write(json.toString().toByteArray(Charsets.UTF_8))
            atomic.finishWrite(stream)
            stream = null
        } finally { if (stream != null) atomic.failWrite(stream) }
    }

    private fun derive(pin: String, s: ByteArray, iter: Int): ByteArray {
        val spec = PBEKeySpec(pin.toCharArray(), s, iter, 256)
        return SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256").generateSecret(spec).encoded
    }

    private fun validate(pin: String): String? = when {
        pin.length !in 4..6 -> "密码须为 4–6 位数字"
        pin.any { !it.isDigit() } -> "密码只能包含数字"
        else -> null
    }

    private companion object {
        const val SCHEMA = 1
        const val DEFAULT_ITERATIONS = 120_000
    }
}