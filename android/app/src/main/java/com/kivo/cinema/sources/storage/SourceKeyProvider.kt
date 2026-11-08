package com.kivo.cinema.sources.storage

import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import com.kivo.cinema.sources.SourceFailure
import java.security.KeyStore
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey

/** Reading a missing key must not silently generate a replacement for existing ciphertext. */
class SourceKeyProvider(private val alias: String = "kivo.sources.v1") {
    fun existing(): SecretKey? = KeyStore.getInstance("AndroidKeyStore").run {
        load(null)
        getKey(alias, null) as? SecretKey
    }

    fun forEncryption(): SecretKey = synchronized(keyLock) {
        existing() ?: KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore").run {
            init(KeyGenParameterSpec.Builder(alias,
                KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT)
                .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                .setRandomizedEncryptionRequired(true).build())
            generateKey()
        }
    }

    fun forDecryption(): SecretKey = existing()
        ?: throw SourceFailure("保存的账号密钥不可用，请重新添加连接；原记录已保留")

    private companion object { val keyLock = Any() }
}
