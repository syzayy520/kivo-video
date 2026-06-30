#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace kivo::video::playback_graph::local_media {

class LocalMediaSourceRegistry {
public:
    static LocalMediaSourceRegistry& instance() noexcept;

    [[nodiscard]] std::uint64_t register_path(std::string path);
    [[nodiscard]] std::optional<std::string> resolve(std::uint64_t source_id) const noexcept;
    void unregister(std::uint64_t source_id) noexcept;

private:
    LocalMediaSourceRegistry() = default;

    std::uint64_t next_id_{0x504D0001};
    std::unordered_map<std::uint64_t, std::string> paths_{};
};

}  // namespace kivo::video::playback_graph::local_media