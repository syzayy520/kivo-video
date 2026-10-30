#pragma once

#include <QJsonObject>
#include <QString>

#include "playback_shell_continuous_evidence_record.hpp"

namespace kivo::ui::shell {

[[nodiscard]] QJsonObject build_real_local_json(
    const ContinuousPlaybackEvidenceRecord& record,
    const QString& continuous_classification,
    const QString& errors);

}  // namespace kivo::ui::shell
