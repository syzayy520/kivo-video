#pragma once

#include "kivo/video/source_core/contracts/uri/source_uri.hpp"
#include "kivo/video/source_core/contracts/auth/source_auth.hpp"
#include "kivo/video/source_core/runtime/timeout/source_timeout.hpp"
#include "kivo/video/source_core/runtime/cancel/source_cancel_token.hpp"

namespace kivo::video::source_core {

/// Open request bound to a raw SourceUri (the only place raw URI exists).
struct SourceOpenRequest {
    SourceUri uri;
    SourceAuthInfo auth;
    SourceTimeout timeout;
    SourceCancelToken cancel_token;
};

}  // namespace kivo::video::source_core
