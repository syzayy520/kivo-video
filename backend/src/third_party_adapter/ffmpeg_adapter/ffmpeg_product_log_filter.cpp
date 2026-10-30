#include "ffmpeg_product_log_filter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <libavutil/log.h>
}
#endif

#include <array>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace kivo::cinema_engine::ffmpeg_adapter {
namespace {

#ifdef KIVO_ENABLE_FFMPEG
[[nodiscard]] bool contains(const std::string_view text,
                            const std::string_view needle) noexcept {
    return text.find(needle) != std::string_view::npos;
}

[[nodiscard]] std::string format_log_message(const char* const format,
                                             va_list arguments) {
    if (format == nullptr) {
        return {};
    }

    std::array<char, 1024> stack_buffer{};
    va_list stack_arguments;
    va_copy(stack_arguments, arguments);
    const int stack_size =
        std::vsnprintf(stack_buffer.data(), stack_buffer.size(), format, stack_arguments);
    va_end(stack_arguments);

    if (stack_size < 0) {
        return {};
    }
    if (static_cast<std::size_t>(stack_size) < stack_buffer.size()) {
        return std::string(stack_buffer.data(), static_cast<std::size_t>(stack_size));
    }

    std::vector<char> heap_buffer(static_cast<std::size_t>(stack_size) + 1U);
    va_list heap_arguments;
    va_copy(heap_arguments, arguments);
    std::vsnprintf(heap_buffer.data(), heap_buffer.size(), format, heap_arguments);
    va_end(heap_arguments);
    return std::string(heap_buffer.data(), static_cast<std::size_t>(stack_size));
}

[[nodiscard]] bool is_product_probe_noise(const int level,
                                          const std::string_view message) noexcept {
    if (level != AV_LOG_WARNING) {
        return false;
    }

    if (contains(message, "not enough frames to estimate rate") &&
        contains(message, "consider increasing probesize")) {
        return true;
    }

    if (contains(message, "Could not find codec parameters for stream") &&
        contains(message, "Subtitle: hdmv_pgs_subtitle") &&
        contains(message, "unspecified size")) {
        return true;
    }

    return contains(message, "Consider increasing the value for the 'analyzeduration'") &&
           contains(message, "'probesize'");
}

void product_log_callback(void* const ptr,
                          const int level,
                          const char* const format,
                          va_list arguments) {
    const std::string message = format_log_message(format, arguments);
    if (is_product_probe_noise(level, message)) {
        return;
    }

    av_log_default_callback(ptr, level, format, arguments);
}
#endif

}  // namespace

void install_product_log_filter() noexcept {
#ifdef KIVO_ENABLE_FFMPEG
    static std::once_flag install_once;
    std::call_once(install_once, [] {
        av_log_set_callback(product_log_callback);
    });
#endif
}

}  // namespace kivo::cinema_engine::ffmpeg_adapter
