#pragma once

#include <jni.h>

#include <string>

#include "smb2.h"
#include "libsmb2.h"

// libsmb2 JNI 桥的共用小工具（250 S4）。
//
// 存在的唯一理由：`jni_smb2_bridge.cxx`（通道原语）与 `jni_smb2_list.cxx`
// （两个 JSON 生产者）都要做同样的指针转换与匿名判定。各写一份会出现
// "一边改了、另一边没改"的真相分裂 —— 而这类分裂在 JNI 里就是崩溃或静默错误。

namespace kivo::jni_bridge {

inline struct smb2_context* ctx_of(jlong ptr) {
    return reinterpret_cast<struct smb2_context*>(ptr);
}

inline struct smb2fh* fh_of(jlong ptr) {
    return reinterpret_cast<struct smb2fh*>(ptr);
}

/// 用户名为空 → 传 NULL。
///
/// libsmb2 的匿名分支要求 password == NULL（`auth.c` L760），而 `smb2_connect_share`
/// 的 user 参数同理。**空字符串 ≠ NULL**：空串会发"空用户 + 空密码 NTLMv2"，
/// 小米路由实测拒（0xc0000022），匿名共享因此连不上。
inline const char* user_or_null(const std::string& user) {
    return user.empty() ? nullptr : user.c_str();
}

}  // namespace kivo::jni_bridge
