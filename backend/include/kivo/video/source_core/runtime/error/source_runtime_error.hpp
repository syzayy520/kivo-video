#pragma once

#include "kivo/video/source_core/contracts/error/source_error.hpp"

namespace kivo::video::source_core {

/// Runtime-layer error alias. Reuses contract-layer SourceError.
/// No exception-based business-control path.
using SourceRuntimeError = SourceError;

}  // namespace kivo::video::source_core
