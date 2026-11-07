package com.kivo.cinema.dlna

/** DLNA 渲染器数据类（230 设计稿家族树）。 */
data class DlnaDevice(
    val usn: String,        // 唯一标识（去重键）
    val location: String,   // 设备描述 XML 的 http URL（SSDP LOCATION）
    val friendlyName: String = "",
    val controlUrl: String = "",   // AVTransport control URL（解析描述后填充）
)
