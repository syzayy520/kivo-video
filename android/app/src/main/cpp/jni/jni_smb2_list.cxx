// libsmb2 的两个"JSON 生产者"（250 S4）：目录列举与共享枚举。
//
// 从 `jni_smb2_bridge.cxx` 拆出来的理由：它们是唯一把 libsmb2 的结构体序列化成
// JSON 的地方，会随返回格式独立变化；桥里其余部分是"通道原语"（连/开/读/关），
// 两者被改的原因完全不同。
//
// 所有字符串一律走 `append_json_string`：文件名里的换行、制表符、控制字符
// 以前没有转义，一个字节就能让 `JSONArray` 解析失败、整个目录报"无法读取文件夹"。

#include <jni.h>

#include <string>

#include "jni_smb2_context.hpp"
#include "jni_smb2_json.hpp"
#include "jni_string_convert.hpp"
#include "smb2.h"
#include "libsmb2.h"
#include "libsmb2-raw.h"
#include "libsmb2-share-enum.h"

namespace {

using kivo::jni_bridge::append_json_string;
using kivo::jni_bridge::ctx_of;
using kivo::jni_bridge::to_jstring;
using kivo::jni_bridge::to_utf8;

}  // namespace

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeList(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr,
                                               jstring path) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr) return to_jstring(env, "[]");
    std::string dir = to_utf8(env, path);
    while (dir.size() > 1 && dir.back() == '/') dir.pop_back();

    struct smb2dir* handle = smb2_opendir(ctx, dir.empty() ? nullptr : dir.c_str());
    if (handle == nullptr) return nullptr;  // 错误文本由 Kotlin 经 nativeGetError 读取

    std::string json = "[";
    bool first = true;
    struct smb2dirent* ent;
    while ((ent = smb2_readdir(ctx, handle)) != nullptr) {
        const std::string name = ent->name;
        if (name == "." || name == "..") continue;
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"";
        append_json_string(json, name);
        json += "\",\"dir\":";
        json += ent->st.smb2_type == SMB2_TYPE_DIRECTORY ? "true" : "false";
        json += ",\"size\":";
        json += std::to_string(ent->st.smb2_size);
        json += "}";
    }
    smb2_closedir(ctx, handle);
    json += "]";
    return to_jstring(env, json);
}

// 共享枚举（243 D1）：必须已连到 IPC$。返回 JSON 数组
// [{"name":..,"remark":..,"type":..}]；失败返回 null，错误文本仍走 nativeGetError。
// type 为 SMB2_SHARE_TYPE_*（0=磁盘 1=打印 2=设备 3=IPC），隐藏共享带 0x80000000。
JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_smb_Smb2Native_nativeListShares(JNIEnv* env, jobject /*thiz*/, jlong ctxPtr) {
    auto* ctx = ctx_of(ctxPtr);
    if (ctx == nullptr) return nullptr;
    struct smb2_share_enum_reply* reply = smb2_share_enum_sync(ctx, SMB2_SHARE_INFO_1);
    if (reply == nullptr) return nullptr;

    std::string json = "[";
    bool first = true;
    for (uint32_t i = 0; i < reply->entries_read; i++) {
        struct smb2_share_info_1* info = &reply->share_info.info_1[i];
        if (info->netname == nullptr) continue;
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"";
        append_json_string(json, info->netname);
        json += "\",\"remark\":\"";
        if (info->remark != nullptr) append_json_string(json, info->remark);
        json += "\",\"type\":";
        json += std::to_string(info->type);
        json += "}";
    }
    json += "]";
    smb2_free_data(ctx, reply);
    return to_jstring(env, json);
}

}  // extern "C"
