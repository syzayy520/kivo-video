#pragma once

// 253 家族：会话表 —— **只做一件事**：把 Kotlin 侧的 long 句柄映射到 C++ 的 Session。
//
// 【为什么要有它】
// JNI 导出层被拆成多个文件（生命周期/命令/属性），它们都要查同一个会话表。
// 表必须**只有一份**，否则同一个句柄在两个文件里会指向两个对象 —— 释放时序直接崩。
//
// 【为什么用表而不是直接传指针】
// Kotlin 侧只拿得到一个 long。裸指针在 use-after-free 时会变成野指针崩溃，
// 而表 + 序号能让"句柄已释放"变成一个**明确的错误码**。
//
// 【线程模型】
// 所有访问都要经过本表内部的锁。JNI 层不自己加锁 —— 锁的归属在这里，
// 否则每个 JNI 文件都要重复一遍加锁逻辑（而且总有一个会忘）。

#include <cstdint>
#include <memory>

namespace kivo::video::mpv {

class Session;

/// 会话表句柄。
///
/// 【为什么是整数而不是指针】
/// 句柄只在 JNI 边界上跨语言传递。整数让"句柄非法"成为可判定的事实，
/// 而不是"指针是不是还活着"这种没法在运行时区分的问题。
using SessionHandle = std::int64_t;

/// 分配一个新句柄（不与任何现存句柄冲突）。
SessionHandle next_session_handle();

/// 登记一个会话，返回它的句柄。
SessionHandle register_session(std::unique_ptr<Session> session);

/// 查一个会话。**返回裸指针**，调用方必须在同一把锁的保护下使用 ——
/// 也就是说：查出来立刻用，不要跨调用持有。
///
/// 找不到时返回 nullptr。
Session* find_session(SessionHandle handle);

/// 注销并销毁一个会话。返回 true = 之前确实存在。
bool erase_session(SessionHandle handle);

/// 会话表锁。JNI 层需要"查出来 + 用完"作为一个原子区间时用它。
///
/// 【为什么把它暴露出去】
/// 有些 JNI 函数是"查表 → 调用 → 可能改表"的组合。
/// 如果中间放锁，就成了"锁里的锁"（同一把非递归 mutex 自锁 ⇒ 死锁）；
/// 如果中间不放锁，就可能查到已被释放的指针。
/// 正确做法是 JNI 层用 `lock_session_table()` 拿到一个**作用域锁**，
/// 内部查询用 `find_session_locked()`（不加锁版本）。
class SessionTableLock {
public:
    SessionTableLock();
    ~SessionTableLock();
    SessionTableLock(const SessionTableLock&) = delete;
    SessionTableLock& operator=(const SessionTableLock&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// 已持有表锁时查会话（**不自己加锁**）。
Session* find_session_locked(SessionHandle handle);

}  // namespace kivo::video::mpv
