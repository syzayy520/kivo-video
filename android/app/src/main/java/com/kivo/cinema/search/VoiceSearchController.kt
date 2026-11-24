package com.kivo.cinema.search

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.speech.RecognitionListener
import android.speech.RecognizerIntent
import android.speech.SpeechRecognizer
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import java.util.Locale

/**
 * App 内语音识别（family-tree §四·补 语音卡）：封装 `SpeechRecognizer`，
 * 不再调起系统 AsrActivity（旧式弹窗样式不可控）。识别状态、音量、结果、错误全部可观察。
 * 权限申请在 UI 层（RECORD_AUDIO）；本类不涉及 UI。
 */
class VoiceSearchController(private val app: Context) : RecognitionListener {

    enum class State { Idle, Listening, Processing, Done, Failed }

    var state by mutableStateOf(State.Idle); private set
    /** 实时音量（onRmsChanged，通常 0~12），驱动声波高度。 */
    var rms by mutableStateOf(0f); private set
    var result by mutableStateOf<String?>(null); private set
    var error by mutableStateOf<String?>(null); private set

    private var recognizer: SpeechRecognizer? = null

    fun available(): Boolean = SpeechRecognizer.isRecognitionAvailable(app)

    fun start() {
        release()
        result = null
        error = null
        rms = 0f
        val r = SpeechRecognizer.createSpeechRecognizer(app)
        recognizer = r
        r.setRecognitionListener(this)
        state = State.Listening
        r.startListening(Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)
            .putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
            .putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault())
            .putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true))
    }

    /** 「说完了」：停止收音，进入识别中（onResults/onError 收尾）。 */
    fun finish() {
        if (state != State.Listening) return
        state = State.Processing
        recognizer?.stopListening()
    }

    fun cancel() {
        recognizer?.cancel()
        release()
        state = State.Idle
    }

    fun release() {
        recognizer?.destroy()
        recognizer = null
    }

    override fun onRmsChanged(rmsdB: Float) { rms = rmsdB.coerceIn(0f, 14f) }

    override fun onResults(results: Bundle?) {
        result = results?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)?.firstOrNull()
        state = if (result.isNullOrBlank()) {
            error = "没有听清，请再说一次"
            State.Failed
        } else State.Done
        release()
    }

    override fun onError(code: Int) {
        error = when (code) {
            SpeechRecognizer.ERROR_AUDIO -> "录音失败，请检查麦克风"
            SpeechRecognizer.ERROR_CLIENT -> "识别服务出错，请重试"
            SpeechRecognizer.ERROR_INSUFFICIENT_PERMISSIONS -> "没有麦克风权限，无法语音搜索"
            SpeechRecognizer.ERROR_NETWORK, SpeechRecognizer.ERROR_NETWORK_TIMEOUT -> "网络异常，语音识别不可用"
            SpeechRecognizer.ERROR_NO_MATCH -> "没有听清，请再说一次"
            SpeechRecognizer.ERROR_RECOGNIZER_BUSY -> "识别服务忙，请稍后再试"
            SpeechRecognizer.ERROR_SERVER -> "识别服务异常，请重试"
            SpeechRecognizer.ERROR_SPEECH_TIMEOUT -> "没有听到声音，请靠近麦克风说"
            else -> "语音识别失败（$code）"
        }
        state = State.Failed
        release()
    }

    override fun onReadyForSpeech(params: Bundle?) { state = State.Listening }
    override fun onBeginningOfSpeech() {}
    override fun onEndOfSpeech() { if (state == State.Listening) state = State.Processing }
    override fun onBufferReceived(buffer: ByteArray?) {}
    override fun onPartialResults(partialResults: Bundle?) {}
    override fun onEvent(eventType: Int, params: Bundle?) {}
}