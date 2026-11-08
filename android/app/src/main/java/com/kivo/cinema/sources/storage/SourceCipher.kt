package com.kivo.cinema.sources.storage

import com.kivo.cinema.sources.SourceFailure
import javax.crypto.Cipher
import javax.crypto.spec.GCMParameterSpec

/** Versioned authenticated envelope. Header authentication prevents format confusion. */
class SourceCipher(private val keys: SourceKeyProvider = SourceKeyProvider()) {
    fun encrypt(plain: ByteArray): ByteArray = safely {
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.ENCRYPT_MODE, keys.forEncryption())
        cipher.updateAAD(header)
        check(cipher.iv.size == 12)
        header + cipher.iv + cipher.doFinal(plain)
    }

    fun decrypt(bytes: ByteArray): ByteArray = safely {
        if (bytes.size < header.size + 12 + 16 ||
            !bytes.copyOfRange(0, header.size).contentEquals(header)) {
            throw SourceFailure("源记录格式不受支持或已损坏；原记录已保留")
        }
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.DECRYPT_MODE, keys.forDecryption(),
            GCMParameterSpec(128, bytes.copyOfRange(header.size, header.size + 12)))
        cipher.updateAAD(header)
        cipher.doFinal(bytes, header.size + 12, bytes.size - header.size - 12)
    }

    private fun <T> safely(block: () -> T): T = try { block() }
        catch (e: SourceFailure) { throw e }
        catch (_: Exception) { throw SourceFailure("无法加密或读取保存的账号；原记录已保留，请重新检查连接") }

    private companion object { val header = byteArrayOf(75, 73, 86, 79, 83, 1) }
}
