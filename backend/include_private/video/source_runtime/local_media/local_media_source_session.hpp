#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "kivo/video/providers/local_file/local_file_session_store.hpp"
#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "kivo/video/source_core/runtime/session/source_session.hpp"

namespace kivo::video::source_runtime::local_media {

class LocalMediaSourceSession {
public:
    LocalMediaSourceSession() = default;
    ~LocalMediaSourceSession();

    LocalMediaSourceSession(const LocalMediaSourceSession&) = delete;
    LocalMediaSourceSession& operator=(const LocalMediaSourceSession&) = delete;

    [[nodiscard]] bool open(const std::string& path) noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] const std::string& error() const noexcept { return error_; }
    [[nodiscard]] source_core::ProviderKind provider_kind() const noexcept {
        return provider_kind_;
    }
    [[nodiscard]] std::string provider_kind_name() const;
    [[nodiscard]] static std::string provider_kind_name(source_core::ProviderKind kind);
    [[nodiscard]] std::optional<std::uint64_t> content_length() const noexcept {
        return content_length_;
    }

    [[nodiscard]] kivo::cinema_engine::FfmpegInputCallbacks ffmpeg_callbacks() noexcept;

private:
    static int read_callback(void* user_data, std::uint8_t* buffer, int buffer_size);
    static std::int64_t seek_callback(
        void* user_data,
        std::int64_t offset,
        kivo::cinema_engine::FfmpegIoSeekOrigin origin);

    providers::local_file::LocalFileSessionStore store_{};
    source_core::SourceSession session_{};
    source_core::ProviderKind provider_kind_{source_core::ProviderKind::unknown};
    std::optional<std::uint64_t> content_length_{};
    std::string error_{};
    bool open_{false};
};

}  // namespace kivo::video::source_runtime::local_media
