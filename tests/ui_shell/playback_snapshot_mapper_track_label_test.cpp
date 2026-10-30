#include <QVariantList>
#include <QVariantMap>

#include <cstdint>

#include "playback_shell_snapshot_mapper.hpp"

namespace {

QVariantMap first_item(const QVariantMap& snapshot, const char* const key) {
    const QVariantList items = snapshot.value(QString::fromLatin1(key)).toList();
    return items.isEmpty() ? QVariantMap{} : items.front().toMap();
}

QVariantMap second_item(const QVariantMap& snapshot, const char* const key) {
    const QVariantList items = snapshot.value(QString::fromLatin1(key)).toList();
    return items.size() < 2 ? QVariantMap{} : items.at(1).toMap();
}

bool has_label_and_id(const QVariantMap& item,
                      const QString& label,
                      const std::uint64_t track_id) {
    return item.value(QStringLiteral("label")).toString() == label &&
           item.value(QStringLiteral("name")).toString() == label &&
           item.value(QStringLiteral("trackId")).toULongLong() == track_id &&
           item.value(QStringLiteral("id")).toULongLong() == track_id;
}

}  // namespace

int main() {
    kivo::video::player_runtime_adapter::AdapterSnapshot adapter_snapshot;
    adapter_snapshot.audio_track_ids = {2, 3};
    adapter_snapshot.subtitle_track_ids = {4, 8};
    adapter_snapshot.video_track_ids = {0, 9};

    const QVariantMap snapshot = kivo::ui::shell::map_adapter_snapshot(adapter_snapshot);

    if (!has_label_and_id(first_item(snapshot, "audioTrackOptions"), QStringLiteral("Audio 1"), 2)) {
        return 1;
    }
    if (!has_label_and_id(second_item(snapshot, "audioTrackOptions"), QStringLiteral("Audio 2"), 3)) {
        return 2;
    }
    if (!has_label_and_id(first_item(snapshot, "subtitleOptions"),
                          QStringLiteral("Subtitle 1"),
                          4)) {
        return 3;
    }
    if (!has_label_and_id(second_item(snapshot, "subtitleOptions"),
                          QStringLiteral("Subtitle 2"),
                          8)) {
        return 4;
    }
    if (!has_label_and_id(first_item(snapshot, "videoTrackOptions"), QStringLiteral("Video 1"), 0)) {
        return 5;
    }
    if (!has_label_and_id(second_item(snapshot, "videoTrackOptions"), QStringLiteral("Video 2"), 9)) {
        return 6;
    }

    return 0;
}
