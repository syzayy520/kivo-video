// kivo_expected.hpp — P6A foundation: kivo::Expected<T,E> (compile-only, §3.2)
// Public contract ABI wrapper. NOT std::expected (compiler-unstable).
#pragma once
#include <utility>
#include <variant>
namespace kivo {
template <typename T, typename E>
class Expected {
public:
    Expected(T value) : data_(std::move(value)) {}
    Expected(E error) : data_(std::move(error)) {}  // NOLINT implicit
    bool has_value() const noexcept { return data_.index() == 0; }
    bool is_error() const noexcept { return data_.index() == 1; }
    T& value() { return std::get<0>(data_); }
    const T& value() const { return std::get<0>(data_); }
    E& error() { return std::get<1>(data_); }
    const E& error() const { return std::get<1>(data_); }
private:
    std::variant<T, E> data_;
};
}  // namespace kivo
