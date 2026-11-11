// 253 家族：会话实现 —— **只做一件事**：mpv 句柄的创建 / 初始化 / 绑定 Surface / 销毁。
//
// 拆分理由（家族树 §二）：命令下发、事件循环、渲染各有各的线程与语义，
// 都在各自的 .cpp 里（mpv_commands / mpv_event_loop / mpv_video_output）。
// 本文件**不含**任何命令、事件或渲染逻辑。

#include "mpv_session.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <android/native_window.h>

#include "mpv_session_impl.hpp"
#include "mpv_video_output.hpp"

namespace kivo::video::mpv {
namespace {

/// 步骤标记：native 崩溃时 mpv 自己的日志会**跟着一起断**（日志由 mpv 写），
/// 所以定位崩溃点不能靠它。这里用最原始的办法：每做完一步，往文件里追加一行。
///
/// 用 open/write 原始系统调用（不用 ofstream）—— 崩溃时 C++ 流的状态不确定。
void mark_step(const char* log_file, const char* step) {
    if (log_file == nullptr || log_file[0] == 0) {
        return;
    }
    const int fd = ::open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        return;
    }
    const std::string line = std::string("STEP ") + step + "\n";
    (void)write(fd, line.data(), line.size());
    ::close(fd);
}

}  // namespace

bool Session::attach_surface(ANativeWindow* window, std::string& error) {
    if (!impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    if (window == nullptr) {
        error = "surface is null";
        return false;
    }
    // 渲染层归本会话所有（不是全局静态）—— 理由见 mpv_session_impl.hpp 里
    // "为什么不能是全局"的说明：mpv_render_context_free() 必须早于
    // mpv_terminate_destroy()，否则 mpv 会 abort（真机实测：
    // "Broken API use: mpv_render_context_free() not called."）。
    impl_->video_output = create_video_output(impl_->handle, impl_->log_path);
    if (!impl_->video_output) {
        error = "cannot create video output";
        return false;
    }
    return impl_->video_output->start(window, error);
}

int Session::video_width() const {
    return impl_ && impl_->video_output ? impl_->video_output->width() : 0;
}

int Session::video_height() const {
    return impl_ && impl_->video_output ? impl_->video_output->height() : 0;
}

Session::~Session() {
    shutdown();
}

std::unique_ptr<Session> Session::create(std::string& error) {
    auto session = std::unique_ptr<Session>(new Session());
    session->impl_ = std::make_unique<SessionImpl>();

    mpv_handle* handle = mpv_create();
    if (handle == nullptr) {
        error = "mpv_create failed";
        return nullptr;
    }
    session->impl_->handle = handle;

    // 【不注册 wakeup 回调】
    // 它的唯一用途是打断阻塞中的 mpv_wait_event —— 而那条路径在真机上会崩
    // （见 mpv_event_loop.cpp）。我们现在用属性轮询，不需要它。
    // 不注册 ⇒ mpv 不会在 core 未就绪时回调我们 ⇒ 也少一个潜在崩溃点。
    return session;
}

void Session::set_event_callback(EventCallback callback) {
    if (impl_ && impl_) {
        impl_->event_callback = std::move(callback);
    }
}

void Session::set_render_callback(RenderCallback callback) {
    if (impl_ && impl_) {
        impl_->render_callback = std::move(callback);
    }
}

bool Session::initialize(const std::string& vo, const std::string& ao,
                        const std::string& log_file, std::string& error) {
    if (!impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    mpv_handle* handle = impl_->handle;
    impl_->log_path = log_file;
    mark_step(log_file.c_str(), "enter-initialize");

    // mpv 的日志**落文件**：这台 MIUI 设备上 adb logcat 读不到 app 进程的日志，
    // 而崩溃发生在 mpv 内部（native 崩溃没法用异常捕获）——
    // 所以必须让 mpv 自己把日志写出来，我们再读文件。
    if (!log_file.empty()) {
        mpv_set_option_string(handle, "log-file", log_file.c_str());
        mpv_set_option_string(handle, "msg-level", "all");
    }

    // 硬解走 mediacodec-copy（零拷贝进 Surface），与旧内核一致。
    // video-sync=audio：音画以音频为主时钟 —— 与旧内核的"音频主时钟"一致。
    mark_step(log_file.c_str(), "before-vo");
    mpv_set_option_string(handle, "vo", vo.c_str());
    mark_step(log_file.c_str(), "before-hwdec");
    // hwdec 是**运行时属性**：在 initialize 之前用 set_option 设会被 mpv 用默认值覆盖
    // （真机日志：明明设了 mediacodec-copy，mpv 仍去加载 aimagereader 驱动）。
    // 所以这里只记下来，initialize 之后再用 set_property 设。
    impl_->hwdec_ = "mediacodec-copy";
    // 硬解只对 mpv-android 明确支持的编码启用 —— 与 mpv-android 的
    // hwdec-codecs="h264,hevc,mpeg4,mpeg2video,vp8,vp9,av1" 保持一致。
    // VC-1 / TrueHD 这类平台没有硬解的，走 FFmpeg 软解（mpv 内部自动回退）。
    mpv_set_option_string(handle, "hwdec-codecs", "h264,hevc,mpeg4,mpeg2video,vp8,vp9,av1");
    // 音频输出：ao 由调用方给。
    // 【为什么现在能用了】mpv 的 `ao=audiotrack` 通过 JNI 调 Java 的 AudioTrack，
    // 需要 JavaVM —— 我们在 nativeCreate 里已经用 av_jni_set_java_vm 注册过
    // （见 mpv_java_vm.cpp），所以这里可以放心启用。
    // 【为什么默认仍是 null】Kotlin 侧默认传空（不出声），
    // 便于在"只验证内核"与"完整播放"之间切换。
    mark_step(log_file.c_str(), "before-ao");
    mpv_set_option_string(handle, "ao", ao.empty() ? "null" : ao.c_str());
    mpv_set_option_string(handle, "video-sync", "audio");
    mpv_set_option_string(handle, "hr-seek", "default");
    // demuxer 缓存上限：mpv-android 在移动端把它压到 32~64 MB
    // （"defaults are too high for mobile devices"）。我们照做 ——
    // 缓存太大在 SMB 上会拖慢起播，且占内存。
    mpv_set_option_string(handle, "demuxer-max-bytes", "64MiB");
    mpv_set_option_string(handle, "demuxer-max-back-bytes", "64MiB");

    // ---- 字幕（253-P6-2）----
    //
    // 【为什么 mpv 自己就能渲染字幕】
    // vo=libmpv 的渲染后端（video/out/gpu/libmpv_gpu.c）会调
    // `gl_video_set_osd_source(vo->osd)`，而 osd_render 内部走 libass ——
    // 也就是说**字幕是被 mpv 画进视频帧的**，不需要我们再叠一层 Compose View。
    // 这正是 SenPlayer / Rex 的做法（它们也是 mpv 内核）。
    //
    // 【为什么必须显式指字体目录】
    // 我们编的 libass 是 `CONFIG_FONTCONFIG` 未定义的版本（见
    // libass-android/build/libass-0.16.0/config.h），也就是**没有 fontconfig
    // 字体提供器**。libass 在这种情况下只会用两种字体来源：
    //   · 内嵌在字幕文件里的字体；
    //   · `ass_set_fonts_dir(dir)` 指定目录里的字体文件（ass_add_font 逐个装入）。
    // 不给目录 ⇒ 中文字幕全是空白或豆腐块。
    // Android 的系统字体在 /system/fonts，所以把 sub-fonts-dir 指过去。
    //
    // 【为什么不用 fontconfig】
    // Android 上没有 fontconfig 库；为它交叉编译一份并维护其字体缓存
    // 是纯粹的额外成本，而系统字体目录直读已经够用。
    mpv_set_option_string(handle, "sub-fonts-dir", "/system/fonts");
    // 默认字体族名。
    //
    // 【为什么必须显式给族名，而不是留空或 "sans-serif"】
    // 无 fontconfig 的 libass 匹配链只有三步：
    //   1. 指定的族名（family_default）—— 唯一能用的一步；
    //   2. 字体提供器的 fallback —— 我们没有提供器（"can't find selected font provider"）；
    //   3. 配置文件里的默认字体文件（subfont.ttf）—— 我们没放。
    // 留空 ⇒ 第 1 步匹配空串、失败；"sans-serif" ⇒ 无 fontconfig 时
    // 没有任何族叫这个名字，同样失败（实测日志：
    // "fontselect: failed to find any fallback with glyph 0x0"）。
    // "Noto Sans CJK JP" 是 Android 系统字体里确定存在的族名，
    // 且它带完整 CJK 字形（NotoSansCJK-Regular.ttc）。
    //
    // 【为什么是 JP 而不是 SC】
    // libass 加载字体目录时逐个文件调 ass_add_font，而 ass_add_font 对
    // **TrueType Collection 只取 0 号 face**。Android 的
    // NotoSansCJK-Regular.ttc 里 0 号 face 是 JP（SC/TC/HK/KR 排在后面）。
    // 所以只有 JP 这个族名能被匹配上 —— 设 SC 会静默失败
    // （实测：设 SC 时日志里没有 "Using default font family"，
    //  只有 "failed to find any fallback"）。
    // 字形层面无损失：CJK 全字集每个 face 都带，中文照样显示。
    //
    // 【为什么 osd-font 与 sub-font 都要设】
    // mpv 把字幕转成 ASS 事件后，**由 OSD 层统一渲染**（osd_render），
    // 而 osd_render 取的是 **osd_style** 的字体设置 —— 不是 sub_style。
    // 只设 sub-font 的实测结果是日志仍然打印 (sans-serif, 400, 0)，即完全没生效。
    // 两个都设，才能覆盖"OSD 文本"与"字幕文本"两条路径。
    mpv_set_option_string(handle, "osd-font", "Noto Sans CJK JP");
    mpv_set_option_string(handle, "sub-font", "Noto Sans CJK JP");
    // 自动加载同名字幕文件（外挂 .srt/.ass 放在媒体旁边）。
    mpv_set_option_string(handle, "sub-auto", "fuzzy");
    // 字幕字号：mpv 默认按视频高度的比例算字号。
    // 【为什么用 sub-font-size 而不是 sub-scale】
    // `sub-scale` 是 0~100 的**百分比因子**，且官方明确警告它会破坏 ASS 字幕
    // （会连同脚本里的 \fs 一起缩放）。`sub-font-size` 是绝对字号，
    // 只对纯文本字幕的默认字号生效 —— 对 ASS 是"没有指定 \fs 时用多大"的兜底。
    // 52 px 在 1080 高的画面上约等于 5%，与主流播放器接近。
    mpv_set_option_string(handle, "sub-font-size", "52");
    // 硬字幕（位图字幕，如 PGS/VobSub）在 mpv 里也走同一条 osd 通路，
    // 不需要额外开关。

    mark_step(log_file.c_str(), "before-mpv_initialize");
    int rc = mpv_initialize(handle);
    mark_step(log_file.c_str(), "after-mpv_initialize");
    if (rc < 0) {
        error = "mpv_initialize failed: " + std::string(mpv_error_string(rc));
        return false;
    }

    // hwdec 必须在 initialize **之后**设（见上面的说明）。
    if (!impl_->hwdec_.empty()) {
        mpv_set_property_string(handle, "hwdec", impl_->hwdec_.c_str());
    }

    impl_->event_thread = std::thread([state = impl_.get()] {
        state->run_event_loop();
    });
    return true;
}

std::string Session::error() const {
    return std::string();
}

void Session::shutdown() {
    if (!impl_ || !impl_) {
        return;
    }
    auto& state = impl_;
    state->stopping.store(true, std::memory_order_release);
    if (state->event_thread.joinable()) {
        state->wake_cv.notify_all();
        state->event_thread.join();
    }
    // 【顺序要紧】先销毁渲染层（它会调 mpv_render_context_free），
    // 再销毁 mpv 句柄。mpv 要求 free 早于 terminate_destroy，
    // 否则报 "Broken API use: mpv_render_context_free() not called." 并 abort。
    if (state->video_output) {
        state->video_output->stop();
        state->video_output.reset();
    }
    if (state->handle != nullptr) {
        mpv_terminate_destroy(state->handle);
        state->handle = nullptr;
    }
    impl_.reset();
}

}  // namespace kivo::video::mpv
