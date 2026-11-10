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
// B2 写的是"不得直接调用 FFmpeg API"。本文件是**唯一**的例外，且是受控例外：
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

#ifndef KIVO_VIDEO_MPV_JAVA_VM_HPP
#define KIVO_VIDEO_MPV_JAVA_VM_HPP

#include <jni.h>

#include <string>

namespace kivo::video::mpv {

/// 把 JavaVM 与 Android Context 注册给 FFmpeg（供 mpv 的硬解/音频输出使用）。
///
/// @param env  调用方的 JNI 环境（必须非空）。
/// @param app_context Android Context（可为 null —— 某些路径不需要，
///                但硬解路径需要它来创建 MediaCodec 对象）。
/// @return true = 注册成功；false = 失败（原因见 out_error）。
///
/// 【线程】可在任意线程调用。FFmpeg 的注册函数内部有锁，可重复调用
/// （传同一个 VM 指针是允许的）。
bool register_java_vm(JNIEnv* env, jobject app_context, std::string& out_error);

/// 查询当前已注册的 JavaVM（诊断用；未注册返回 nullptr）。
void* registered_java_vm();

}  // namespace kivo::video::mpv

#endif  // KIVO_VIDEO_MPV_JAVA_VM_HPP
