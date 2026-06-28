// kivo_optional.hpp — P6A foundation: kivo::Optional<T> (compile-only, §3.2)
#pragma once
#include <optional>
namespace kivo {
template <typename T>
using Optional = std::optional<T>;
}  // namespace kivo
