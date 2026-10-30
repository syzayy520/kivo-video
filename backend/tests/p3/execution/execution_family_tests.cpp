#include "kivo/p3/execution/execution_context.hpp"
#include "kivo/p3/execution/monotonic_deadline.hpp"
#include "kivo/p3/execution/budget_allocator.hpp"
#include "kivo/p3/execution/retry_budget.hpp"
#include "kivo/p3/execution/cancellation_signal.hpp"
#include "kivo/p3/execution/async_task_ownership.hpp"
#include "kivo/p3/execution/event_stream_backpressure.hpp"
#include "kivo/p3/execution/runtime_revision_storm_control.hpp"
#include "kivo/p3/execution/feature_flag_closure_policy.hpp"

#include <cassert>
#include <iostream>

static void test_execution_context_default()
{
    kivo::p3::execution::ExecutionContext ctx;
    assert(!ctx.operation_id.empty() == false);
    assert(ctx.schema_version == "p3.execution.context.v1");
    assert(!ctx.is_valid());
    assert(!ctx.has_deadline());
    assert(!ctx.has_budget());
    std::cout << "PASS: execution_context_default\n";
}

static void test_execution_context_valid()
{
    kivo::p3::execution::ExecutionContext ctx;
    ctx.operation_id = "op-001";
    ctx.deadline_monotonic_ns = 1000;
    assert(ctx.is_valid());
    assert(ctx.has_deadline());
    std::cout << "PASS: execution_context_valid\n";
}

static void test_monotonic_deadline()
{
    kivo::p3::execution::MonotonicDeadline d;
    d.deadline_monotonic_ns = 5000;
    d.now_monotonic_ns = 3000;
    assert(!d.is_expired());
    assert(d.remaining_ns() == 2000);
    assert(d.has_remaining());
    d.now_monotonic_ns = 5000;
    assert(d.is_expired());
    assert(!d.has_remaining());
    std::cout << "PASS: monotonic_deadline\n";
}

static void test_budget_allocator()
{
    kivo::p3::execution::BudgetAllocator b;
    b.total_budget_ns = 10000;
    b.reserved_cleanup_ns = 1000;
    b.reserved_process_kill_ns = 500;
    b.consumed_ns = 3000;
    assert(b.available_ns() == 7000);
    assert(b.has_reserved_cleanup());
    assert(b.has_reserved_process_kill());
    assert(b.can_consume(5000));
    assert(!b.can_consume(8000));
    std::cout << "PASS: budget_allocator\n";
}

static void test_retry_budget()
{
    kivo::p3::execution::RetryBudget r;
    r.max_retries = 3;
    r.is_idempotent = true;
    assert(r.can_retry());
    r.consume();
    r.consume();
    r.consume();
    assert(!r.can_retry());
    r.is_idempotent = false;
    assert(!r.can_retry());
    std::cout << "PASS: retry_budget\n";
}

static void test_circuit_breaker()
{
    kivo::p3::execution::CircuitBreaker cb;
    cb.state = kivo::p3::execution::CircuitBreakerState::closed;
    assert(cb.allows_new_attempt());
    cb.state = kivo::p3::execution::CircuitBreakerState::open;
    cb.open_until_ns = 5000;
    cb.now_ns = 3000;
    assert(!cb.allows_new_attempt());
    cb.now_ns = 5000;
    assert(cb.allows_new_attempt());
    std::cout << "PASS: circuit_breaker\n";
}

static void test_cancellation_signal()
{
    kivo::p3::execution::CancellationSignal c;
    assert(!c.is_cancelled());
    c.request(1000);
    assert(c.is_requested());
    assert(c.is_cancelled());
    c.complete();
    assert(c.is_completed());
    std::cout << "PASS: cancellation_signal\n";
}

static void test_async_task_ownership()
{
    kivo::p3::execution::AsyncTaskOwnership t;
    assert(!t.is_valid());
    t.task_id = "task-001";
    t.owner_id = "owner-001";
    t.is_owned = true;
    assert(t.is_valid());
    assert(t.has_owner());
    std::cout << "PASS: async_task_ownership\n";
}

static void test_event_stream_backpressure()
{
    kivo::p3::execution::EventStreamBackpressure e;
    e.max_buffered = 100;
    e.current_buffered = 50;
    assert(!e.has_backpressure());
    e.current_buffered = 100;
    assert(e.has_backpressure());
    assert(e.critical_must_not_drop());
    std::cout << "PASS: event_stream_backpressure\n";
}

static void test_runtime_revision_storm_control()
{
    kivo::p3::execution::RuntimeRevisionStormControl r;
    r.max_revision_per_minute = 5;
    r.min_interval_ns = 1'000'000'000;
    r.last_revision_ns = 0;
    assert(r.can_revise(2'000'000'000));
    assert(!r.is_throttled(2'000'000'000));
    r.revisions_this_minute = 5;
    assert(!r.can_revise(2'000'000'000));
    assert(r.is_throttled(2'000'000'000));
    std::cout << "PASS: runtime_revision_storm_control\n";
}

static void test_feature_flag_closure_policy()
{
    kivo::p3::execution::FeatureFlagClosurePolicy f;
    f.is_experimental = false;
    assert(f.is_valid_for_closure());
    f.is_experimental = true;
    f.default_on = true;
    assert(!f.is_valid_for_closure());
    f.default_on = false;
    f.is_closure_path = true;
    f.has_separate_evidence = false;
    assert(!f.is_valid_for_closure());
    f.has_separate_evidence = true;
    assert(f.is_valid_for_closure());
    std::cout << "PASS: feature_flag_closure_policy\n";
}

int main()
{
    test_execution_context_default();
    test_execution_context_valid();
    test_monotonic_deadline();
    test_budget_allocator();
    test_retry_budget();
    test_circuit_breaker();
    test_cancellation_signal();
    test_async_task_ownership();
    test_event_stream_backpressure();
    test_runtime_revision_storm_control();
    test_feature_flag_closure_policy();
    std::cout << "ALL PASS: execution_family_tests (11 tests)\n";
    return 0;
}
