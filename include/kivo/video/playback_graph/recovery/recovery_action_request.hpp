#pragma once

namespace kivo::video::playback_graph {

enum class RecoveryActionKind {
    Retry,
    Reopen
};

struct RecoveryActionRequest {
    RecoveryActionKind kind{RecoveryActionKind::Retry};
};

}  // namespace kivo::video::playback_graph