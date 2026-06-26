#pragma once

// Kivo Test Exit Utilities
// Provides non-aborting test exit helpers for runtime gate tests.
// No assert(), no abort(), no popups.

#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <crtdbg.h>
#include <cstdlib>
#endif

namespace kivo::test {

// Exit codes for runtime gate tests
enum class TestExitCode {
    RUNTIME_PASS = 0,
    BLOCKED_ENV = 1,
    FAIL = 2,
    BLOCKED_SAMPLE_TOO_SHORT = 3,
    BLOCKED_SAMPLE_INVALID = 4
};

// Install guard to prevent MSVC Debug CRT popups
inline void install_no_crt_popup_guard() {
#ifdef _WIN32
    SetErrorMode(
        SEM_FAILCRITICALERRORS |
        SEM_NOGPFAULTERRORBOX |
        SEM_NOOPENFILEERRORBOX
    );

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

// Return helpers - these print classification and return exit code
inline int return_pass(const std::string& message) {
    std::cout << "RUNTIME_PASS: " << message << "\n";
    std::cout << "Classification: RUNTIME_PASS\n";
    return static_cast<int>(TestExitCode::RUNTIME_PASS);
}

inline int return_blocked_env(const std::string& message) {
    std::cout << "BLOCKED_ENV: " << message << "\n";
    std::cout << "Classification: BLOCKED_ENV\n";
    return static_cast<int>(TestExitCode::BLOCKED_ENV);
}

inline int return_fail(const std::string& message) {
    std::cout << "FAIL: " << message << "\n";
    std::cout << "Classification: FAIL\n";
    return static_cast<int>(TestExitCode::FAIL);
}

inline int return_blocked_sample_too_short(const std::string& message) {
    std::cout << "BLOCKED_SAMPLE_TOO_SHORT: " << message << "\n";
    std::cout << "Classification: BLOCKED_SAMPLE_TOO_SHORT\n";
    return static_cast<int>(TestExitCode::BLOCKED_SAMPLE_TOO_SHORT);
}

inline int return_blocked_sample_invalid(const std::string& message) {
    std::cout << "BLOCKED_SAMPLE_INVALID: " << message << "\n";
    std::cout << "Classification: BLOCKED_SAMPLE_INVALID\n";
    return static_cast<int>(TestExitCode::BLOCKED_SAMPLE_INVALID);
}

// Check macro that returns instead of asserting
#define KIVO_TEST_REQUIRE_OR_RETURN(cond, exit_code, message) \
    do { \
        if (!(cond)) { \
            std::cout << "CHECK FAILED: " << #cond << "\n"; \
            std::cout << message << "\n"; \
            return kivo::test::return_fail(message); \
        } \
    } while (0)

#define KIVO_TEST_REQUIRE_OR_RETURN_BLOCKED(cond, exit_code, message) \
    do { \
        if (!(cond)) { \
            std::cout << "CHECK FAILED: " << #cond << "\n"; \
            std::cout << message << "\n"; \
            return kivo::test::return_blocked_env(message); \
        } \
    } while (0)

} // namespace kivo::test
