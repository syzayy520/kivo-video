// 253 家族：会话表的实现 —— **只做一件事**：句柄 ↔ Session 的映射与它的锁。
//
// 【为什么单独一个文件】
// JNI 导出层被拆成多个文件（生命周期 / 命令 / 属性 / 轨道）。
// 它们都要查同一张表，而表的锁**只能有一把**。
// 锁在表这一层，两个 JNI 文件就都只是"用表"，而不是各自造锁。
//
// 【锁的形态】
// 一把 std::mutex + 全局表。所有 JNI 调用都经过它，所以必须加锁。
//
// 【为什么提供"作用域锁"而不是让每个 JNI 函数自己 lock/unlock】
// JNI 函数常见形态是"查表 → 拿到指针 → 调方法 → 可能删表"。
// 如果查表时加锁、调完解锁，中间指针可能已被别的线程释放（use-after-free）。
// 所以正确形态是：**整段持锁**。
// 用 ScopeLock 表达这件事，比每个函数手写 lock_guard 更不容易漏。

#include "mpv_session_table.hpp"

#include <mutex>
#include <unordered_map>

#include "mpv_session.hpp"

namespace kivo::video::mpv {
namespace {

std::mutex& table_lock() {
    static std::mutex lock;
    return lock;
}

std::unordered_map<SessionHandle, std::unique_ptr<Session>>& session_table() {
    static std::unordered_map<SessionHandle, std::unique_ptr<Session>> table;
    return table;
}

}  // namespace

SessionHandle next_session_handle() {
    static SessionHandle counter = 1;
    return counter++;
}

SessionHandle register_session(std::unique_ptr<Session> session) {
    const SessionHandle handle = next_session_handle();
    std::lock_guard lock(table_lock());
    session_table()[handle] = std::move(session);
    return handle;
}

Session* find_session_locked(SessionHandle handle) {
    auto& table = session_table();
    auto it = table.find(handle);
    return it == table.end() ? nullptr : it->second.get();
}

Session* find_session(SessionHandle handle) {
    std::lock_guard lock(table_lock());
    return find_session_locked(handle);
}

bool erase_session(SessionHandle handle) {
    std::lock_guard lock(table_lock());
    auto& table = session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        return false;
    }
    // 销毁发生在锁内：Session 的析构会调 mpv_terminate_destroy，
    // 那会阻塞一小段时间（等内核退出）。放在锁外能缩短临界区，
    // 但那样"表里已删、对象还活着"的窗口里，另一个线程可能拿到同一指针 ——
    // 所以这里选择**在锁内销毁**：正确性优先，临界区只多几十毫秒。
    table.erase(it);
    return true;
}

struct SessionTableLock::Impl {
    std::unique_lock<std::mutex> lock;
    Impl() : lock(table_lock()) {}
};

SessionTableLock::SessionTableLock() : impl_(std::make_unique<Impl>()) {}

SessionTableLock::~SessionTableLock() = default;

}  // namespace kivo::video::mpv
