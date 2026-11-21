package com.kivo.cinema.subtitle

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.file.Files

// ============================================================================
// TranslationJournal 的 JVM 单测（评审 C5/F4/E7/G7 的契约锁定）。
//
//   - 行结果追加 + 回放（cueId → 最近记录，幂等）；
//   - 损坏/截断行 → 回放截断到最后有效行；
//   - 事件与行结果同 journal、seq 单调；
//   - manifest = 意图真源（独立 fsync），与 journal（进度真源）分工；
//   - 转义往返：text 含 \t \n \\ 不破坏单行协议。
// ============================================================================

class TranslationJournalTest {

    private fun tempDir(): java.io.File =
        Files.createTempDirectory("kivo-journal-test").toFile()

    @Test
    fun `line results roundtrip with escaping`() {
        val dir = tempDir()
        val journal = TranslationJournal(dir, "job1")
        // text 含制表符/换行/反斜杠 —— 全部会被转义保单行。
        journal.appendLineResult(0, TranslationLineStatus.LLM_TRANSLATED, "a\tb\nc\\d")
        journal.appendLineResult(1, TranslationLineStatus.RULE_TRANSLATED, "[音乐]")
        journal.flush()

        val replay = journal.replay()
        assertEquals(2, replay.lineResults.size)
        assertEquals("a\tb\nc\\d", replay.lineResults[0]!!.text)
        assertEquals(TranslationLineStatus.LLM_TRANSLATED, replay.lineResults[0]!!.status)
        assertEquals("[音乐]", replay.lineResults[1]!!.text)
        assertEquals(2, replay.lastSeq)
    }

    @Test
    fun `events and line results share monotonic seq`() {
        val dir = tempDir()
        val journal = TranslationJournal(dir, "job2")
        journal.appendEvent(TranslationJournalEvents.JOB_CREATED)
        journal.appendLineResult(0, TranslationLineStatus.LLM_TRANSLATED, "x")
        journal.appendEvent(TranslationJournalEvents.BLOCK_COMMITTED)
        journal.flush()

        val replay = journal.replay()
        assertEquals(listOf("JOB_CREATED", "BLOCK_COMMITTED"), replay.events)
        assertEquals(3, replay.lastSeq)
    }

    @Test
    fun `truncated tail is dropped on replay`() {
        val dir = tempDir()
        val journal = TranslationJournal(dir, "job3")
        journal.appendLineResult(0, TranslationLineStatus.LLM_TRANSLATED, "good")
        journal.flush()
        // 模拟进程被杀：追加半行（无换行结尾、JSON/协议不完整）。
        java.io.FileOutputStream(journalFile(dir, "job3"), true).use {
            it.write("L\t2\t1\tFAILED_FAL".toByteArray())
        }

        val replay = journal.replay()
        // 半行解析失败 → 截断，第一条完整记录保留。
        assertEquals(1, replay.lineResults.size)
        assertEquals("good", replay.lineResults[0]!!.text)
        assertEquals(1, replay.lastSeq)
    }

    @Test
    fun `duplicate cue id keeps latest record`() {
        val dir = tempDir()
        val journal = TranslationJournal(dir, "job4")
        journal.appendLineResult(0, TranslationLineStatus.FAILED_FALLBACK, "original")
        journal.appendLineResult(0, TranslationLineStatus.LLM_TRANSLATED, "retry ok")
        journal.flush()

        val replay = journal.replay()
        // 幂等：同 cue 重复处理以最近记录为准（评审 E7）。
        assertEquals("retry ok", replay.lineResults[0]!!.text)
        assertEquals(TranslationLineStatus.LLM_TRANSLATED, replay.lineResults[0]!!.status)
    }

    @Test
    fun `manifest is intent source of truth`() {
        val dir = tempDir()
        TranslationJournal.writeManifest(
            dir, "job5",
            TranslationManifest(
                taskState = "TRANSLATING",
                totalCues = 1000,
                cacheKey = "hash-abc",
                sourceHash = "sha-123",
                failedFallbacks = 2,
            ),
        )
        val manifest = TranslationJournal.readManifest(dir, "job5")
        assertNotNull(manifest)
        assertEquals("TRANSLATING", manifest!!.taskState)
        assertEquals(1000, manifest.totalCues)
        assertEquals("hash-abc", manifest.cacheKey)
        assertEquals("sha-123", manifest.sourceHash)
        assertEquals(2, manifest.failedFallbacks)
    }

    @Test
    fun `missing manifest returns null not invented state`() {
        val dir = tempDir()
        assertNull(TranslationJournal.readManifest(dir, "nope"))
    }

    @Test
    fun `prime seq continues after reopen`() {
        val dir = tempDir()
        val journal1 = TranslationJournal(dir, "job6")
        journal1.appendLineResult(0, TranslationLineStatus.LLM_TRANSLATED, "first")
        journal1.flush()

        // 重新打开（续译场景）：seq 从既有 journal 最大值继续。
        val journal2 = TranslationJournal(dir, "job6")
        journal2.primeSeqFromDisk()
        journal2.appendLineResult(1, TranslationLineStatus.LLM_TRANSLATED, "second")
        journal2.flush()

        val replay = journal2.replay()
        assertEquals(2, replay.lineResults.size)
        assertEquals(2, replay.lastSeq)
    }

    private fun journalFile(dir: java.io.File, jobId: String) =
        java.io.File(dir, "$jobId.journal")
}