#include "video/playback_graph/local_media/local_media_source_registry.hpp"

namespace kivo::video::playback_graph::local_media {

LocalMediaSourceRegistry& LocalMediaSourceRegistry::instance() noexcept {
    static LocalMediaSourceRegistry registry{};
    return registry;
}

std::uint64_t LocalMediaSourceRegistry::register_path(std::string path) {
    const std::uint64_t source_id = next_id_++;
    paths_[source_id] = std::move(path);
    return source_id;
}

std::optional<std::string> LocalMediaSourceRegistry::resolve(
    const std::uint64_t source_id) const noexcept {
    const auto it = paths_.find(source_id);
    if (it == paths_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void LocalMediaSourceRegistry::unregister(const std::uint64_t source_id) noexcept {
    paths_.erase(source_id);
}

}  // namespace kivo::video::playback_graph::local_media