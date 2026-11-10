// 253 家族：JavaVM 注册 —— **只做一件事**：把 JavaVM / Android Context 注册到 FFmpeg。
//
// 【为什么需要它】
// mpv 的两条关键路径都要 JavaVM：
//   · 硬解 `hwdec=mediacodec*` → mpv 的 hwdec_aimagereader.c 调 av_jni_get_java_vm()
//   · 音频 `ao=audiotrack`      → mpv 的 ao_audiotrack.c 用 MP_JNI_GET_ENV()
// 而 mpv 自己**不注册** JavaVM —— 它假设宿主（mpv-android）替它做。
// mpv-android 的做法（app/src/main/jni/main.cpp:44）：
//     env->GetJavaVM(&g_vm);
//     av_jni_set_java_vm(g_vm, NULL);
//     av_jni_set_android_app_ctx(global_appctx, NULL);
// 我们照做，但把它隔离在本文件里。
//
// 【与家族边界 B2 的关系】
// B2 写的是"不得直接调用 FFmpeg API"。本文件是**唯一**的受控例外：
//   · 只调用 3 个函数：av_jni_set_java_vm / av_jni_set_android_app_ctx
//     / av_jni_get_java_vm —— 它们是 FFmpeg 的"环境注册"入口，不是解码/解复用 API；
//   · 本文件不含任何播放逻辑；
//   · 若上游 mpv 将来提供公开的 VM 注册接口，本文件应改调那个接口并删除此豁免。
// 详见 docs/design/backend-core/253-mpv-kernel-family-tree.md 的 B2 备注。
//
// 【为什么必须先注册再创建会话】
// mpv 的 `hwdec` 驱动与 `ao=audiotrack` 都在**第一次使用时**才去取 VM。
// 若那时 VM 还没注册，就会看到 mpv 日志里的
//   "No Java virtual machine has been registered"
// 表现为：mediacodec 硬解初始化失败 / 音频输出静默失败。
// 所以注册必须发生在 mpv_initialize 之前。

#include "mpv_java_vm.hpp"

// 【为什么自己声明而不是直接用 FFmpeg 的头】
// FFmpeg 的 libavcodec/jni.h **没有** `extern "C"` 包裹（它假定调用方是 C，
// mpv 就是在 C 文件 misc/jni.c 里 include 它的）。
// 我们是 C++ ⇒ 直接 include 会让函数名被 name mangling
// （_Z19av_jni_set_java_vmPvS_），链接器找不到 C 符号
// （实测报错就带着 "did you mean: extern \"C\" av_jni_set_java_vm"）。
// 所以这里用 extern "C" 显式声明，签名取自 FFmpeg 7.1 的 jni.h：
//     int  av_jni_set_java_vm(void *vm, void *log_ctx);
//     void *av_jni_get_java_vm(void *log_ctx);
//     int  av_jni_set_android_app_ctx(void *app_ctx, void *log_ctx);
extern "C" {
int av_jni_set_java_vm(void* vm, void* log_ctx);
void* av_jni_get_java_vm(void* log_ctx);
int av_jni_set_android_app_ctx(void* app_ctx, void* log_ctx);
}

#include <string>

namespace kivo::video::mpv {

bool register_java_vm(JNIEnv* env, jobject app_context, std::string& out_error) {
    if (env == nullptr) {
        out_error = "JNIEnv is null";
        return false;
    }

    JavaVM* vm = nullptr;
    if (env->GetJavaVM(&vm) != JNI_OK || vm == nullptr) {
        out_error = "GetJavaVM failed";
        return false;
    }

    // 注册 VM。FFmpeg 允许重复注册同一个指针。
    if (av_jni_set_java_vm(vm, nullptr) < 0) {
        out_error = "av_jni_set_java_vm failed";
        return false;
    }

    // 注册 Android Context：mediacodec 硬解要用它创建 MediaCodec 对象。
    // 必须传**全局引用** —— 局部引用在本次 JNI 调用返回后即失效。
    if (app_context != nullptr) {
        jobject global_context = env->NewGlobalRef(app_context);
        if (global_context == nullptr) {
            out_error = "NewGlobalRef(app_context) failed";
            return false;
        }
        if (av_jni_set_android_app_ctx(global_context, nullptr) < 0) {
            out_error = "av_jni_set_android_app_ctx failed";
            return false;
        }
        // 全局引用由 FFmpeg 持有（它会在使用后释放），这里不 DeleteGlobalRef。
    }

    return true;
}

void* registered_java_vm() {
    return av_jni_get_java_vm(nullptr);
}

}  // namespace kivo::video::mpv
