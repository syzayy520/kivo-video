package com.kivo.cinema.danmaku

import org.xmlpull.v1.XmlPullParser
import org.xmlpull.v1.XmlPullParserFactory
import java.io.Reader

/**
 * B 站 XML 弹幕解析器（223 设计稿 D1）。
 *
 * 格式：`<d p="time,mode,size,color,date,pool,uid,rowhash">text</d>`
 * time=秒、mode=1滚动/4底部/5顶部、size=字号、color=RRGGBB 十进制。
 *
 * 单一职责：XML → List<DanmakuItem>——不涉及渲染/网络。
 */
object DanmakuParser {

    fun parse(reader: Reader): List<DanmakuItem> {
        val factory = XmlPullParserFactory.newInstance()
        val parser = factory.newPullParser()
        parser.setInput(reader)

        val items = mutableListOf<DanmakuItem>()
        var currentP = ""

        while (parser.eventType != XmlPullParser.END_DOCUMENT) {
            when (parser.eventType) {
                XmlPullParser.START_TAG -> {
                    if (parser.name == "d") {
                        currentP = parser.getAttributeValue(null, "p") ?: ""
                    }
                }
                XmlPullParser.TEXT -> {
                    if (currentP.isNotEmpty()) {
                        val text = parser.text?.trim() ?: ""
                        if (text.isNotEmpty()) {
                            items.add(parseDanmaku(currentP, text))
                        }
                    }
                }
                XmlPullParser.END_TAG -> {
                    if (parser.name == "d") {
                        currentP = ""
                    }
                }
            }
            parser.next()
        }
        return items.sortedBy { it.time }
    }

    /** 解析 p 属性："time,mode,size,color,date,pool,uid,rowhash" */
    fun parseDanmaku(pAttr: String, text: String): DanmakuItem {
        val parts = pAttr.split(",")
        return DanmakuItem(
            text = text,
            time = parts.getOrNull(0)?.toFloatOrNull() ?: 0f,
            mode = parts.getOrNull(1)?.toIntOrNull() ?: 1,
            size = parts.getOrNull(2)?.toIntOrNull() ?: 25,
            color = parts.getOrNull(3)?.toLongOrNull()?.toInt() ?: 0xFFFFFF,
        )
    }
}
