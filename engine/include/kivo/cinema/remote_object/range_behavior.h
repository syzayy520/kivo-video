#pragma once

namespace kivo::cinema::remote_object {

enum class RangeBehavior {
  Unknown,
  Verified,
  Unstable,
  Unsupported,
  RequiresCompatibilityMode,
};

}  // namespace kivo::cinema::remote_object
