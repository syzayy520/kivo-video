// subtitle_attachment_map.hpp — P5C C12: Subtitle attachment map
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "../attachment/mime_conflict_policy.hpp"

namespace kivo::video_plane::ffmpeg_demux {

struct SubtitleAttachmentEntry {
    uint64_t attachment_index{0};
    std::string filename;
    std::string mime_type;
    uint64_t byte_size{0};
    std::string track_id;  // associated subtitle track
    bool is_external{false};
};

class FakeSubtitleAttachmentMap {
public:
    FakeSubtitleAttachmentMap() = default;

    // Register attachment
    void register_attachment(SubtitleAttachmentEntry const& entry) {
        entries_.push_back(entry);
    }

    // Find attachment by filename
    std::optional<SubtitleAttachmentEntry> find_by_filename(std::string const& filename) const {
        for (auto const& e : entries_) {
            if (e.filename == filename) return e;
        }
        return std::nullopt;
    }

    // Find attachments by track id
    std::vector<SubtitleAttachmentEntry> find_by_track(std::string const& track_id) const {
        std::vector<SubtitleAttachmentEntry> result;
        for (auto const& e : entries_) {
            if (e.track_id == track_id) result.push_back(e);
        }
        return result;
    }

    // Get external attachments
    std::vector<SubtitleAttachmentEntry> external_attachments() const {
        std::vector<SubtitleAttachmentEntry> result;
        for (auto const& e : entries_) {
            if (e.is_external) result.push_back(e);
        }
        return result;
    }

    // Validate MIME types
    bool validate_mime_types() const {
        for (auto const& e : entries_) {
            if (e.mime_type.empty()) return false;
        }
        return true;
    }

    size_t attachment_count() const { return entries_.size(); }
    std::vector<SubtitleAttachmentEntry> const& entries() const { return entries_; }

private:
    std::vector<SubtitleAttachmentEntry> entries_;
};

}  // namespace kivo::video_plane::ffmpeg_demux
