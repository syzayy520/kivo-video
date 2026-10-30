#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// BudgetKind enumerates budget behavior for serial/parallel/cleanup work.
enum class BudgetKind
{
    shared_deadline,
    partitioned_budget,
    reserved_budget,
    best_effort
};

/// BudgetAllocator defines budget behavior for serial/parallel/cleanup work.
/// Reserved cleanup/evidence/process-kill budget is mandatory.
/// schema_version: p3.execution.budget_allocator.v1
struct BudgetAllocator
{
    std::string schema_version{"p3.execution.budget_allocator.v1"};
    BudgetKind kind{BudgetKind::shared_deadline};
    int64_t total_budget_ns{0};
    int64_t reserved_cleanup_ns{0};
    int64_t reserved_evidence_ns{0};
    int64_t reserved_process_kill_ns{0};
    int64_t consumed_ns{0};

    int64_t available_ns() const noexcept { return total_budget_ns - consumed_ns; }
    bool has_reserved_cleanup() const noexcept { return reserved_cleanup_ns > 0; }
    bool has_reserved_process_kill() const noexcept { return reserved_process_kill_ns > 0; }
    bool can_consume(int64_t ns) const noexcept { return available_ns() >= ns; }
};

} // namespace kivo::p3::execution
