#pragma once

#include <iostream>

namespace kivo::video::source_core::test {

inline int fail(const char* expr, const char* file, int line) {
    std::cerr << "FAILED: " << expr << " at " << file << ":" << line << "\n";
    return 1;
}

#define CHECK_TRUE(expr) \
    do { if (!(expr)) return ::kivo::video::source_core::test::fail(#expr, __FILE__, __LINE__); } while (false)

#define CHECK_EQ(a, b) \
    do { if (!((a) == (b))) return ::kivo::video::source_core::test::fail(#a " == " #b, __FILE__, __LINE__); } while (false)

#define CHECK_NEQ(a, b) \
    do { if ((a) == (b)) return ::kivo::video::source_core::test::fail(#a " != " #b, __FILE__, __LINE__); } while (false)

#define CHECK_HAS_VALUE(opt) \
    do { if (!(opt).has_value()) return ::kivo::video::source_core::test::fail(#opt ".has_value()", __FILE__, __LINE__); } while (false)

}  // namespace kivo::video::source_core::test
