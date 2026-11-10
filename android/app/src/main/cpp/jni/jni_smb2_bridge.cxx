// libsmb2 的 JNI 桥（220 设计稿 D1 定案实验：与 SenPlayer 同款库同形态）。
//
// 单一职责：libsmb2 同步 API 的薄封装——无解码/播放逻辑。
// 线程模型：一个 context 一个线程串行调（spike 单线程；LocalRangeProxy 侧再加锁）。
// 错误纪律（G9）：每个调用后检查 rc，错误文本经 nativeGetError 交给 Kotlin 组装 reason。

#include <jni.h>
#include <fcntl.h>
#include <string>
#include <cstring>
#include <vector>

#include "smb2.h"
#include "libsmb2.h"
#include "libsmb2-raw.h"
#include "libsmb2-share-enum.h"

namespace {

jstring to_jstring(JNIEnv* env, const char* s) {
    return env->NewStringUTF(s == nullptr ? "" : s);
}

std::string to_std(JNIEnv* env, jstring s) {
    if (s == nullptr) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c == nullptr ? "" : c;
    if (c != nullptr) env->ReleaseStringUTFChars(s, c);
    return out;
}

// user 为空 → 传 NULL（libsmb2 匿名语义，SenPlayer 空凭据形态对位）
const char* user_or_null(const std::string& user) {
    return user.empty() ? nullptr : user.c_str();
}

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
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx != nullptr) smb2_destroy_context(ctx);
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeConnect(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                                  jstring host, jint port, jstring share,
                                                  jstring user, jstring pass, jstring domain) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx == nullptr) return -1;
    std::string h = to_std(env, host);
    std::string s = to_std(env, share);
    std::string u = to_std(env, user);
    std::string p = to_std(env, pass);
    std::string d = to_std(env, domain);

    if (!d.empty()) smb2_set_domain(ctx, d.c_str());
    if (!u.empty()) smb2_set_user(ctx, u.c_str());
    // 密码为空 → 不调 smb2_set_password（保持 NULL）——libsmb2 auth.c L760：
    // password==NULL 才走匿名分支（NTLMSSP_NEGOTIATE_ANONYMOUS 0x800 标志）。
    // 空字符串 ≠ NULL：会发"空用户+空密码 NTLMv2"，小米路由实测拒（0xc0000022）。
    if (!p.empty()) smb2_set_password(ctx, p.c_str());

    std::string hostPort = h + ":" + std::to_string(port);
    int rc = smb2_connect_share(ctx, hostPort.c_str(), s.c_str(), user_or_null(u));
    return static_cast<jint>(rc);
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeList(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                               jstring path) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx == nullptr) return to_jstring(env, "[]");
    std::string dir = to_std(env, path);
    while (dir.size() > 1 && dir.back() == '/') dir.pop_back();

    struct smb2dir* d = smb2_opendir(ctx, dir.empty() ? nullptr : dir.c_str());
    if (d == nullptr) {
        // 错误文本由 Kotlin 经 nativeGetError 读取
        return nullptr;
    }
    std::string json = "[";
    bool first = true;
    struct smb2dirent* ent;
    while ((ent = smb2_readdir(ctx, d)) != nullptr) {
        std::string name = ent->name;
        if (name == "." || name == "..") continue;
        if (!first) json += ",";
        first = false;
        bool isDir = ent->st.smb2_type == SMB2_TYPE_DIRECTORY;
        json += "{\"name\":\"";
        // 最小 JSON 转义（文件名可能含引号/反斜杠）
        for (char c : name) {
            if (c == '"' || c == '\\') json += '\\';
            json += c;
        }
        json += "\",\"dir\":";
        json += isDir ? "true" : "false";
        json += ",\"size\":";
        json += std::to_string(ent->st.smb2_size);
        json += "}";
    }
    smb2_closedir(ctx, d);
    json += "]";
    return to_jstring(env, json.c_str());
}

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeOpen(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                               jstring path) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx == nullptr) return 0;
    std::string p = to_std(env, path);
    struct smb2fh* fh = smb2_open(ctx, p.c_str(), O_RDONLY);
    return reinterpret_cast<jlong>(fh);
}

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeSize(JNIEnv* /*env*/, jobject /*thiz*/, jlong ctxPtr,
                                               jlong fhPtr) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    auto* fh = reinterpret_cast<struct smb2fh*>(fhPtr);
    if (ctx == nullptr || fh == nullptr) return -1;
    struct smb2_stat_64 st;
    std::memset(&st, 0, sizeof(st));
    if (smb2_fstat(ctx, fh, &st) != 0) return -1;
    return static_cast<jlong>(st.smb2_size);
}

JNIEXPORT jbyteArray JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeReadAt(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                                 jlong fhPtr, jlong offset, jint len) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    auto* fh = reinterpret_cast<struct smb2fh*>(fhPtr);
    if (ctx == nullptr || fh == nullptr || len <= 0) return nullptr;
    std::vector<char> buf(static_cast<size_t>(len));
    int64_t n = smb2_pread(ctx, fh, reinterpret_cast<uint8_t*>(buf.data()),
                           static_cast<uint32_t>(len), static_cast<uint64_t>(offset));
    if (n < 0) return nullptr;
    jbyteArray arr = env->NewByteArray(static_cast<jsize>(n));
    if (arr != nullptr) {
        env->SetByteArrayRegion(arr, 0, static_cast<jsize>(n),
                                reinterpret_cast<const jbyte*>(buf.data()));
    }
    return arr;
}

// 直接读进调用方给的 ByteArray（243 S2）：只跨界一次，省掉 native 侧 vector +
// NewByteArray + 二次拷贝。返回读到的字节数；0 = EOF；<0 = 失败（读 nativeGetError / nativeNtStatus）。
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeReadInto(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                                   jlong fhPtr, jlong offset, jbyteArray buf,
                                                   jint off, jint len) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    auto* fh = reinterpret_cast<struct smb2fh*>(fhPtr);
    if (ctx == nullptr || fh == nullptr || buf == nullptr || len <= 0 || off < 0) return -1;
    if (env->GetArrayLength(buf) < off + len) return -1;
    jbyte* dst = env->GetByteArrayElements(buf, nullptr);
    if (dst == nullptr) return -1;
    int64_t n = smb2_pread(ctx, fh, reinterpret_cast<uint8_t*>(dst + off),
                           static_cast<uint32_t>(len), static_cast<uint64_t>(offset));
    env->ReleaseByteArrayElements(buf, dst, 0);  // 0 = 拷回并释放
    if (n < 0) return -1;
    return static_cast<jint>(n);
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeCloseFh(JNIEnv* /*env*/, jobject /*thiz*/, jlong ctxPtr,
                                                  jlong fhPtr) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    auto* fh = reinterpret_cast<struct smb2fh*>(fhPtr);
    if (ctx == nullptr || fh == nullptr) return -1;
    return static_cast<jint>(smb2_close(ctx, fh));
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeGetError(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx == nullptr) return to_jstring(env, "context null");
    return to_jstring(env, smb2_get_error(ctx));
}

// 共享枚举（243 D1）：必须已连到 IPC$。返回 JSON 数组
// [{"name":..,"remark":..,"type":..}]；失败返回 null，错误文本仍走 nativeGetError。
// type 为 SMB2_SHARE_TYPE_*（0=磁盘 1=打印 2=设备 3=IPC），隐藏共享带 0x80000000。
JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeListShares(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx == nullptr) return nullptr;
    struct smb2_share_enum_reply* reply =
        smb2_share_enum_sync(ctx, SMB2_SHARE_INFO_1);
    if (reply == nullptr) return nullptr;

    std::string json = "[";
    bool first = true;
    for (uint32_t i = 0; i < reply->entries_read; i++) {
        struct smb2_share_info_1* info = &reply->share_info.info_1[i];
        if (info->netname == nullptr) continue;
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"";
        for (const char* p = info->netname; *p; ++p) {
            if (*p == '"' || *p == '\\') json += '\\';
            json += *p;
        }
        json += "\",\"remark\":\"";
        if (info->remark != nullptr) {
            for (const char* p = info->remark; *p; ++p) {
                if (*p == '"' || *p == '\\') json += '\\';
                json += *p;
            }
        }
        json += "\",\"type\":";
        json += std::to_string(info->type);
        json += "}";
    }
    json += "]";
    smb2_free_data(ctx, reply);
    return to_jstring(env, json.c_str());
}

// 最近一次失败的 NT status（243 D7 错误分类用）；0 = 无错误。
JNIEXPORT jint JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeNtStatus(JNIEnv* /*env*/, jobject /*thiz*/, jlong ctxPtr) {
    auto* ctx = reinterpret_cast<struct smb2_context*>(ctxPtr);
    if (ctx == nullptr) return 0;
    return static_cast<jint>(smb2_get_nterror(ctx));
}

}  // extern "C"
