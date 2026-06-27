#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::p3::execution {

/// ExecutionContext propagates operational context to all child work.
/// Every provider/probe/session task must run with a valid context.
/// schema_version: p3.execution.context.v1
struct ExecutionContext
{
    std::string schema_version{"p3.execution.context.v1"};
    std::string operation_id{};
    std::string trace_context{};
    std::string cancellation_signal{};
    std::optional<int64_t> deadline_monotonic_ns{};
    std::optional<int64_t> budget_allocator{};
    std::optional<int64_t> retry_budget{};
    std::optional<int64_t> resource_budget{};
    std::string network_policy{};
    std::string evidence_policy{};
    std::string redaction_policy{};
    std::string task_group_policy{};
    std::optional<int64_t> playback_latency_budget_ns{};
    std::string platform_capability_hints{};
    std::string clock_provider{};
    std::string id_provider{};
    std::string feature_flags{};

    bool has_deadline() const noexcept { return deadline_monotonic_ns.has_value(); }
    bool has_budget() const noexcept { return budget_allocator.has_value(); }
    bool is_valid() const noexcept { return !operation_id.empty(); }
};

} // namespace kivo::p3::execution
