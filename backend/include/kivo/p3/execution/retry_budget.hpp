#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// RetryBudget bounds retries. Retry only idempotent operations.
/// CircuitBreaker blocks new attempts only, does not interrupt P4 healthy session.
/// schema_version: p3.execution.retry_budget.v1
struct RetryBudget
{
    std::string schema_version{"p3.execution.retry_budget.v1"};
    int32_t max_retries{3};
    int32_t retries_used{0};
    int64_t retry_after_ms{0};
    bool is_idempotent{false};

    bool can_retry() const noexcept { return is_idempotent && retries_used < max_retries; }
    void consume() noexcept { ++retries_used; }
};

/// CircuitBreakerState enumerates circuit breaker states.
enum class CircuitBreakerState
{
    closed,
    open,
    half_open
};

/// CircuitBreaker blocks new attempts only.
struct CircuitBreaker
{
    std::string schema_version{"p3.execution.circuit_breaker.v1"};
    CircuitBreakerState state{CircuitBreakerState::closed};
    int32_t failure_threshold{5};
    int32_t failure_count{0};
    int64_t open_until_ns{0};
    int64_t now_ns{0};

    bool allows_new_attempt() const noexcept;
    bool is_open() const noexcept { return state == CircuitBreakerState::open; }
};

inline bool CircuitBreaker::allows_new_attempt() const noexcept
{
    if (state == CircuitBreakerState::closed) return true;
    if (state == CircuitBreakerState::half_open) return true;
    return now_ns >= open_until_ns;
}

} // namespace kivo::p3::execution
