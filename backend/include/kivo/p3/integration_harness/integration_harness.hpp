#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace kivo::p3::integration_harness {

/// FakeServerConfig configures an in-process fake server for deterministic testing.
/// No real network. No real accounts. Uses TCP loopback with random ports.
/// schema_version: p3.harness.fake_server.v1
struct FakeServerConfig
{
    std::string schema_version{"p3.harness.fake_server.v1"};
    std::string server_id{};
    int32_t port{0}; // 0 = random port
    bool use_tcp_loopback{true};
    int64_t controlled_delay_ns{0};
    bool simulate_timeout{false};
    bool simulate_cancel{false};
    std::string redacted_bind_address{"127.0.0.1"};
};

/// FakeServerState defines fake server runtime state.
struct FakeServerState
{
    std::string schema_version{"p3.harness.server_state.v1"};
    bool is_running{false};
    int32_t actual_port{0};
    int32_t request_count{0};
    bool cancel_triggered{false};

    bool is_ready() const noexcept { return is_running && actual_port > 0; }
};

/// ControlledClock provides test-controlled time (no sleep race).
struct ControlledClock
{
    std::string schema_version{"p3.harness.controlled_clock.v1"};
    int64_t current_ns{0};

    int64_t now_monotonic() const noexcept { return current_ns; }
    void advance(int64_t delta_ns) noexcept { current_ns += delta_ns; }
    void set(int64_t ns) noexcept { current_ns = ns; }
};

/// CancellationBarrier provides barrier-based cancellation (no sleep race).
struct CancellationBarrier
{
    std::string schema_version{"p3.harness.cancel_barrier.v1"};
    bool cancel_signalled{false};
    int32_t waiter_count{0};

    void signal_cancel() noexcept { cancel_signalled = true; }
    bool is_cancelled() const noexcept { return cancel_signalled; }
};

/// IntegrationHarnessConfig defines overall harness configuration.
struct IntegrationHarnessConfig
{
    std::string schema_version{"p3.harness.config.v1"};
    ControlledClock clock{};
    CancellationBarrier barrier{};
    bool deterministic{true};
    bool no_sleep_race{true};
    bool no_real_account{true};
};

} // namespace kivo::p3::integration_harness
