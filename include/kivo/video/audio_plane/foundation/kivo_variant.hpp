// kivo_variant.hpp — P6A foundation: kivo::Variant<T...> (compile-only, §3.2)
#pragma once
#include <variant>
namespace kivo {
template <typename... Ts>
using Variant = std::variant<Ts...>;
}  // namespace kivo
