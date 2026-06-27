#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// AsyncTaskOwnership defines task ownership for async non-blocking operations.
/// Detached tasks without owner are forbidden.
/// schema_version: p3.execution.async_task.v1
struct AsyncTaskOwnership
{
    std::string schema_version{"p3.execution.async_task.v1"};
    std::string task_id{};
    std::string owner_id{};
    bool is_owned{false};
    int64_t deadline_ns{0};

    bool has_owner() const noexcept { return is_owned && !owner_id.empty(); }
    bool is_valid() const noexcept { return !task_id.empty() && has_owner(); }
};

} // namespace kivo::p3::execution
