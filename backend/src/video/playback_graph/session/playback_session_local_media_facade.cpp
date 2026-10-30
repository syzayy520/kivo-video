#include "kivo/video/playback_graph/playback_session.hpp"

#include <string>

#include "video/playback_graph/local_media/local_media_source_registry.hpp"
#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph {
namespace {

using Runtime = runtime::PlaybackSessionRuntime;

[[nodiscard]] Runtime* as_runtime(void* impl) noexcept {
    return static_cast<Runtime*>(impl);
}

[[nodiscard]] CommandToken allocation_failure_token() noexcept {
    CommandToken token{};
    token.immediate_error = PlaybackGraphError::OutOfMemory;
    return token;
}

}  // namespace

CommandToken PlaybackSession::open_local_media_file(const std::string_view path) noexcept {
    if (path.empty()) {
        CommandToken token{};
        token.immediate_error = PlaybackGraphError::SourceOpenFailed;
        return token;
    }
    auto* runtime = as_runtime(impl_);
    if (runtime == nullptr) {
        return allocation_failure_token();
    }
    OpenRequest request{};
    request.source_id =
        local_media::LocalMediaSourceRegistry::instance().register_path(std::string(path));
    return runtime->open(request);
}

}  // namespace kivo::video::playback_graph
