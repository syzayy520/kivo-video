package com.kivo.cinema.ui.compose.player.seek

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

internal class PlayerScrubState {
    var active by mutableStateOf(false)
        private set
    var visible by mutableStateOf(false)
        private set
    var originMs by mutableLongStateOf(0L)
        private set
    var targetMs by mutableLongStateOf(0L)
        private set
    var revision by mutableLongStateOf(0L)
        private set

    fun begin(positionMs: Long) {
        originMs = positionMs
        targetMs = positionMs
        active = true
        visible = true
        revision++
    }

    fun update(positionMs: Long) {
        targetMs = positionMs
    }

    fun finish() {
        active = false
        revision++
    }

    fun hide() {
        visible = false
    }

    fun cancel() {
        active = false
        visible = false
        revision++
    }
}
