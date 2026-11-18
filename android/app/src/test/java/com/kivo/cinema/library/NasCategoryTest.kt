package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * NAS 目录 → 分类映射测试。
 *
 * 分类错了会让片进错行（用户明确要求"必须分电影/剧集/综艺/动漫/演唱会"），
 * 或整类被丢进"认不出"而消失，所以映射表要锁住。
 */
class NasCategoryTest {

    @Test
    fun `电影类目录`() {
        assertEquals(NasCategory.Movie, NasCategory.fromDirName("电影"))
        assertEquals(NasCategory.Movie, NasCategory.fromDirName("REMUX"))
        assertEquals(NasCategory.Movie, NasCategory.fromDirName("原盘"))
    }

    @Test
    fun `剧集类目录`() {
        assertEquals(NasCategory.Tv, NasCategory.fromDirName("剧集"))
        assertEquals(NasCategory.Tv, NasCategory.fromDirName("电视剧"))
        assertEquals(NasCategory.Tv, NasCategory.fromDirName("美剧"))
    }

    @Test
    fun `动漫与综艺`() {
        assertEquals(NasCategory.Anime, NasCategory.fromDirName("动漫"))
        assertEquals(NasCategory.Anime, NasCategory.fromDirName("动画片"))
        assertEquals(NasCategory.Anime, NasCategory.fromDirName("番剧"))
        assertEquals(NasCategory.Variety, NasCategory.fromDirName("综艺"))
    }

    @Test
    fun `演唱会与纪录片`() {
        assertEquals(NasCategory.Concert, NasCategory.fromDirName("演唱会"))
        assertEquals(NasCategory.Concert, NasCategory.fromDirName("音乐"))
        assertEquals(NasCategory.Documentary, NasCategory.fromDirName("纪录片"))
    }

    @Test
    fun `认不出的目录返回 null 而不是塞进其他`() {
        assertNull(NasCategory.fromDirName("BT源"))
        assertNull(NasCategory.fromDirName("随便什么"))
    }

    @Test
    fun `展示顺序与默认可见分类固定`() {
        assertEquals(
            listOf(
                NasCategory.Movie, NasCategory.Tv, NasCategory.Variety,
                NasCategory.Anime, NasCategory.Concert, NasCategory.Documentary,
            ),
            NasCategory.ordered,
        )
        assertEquals(setOf(NasCategory.Movie, NasCategory.Tv), NasCategory.defaultVisible)
    }
}
