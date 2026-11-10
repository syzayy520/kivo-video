// libsmb2 的通道原语 JNI 桥（220 定案：与 SenPlayer 同款库同形态；250 做健壮性收口）。
//
// 单一职责：连接、打开、读、关闭、错误读取。目录列举与共享枚举在
// `jni_smb2_list.cxx`（那两个是 JSON 生产者，改动的理由不同）。
//
// 线程模型：一个 context 一个线程串行调；并发由 Kotlin 侧串行化。
// 错误纪律（G9）：每个调用后检查 rc，失败返回原值，由 Kotlin 组装用户文案。
//
// 250 的两处结构变化：
// 1. 字符串一律经 `jni_string_convert`（UTF-8 ↔ UTF-16）。原先用
//    `NewStringUTF` / `GetStringUTFChars`，它们走 modified UTF-8，与 SMB 的
//    标准 UTF-8 不兼容：含 emoji 的文件名要么 abort、要么路径字节错。
// 2. 连接参数收**已规范化的 authority**（`[v6]:port`），不再在这里拼
//    `host + ":" + port`：IPv6 漏了方括号就永远连不上，而这个拼串原先藏在这一层。

#include <jni.h>
#include <fcntl.h>

#include <cstring>
#include <string>

#include "jni_smb2_context.hpp"
#include "jni_string_convert.hpp"
#include "smb2.h"
#include "libsmb2.h"

namespace {

using kivo::jni_bridge::ctx_of;
using kivo::jni_bridge::fh_of;
using kivo::jni_bridge::to_utf8;
using kivo::jni_bridge::user_or_null;

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeCreate(JNIEnv* /*env*/, jobject /*thiz*/) {
    struct smb2_context* ctx = smb2_init_context();
    if (ctx != nullptr) {
        smb2_set_security_mode(ctx, SMB2_NEGOTIATE_SIGNING_ENABLED);
        smb2_set_timeout(ctx, 15);
    }
    return reinterpret_cast<jlong>(ctx);
}

JNIEXPORT void JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeDestroy(JNIEnv* /*env*/, jobject /*thiz*/, jlong ctxPtr) {
    if (auto* ctx = ctx_of(ctxPtr)) {
        smb2_destroy_context(ctx);
    }
}

// 读阶段超时（250 D2）。连接与打开文件仍用 nativeCreate 里那 15 s —— 慢网络上的
// 认证 + 树连接真要几秒；单次 READ 真机最坏 300 ms 量级，3 s 已是 10 倍余量。
//
// 注意：真机实测这个值在"连接半死"时**不生效**（断网后 pread 仍 15 s 才回），
// 所以断网感知由 Kotlin 侧 `NetworkLinkMonitor` + `RangeFd` 看门狗承担。
// 这里保留是为了给"真的卡住的 PDU"一个上限，不承担用户体验指标。
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeSetReadTimeout(JNIEnv* /*env*/, jobject /*thiz*/,
                                                         jlong ctxPtr, jint seconds) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr || seconds <= 0) return -1;
    smb2_set_timeout(ctx, static_cast<int>(seconds));
    return 0;
}

// [authority] 形如 `host:port` / `[::1]:445`，由 `SmbAddress.authority` 规范化后传入。
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeConnect(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                                  jstring authority, jstring share,
                                                  jstring user, jstring pass, jstring domain) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr) return -1;
    const std::string hostPort = to_utf8(env, authority);
    const std::string shareName = to_utf8(env, share);
    const std::string userName = to_utf8(env, user);
    const std::string password = to_utf8(env, pass);
    const std::string domainName = to_utf8(env, domain);

    if (!domainName.empty()) smb2_set_domain(ctx, domainName.c_str());
    if (!userName.empty()) smb2_set_user(ctx, userName.c_str());
    // 密码为空 → 不调 smb2_set_password（保持 NULL）——libsmb2 auth.c L760：
    // password==NULL 才走匿名分支（NTLMSSP_NEGOTIATE_ANONYMOUS 0x800 标志）。
    if (!password.empty()) smb2_set_password(ctx, password.c_str());

    return static_cast<jint>(
        smb2_connect_share(ctx, hostPort.c_str(), shareName.c_str(), user_or_null(userName)));
}

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeOpen(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                               jstring path) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr) return 0;
    return reinterpret_cast<jlong>(smb2_open(ctx, to_utf8(env, path).c_str(), O_RDONLY));
}

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeSize(JNIEnv* /*env*/, jobject /*thiz*/, jlong ctxPtr,
                                               jlong fhPtr) {
    auto* ctx = ctx_of(ctxPtr);
    auto* fh = fh_of(fhPtr);
    if (ctx == nullptr || fh == nullptr) return -1;
    struct smb2_stat_64 st;
    std::memset(&st, 0, sizeof(st));
    if (smb2_fstat(ctx, fh, &st) != 0) return -1;
    return static_cast<jlong>(st.smb2_size);
}

// 直接读进调用方给的 ByteArray（243 S2）：只跨界一次，省掉 native 侧 vector +
// NewByteArray + 二次拷贝。
//
// 250 D9：**失败返回 rc 原值**（NT status 或 -errno），不再压成 -1。
// 压成 -1 就只能拿 smb2_get_nterror 去解释，而超时路径没有回复报文、那个值是
// 上一次回复留下的 —— "这次超时"会被译成"上次的权限错误"。
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeReadInto(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                                   jlong fhPtr, jlong offset, jbyteArray buf,
                                                   jint off, jint len) {
    auto* ctx = ctx_of(ctxPtr);
    auto* fh = fh_of(fhPtr);
    if (ctx == nullptr || fh == nullptr || buf == nullptr || len <= 0 || off < 0) return -1;
    if (env->GetArrayLength(buf) < off + len) return -1;
    jbyte* dst = env->GetByteArrayElements(buf, nullptr);
    if (dst == nullptr) return -1;
    const int64_t read = smb2_pread(ctx, fh, reinterpret_cast<uint8_t*>(dst + off),
                                    static_cast<uint32_t>(len), static_cast<uint64_t>(offset));
    env->ReleaseByteArrayElements(buf, dst, 0);  // 0 = 拷回并释放
    return static_cast<jint>(read);
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeCloseFh(JNIEnv* /*env*/, jobject /*thiz*/, jlong ctxPtr,
                                                  jlong fhPtr) {
    auto* ctx = ctx_of(ctxPtr);
    auto* fh = fh_of(fhPtr);
    if (ctx == nullptr || fh == nullptr) return -1;
    return static_cast<jint>(smb2_close(ctx, fh));
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeGetError(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr) return kivo::jni_bridge::to_jstring(env, "context null");
    return kivo::jni_bridge::to_jstring(env, smb2_get_error(ctx));
}

// 最近一次失败的 NT status（243 D7 错误分类用）；0 = 无错误。
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeNtStatus(JNIEnv* /*env*/, jobject /*thiz*/,
                                                   jlong ctxPtr) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr) return 0;
    return static_cast<jint>(smb2_get_nterror(ctx));
}

}  // extern "C"
