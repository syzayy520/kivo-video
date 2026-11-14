// 278-P2 · Surface dataspace 设置（真 HDR 输出用）
//
// 【为什么单独一个文件】
// 设置 Surface 的 dataspace 与 mpv 会话**无关**（不碰 mpv handle），
// 但与"画面输出"这个责任主体有关 ⇒ 独立成文件，避免塞进会话桥里混职责。
//
// 【为什么用 ANativeWindow_setBuffersDataSpace 而不是 Surface.setDataspace】
// · `Surface.setDataspace` 是 **API 33+**（我们 minSdk 26 ⇒ 编译不过）；
// · `ANativeWindow_setBuffersDataSpace` 是 **API 28+**（NDK 稳定 API），
//   且**正是 SenPlayer 用的那一个**（它的库里就有这个符号）⇒ 与它对齐。
//
// 【dataspace 值】
// `ADATASPACE_BT2020_PQ = 163971072`（android/data_space.h）。
// 语义 = BT.2020 色域 + SMPTE ST 2084(PQ) 传递函数 + 全范围。

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <jni.h>

extern "C" {

/**
 * 给 Surface 设定 dataspace。
 *
 * @param surface jobject（android.view.Surface）
 * @param dataspace ADATASPACE_* 常量
 * @return 0 = 成功；负数 = 失败（调用方据此退回 SDR，不要崩）
 */
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeSetSurfaceDataspace(JNIEnv* env, jclass,
                                                              jobject surface,
                                                              jint dataspace) {
    if (surface == nullptr) {
        return -1;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        return -2;
    }
    // API 28 才有这个函数，而我们 minSdk = 26
    // ⇒ 用 **dlsym** 运行时查符号（clang 的 __builtin_available 不认 NDK 的
    //   __INTRODUCED_IN 标注；dlsym 是唯一在所有 API 级别都能编过的做法）。
    //
    // 【为什么不能只用 RTLD_DEFAULT】Android 11 起这个符号从 libandroid.so
    //   **拆到了 libnativewindow.so** ⇒ RTLD_DEFAULT（在全局命名空间里找）
    //   在新系统上**查不到**（真机实测 rc=-3）。
    // ⇒ 两个库都要试。
    using SetDataSpaceFn = int32_t (*)(ANativeWindow*, int32_t);
    static SetDataSpaceFn set_ds = nullptr;
    static bool probed = false;
    if (!probed) {
        probed = true;
        void* sym = dlsym(RTLD_DEFAULT, "ANativeWindow_setBuffersDataSpace");
        if (sym == nullptr) {
            void* handle = dlopen("libnativewindow.so", RTLD_LAZY | RTLD_LOCAL);
            if (handle != nullptr) {
                sym = dlsym(handle, "ANativeWindow_setBuffersDataSpace");
            }
        }
        if (sym == nullptr) {
            void* handle = dlopen("libandroid.so", RTLD_LAZY | RTLD_LOCAL);
            if (handle != nullptr) {
                sym = dlsym(handle, "ANativeWindow_setBuffersDataSpace");
            }
        }
        set_ds = reinterpret_cast<SetDataSpaceFn>(sym);
    }
    if (set_ds == nullptr) {
        ANativeWindow_release(window);
        return -3;  // 设备低于 API 28 ⇒ 设不了（上层据此保持 SDR）
    }
    // NDK 的返回值：0 = 成功，负值 = 错误（见 native_window.h）。
    const int rc = set_ds(window, static_cast<int32_t>(dataspace));
    ANativeWindow_release(window);
    return rc;
}

}  // extern "C"
