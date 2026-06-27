// track_discovery.hpp — P5C C11: Track discovery
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::ffmpeg_demux {

enum class TrackKind {
    Video,
    Audio,
    Subtitle,
    Attachment,
    Data,
    Unknown
};

struct DiscoveredTrack {
    uint64_t track_index{0};
    TrackKind kind{TrackKind::Unknown};
    std::string codec_name;
    std::string language;
    bool is_default{false};
    bool is_forced{false};
    uint64_t duration_ms{0};
};

class FakeTrackDiscovery {
public:
    FakeTrackDiscovery() = default;

    // Discover tracks from fake stream info
    void discover(std::vector<DiscoveredTrack> const& tracks) {
        tracks_ = tracks;
        discovered_ = true;
    }

    // Get all tracks
    std::vector<DiscoveredTrack> const& tracks() const { return tracks_; }

    // Get tracks by kind
    std::vector<DiscoveredTrack> tracks_by_kind(TrackKind kind) const {
        std::vector<DiscoveredTrack> result;
        for (auto const& t : tracks_) {
            if (t.kind == kind) result.push_back(t);
        }
        return result;
    }

    // Get default track for kind
    std::vector<DiscoveredTrack>::const_iterator find_default(TrackKind kind) const {
        for (auto it = tracks_.begin(); it != tracks_.end(); ++it) {
            if (it->kind == kind && it->is_default) return it;
        }
        return tracks_.end();
    }

    bool is_discovered() const { return discovered_; }
    size_t track_count() const { return tracks_.size(); }

private:
    std::vector<DiscoveredTrack> tracks_;
    bool discovered_{false};
};

}  // namespace kivo::video_plane::ffmpeg_demux
