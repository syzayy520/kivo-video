#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"

namespace kivo::playback::control_plane::revision {

/// CallbackToken provides nonce only, no identity semantics.
/// nonce is monotonically increasing within same SessionId.
/// token is not recycled or reused.
/// token does not carry generation/read_epoch.
/// token cannot independently determine identity.
struct CallbackToken
{
    U64 nonce{0};
    constexpr CallbackToken() noexcept = default;
    constexpr explicit CallbackToken(U64 n) noexcept : nonce(n) {}
};

} // namespace kivo::playback::control_plane::revision
