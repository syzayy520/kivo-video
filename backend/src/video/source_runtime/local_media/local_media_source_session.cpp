#include "video/source_runtime/local_media/local_media_source_session.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

#include "kivo/video/source_core/contracts/uri/source_uri.hpp"
#include "kivo/video/source_core/runtime/open/source_open_request.hpp"
#include "kivo/video/source_core/runtime/read/source_read_request.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_request.hpp"

namespace kivo::video::source_runtime::local_media {
namespace {

[[nodiscard]] std::string source_error_text(const source_core::SourceError& error) {
    if (error.is_ok()) {
        return {};
    }
    return error.message.empty() ? "source error" : error.message;
}

[[nodiscard]] source_core::SeekOrigin map_seek_origin(
    const kivo::cinema_engine::FfmpegIoSeekOrigin origin) noexcept {
    switch (origin) {
    case kivo::cinema_engine::FfmpegIoSeekOrigin::Current:
        return source_core::SeekOrigin::current;
    case kivo::cinema_engine::FfmpegIoSeekOrigin::End:
        return source_core::SeekOrigin::end;
    case kivo::cinema_engine::FfmpegIoSeekOrigin::Begin:
    case kivo::cinema_engine::FfmpegIoSeekOrigin::Size:
        return source_core::SeekOrigin::begin;
    }
    return source_core::SeekOrigin::begin;
}

}  // namespace

LocalMediaSourceSession::~LocalMediaSourceSession() {
    close();
}

bool LocalMediaSourceSession::open(const std::string& path) noexcept {
    close();

    source_core::SourceOpenRequest request{};
    request.uri = source_core::SourceUri::from_raw(path);

    const auto result = providers::local_file::LocalFileProvider::open(request, store_);
    if (!result.is_success()) {
        const auto* error = result.error_if_failure();
        error_ = error == nullptr ? "source open failed" : source_error_text(*error);
        return false;
    }

    const auto* session = result.session_if_success();
    if (session == nullptr) {
        error_ = "source open returned no session";
        return false;
    }
    session_ = *session;
    provider_kind_ = session_.provider_kind;

    const auto direct =
        providers::local_file::LocalFileProvider::make_direct_stream_input(
            session_.session_id, store_);
    if (!direct.is_success()) {
        error_ = direct.error.has_value() ? source_error_text(*direct.error)
                                          : "direct stream input failed";
        close();
        return false;
    }

    content_length_ = direct.input->capability.content_length_bytes;
    open_ = true;
    error_.clear();
    return true;
}

std::string LocalMediaSourceSession::provider_kind_name() const {
    return provider_kind_name(provider_kind_);
}

std::string LocalMediaSourceSession::provider_kind_name(
    const source_core::ProviderKind kind) {
    switch (kind) {
    case source_core::ProviderKind::local_file:
        return "local_file";
    case source_core::ProviderKind::smb:
        return "smb";
    case source_core::ProviderKind::http_file:
        return "http_file";
    case source_core::ProviderKind::https_file:
        return "https_file";
    case source_core::ProviderKind::webdav:
        return "webdav";
    default:
        return "unknown";
    }
}

void LocalMediaSourceSession::close() noexcept {
    if (open_) {
        (void)providers::local_file::LocalFileProvider::close(session_.session_id, store_);
    }
    session_ = {};
    provider_kind_ = source_core::ProviderKind::unknown;
    content_length_.reset();
    open_ = false;
}

kivo::cinema_engine::FfmpegInputCallbacks
LocalMediaSourceSession::ffmpeg_callbacks() noexcept {
    kivo::cinema_engine::FfmpegInputCallbacks callbacks{};
    callbacks.user_data = this;
    callbacks.read = &LocalMediaSourceSession::read_callback;
    callbacks.seek = &LocalMediaSourceSession::seek_callback;
    return callbacks;
}

int LocalMediaSourceSession::read_callback(
    void* const user_data,
    std::uint8_t* const buffer,
    const int buffer_size) {
    auto* self = static_cast<LocalMediaSourceSession*>(user_data);
    if (self == nullptr || buffer == nullptr || buffer_size <= 0 || !self->open_) {
        return -1;
    }

    source_core::SourceReadRequest request{};
    request.session_id = self->session_.session_id;
    request.length = static_cast<std::uint64_t>(buffer_size);

    const auto result = providers::local_file::LocalFileProvider::read(request, self->store_);
    if (!result.is_success()) {
        self->error_ = source_error_text(result.error);
        return -1;
    }

    const auto copy_size = std::min<std::size_t>(
        result.bytes.size(), static_cast<std::size_t>(buffer_size));
    if (copy_size > 0) {
        std::memcpy(buffer, result.bytes.data(), copy_size);
    }
    return static_cast<int>(copy_size);
}

std::int64_t LocalMediaSourceSession::seek_callback(
    void* const user_data,
    const std::int64_t offset,
    const kivo::cinema_engine::FfmpegIoSeekOrigin origin) {
    auto* self = static_cast<LocalMediaSourceSession*>(user_data);
    if (self == nullptr || !self->open_) {
        return -1;
    }
    if (origin == kivo::cinema_engine::FfmpegIoSeekOrigin::Size) {
        return self->content_length_.has_value()
                   ? static_cast<std::int64_t>(*self->content_length_)
                   : -1;
    }

    source_core::SourceSeekRequest request{};
    request.session_id = self->session_.session_id;
    request.offset = offset;
    request.origin = map_seek_origin(origin);

    const auto result = providers::local_file::LocalFileProvider::seek(request, self->store_);
    if (!result.is_success() || !result.new_position.has_value()) {
        self->error_ = source_error_text(result.error);
        return -1;
    }
    if (*result.new_position >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        self->error_ = "source seek overflow";
        return -1;
    }
    return static_cast<std::int64_t>(*result.new_position);
}

}  // namespace kivo::video::source_runtime::local_media
