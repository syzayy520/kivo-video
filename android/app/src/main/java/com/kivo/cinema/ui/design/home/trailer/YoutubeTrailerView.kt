package com.kivo.cinema.ui.design.home.trailer

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Color
import android.view.ViewGroup
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebResourceError
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.core.content.ContextCompat
import androidx.webkit.ProxyConfig
import androidx.webkit.ProxyController
import androidx.webkit.WebViewFeature
import com.kivo.cinema.tmdb.trailer.TmdbTrailer

/** The official hosted player owns decoding and the platform video URL. */
@SuppressLint("SetJavaScriptEnabled")
internal class YoutubeTrailerView(
    context: Context,
    private val video: TmdbTrailer,
    private val proxyAddress: String?,
    private val onPlaying: () -> Unit,
    private val onEnded: () -> Unit,
    private val onFailure: (String) -> Unit,
    private val onLayoutReport: (String) -> Unit,
) : WebView(context) {
    private var released = false
    private var ownsProxy = false

    init {
        // WRAP_CONTENT makes Chromium resolve percentage/vh heights against a
        // zero document viewport, even when Compose measures this view to 585dp.
        layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
        setBackgroundColor(Color.TRANSPARENT)
        isVerticalScrollBarEnabled = false
        isHorizontalScrollBarEnabled = false
        settings.javaScriptEnabled = true
        settings.domStorageEnabled = true
        settings.mediaPlaybackRequiresUserGesture = false
        settings.allowFileAccess = false
        settings.allowContentAccess = false
        webChromeClient = WebChromeClient()
        webViewClient = object : WebViewClient() {
            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest) = true
            override fun onReceivedError(view: WebView, request: WebResourceRequest, error: WebResourceError) {
                if (request.isForMainFrame) onFailure("预告片网络连接失败")
            }
        }
        addJavascriptInterface(Events(), "PreviewEvents")
    }

    fun start() {
        if (proxyAddress == null) loadPlayer()
        else if (WebViewFeature.isFeatureSupported(WebViewFeature.PROXY_OVERRIDE)) {
            ownsProxy = true
            val config = ProxyConfig.Builder().addProxyRule("socks://$proxyAddress").build()
            ProxyController.getInstance().setProxyOverride(config, ContextCompat.getMainExecutor(context)) {
                if (!released) loadPlayer()
            }
        } else onFailure("当前系统暂不支持预告片网络连接")
    }

    private fun loadPlayer() {
        if (released) return
        // The stable HTTPS base supplies the Referer/origin required by the
        // IFrame API. All video identifiers are validated by the repository.
        loadDataWithBaseURL("https://com.kivo.cinema/", """
            <!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1,minimum-scale=1,maximum-scale=1,user-scalable=no">
            <style>html,body{margin:0;width:100%;height:100%;overflow:hidden;background:transparent}
            #viewport{position:fixed;inset:0;overflow:hidden;background:#000}
            /* Crop the hosted 16:9 canvas, including cinematic letterbox margins,
               as a decorative full-bleed background. Never stretch the video. */
            #player{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%) scale(1.35);
            width:max(100vw,177.778vh);height:max(100vh,56.25vw);pointer-events:none}</style></head>
            <body><div id="viewport"><div id="player"></div></div><script>
            var player;
            function onYouTubeIframeAPIReady(){player=new YT.Player('player',{
              videoId:'${video.youtubeId}',playerVars:{autoplay:1,controls:0,playsinline:1,
              mute:1,rel:0,disablekb:1,fs:0,origin:'https://com.kivo.cinema'},
              events:{onReady:function(e){e.target.mute();e.target.playVideo();},
              onStateChange:function(e){PreviewEvents.state(e.data);},
              onError:function(e){PreviewEvents.error(e.data);}}});}
            </script><script src="https://www.youtube.com/iframe_api"></script></body></html>
        """.trimIndent(), "text/html", "UTF-8", null)
    }

    fun setMuted(muted: Boolean) {
        if (!released) evaluateJavascript("if(window.player){player.${if (muted) "mute" else "unMute"}();}", null)
    }

    private fun reportLayout() {
        if (!com.kivo.cinema.BuildConfig.DEBUG) return
        evaluateJavascript("""
            (function(){var p=document.getElementById('player'),r=p.getBoundingClientRect();
            return JSON.stringify({viewport:[innerWidth,innerHeight],frame:[r.x,r.y,r.width,r.height],
              document:[document.documentElement.clientWidth,document.documentElement.clientHeight],
              css:[getComputedStyle(p).width,getComputedStyle(p).height]});})()
        """.trimIndent()) { onLayoutReport("native=${width}x${height}; params=${layoutParams.width},${layoutParams.height}; dom=$it") }
    }

    fun release() {
        if (released) return
        released = true
        stopLoading()
        removeJavascriptInterface("PreviewEvents")
        loadUrl("about:blank")
        onPause()
        removeAllViews()
        destroy()
        if (ownsProxy) ProxyController.getInstance().clearProxyOverride(ContextCompat.getMainExecutor(context)) {}
    }

    private inner class Events {
        @JavascriptInterface fun state(value: Int) {
            post {
                if (!released) when (value) { 1 -> { reportLayout(); onPlaying() }; 0 -> onEnded() }
            }
        }
        @JavascriptInterface fun error(code: Int) {
            post {
                if (!released) onFailure(when (code) {
                    101, 150 -> "该预告片暂不支持站内播放"
                    100 -> "该预告片已下架或设为私密"
                    153 -> "预告片平台暂时无法验证播放请求"
                    else -> "预告片播放失败（$code）"
                })
            }
        }
    }
}
