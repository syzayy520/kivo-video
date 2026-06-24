#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

enum class RangeBehavior {
  Unknown,
  Verified,
  Unstable,
  Unsupported,
  LyingRange,
  RequiresCompatMode
};

enum class DirectLinkLifetime {
  Stable,
  TtlKnown,
  TtlUnknown,
  Expired,
  RefreshRequired
};

enum class RefreshStrategy {
  None,
  RefreshSignedUrl,
  RefreshServerDirectLink,
  RefreshProviderToken,
  ReprobeObject,
  UserReauthRequired
};

struct RemoteObjectIdentity {
  ContractMetadata metadata;
  std::string remote_object_identity_id;
  std::string source_id;
  std::string source_kind;
  std::string provider_kind;
  std::optional<std::string> provider_object_id;
  std::optional<std::string> server_item_id;
  std::optional<std::string> canonical_path;
  std::optional<std::uint64_t> content_length;
  std::optional<std::string> etag;
  std::optional<std::string> last_modified;
  std::optional<std::string> file_hash;
  double stable_identity_confidence{0.0};
  RangeBehavior range_behavior{RangeBehavior::Unknown};
  DirectLinkLifetime direct_link_lifetime{DirectLinkLifetime::TtlUnknown};
  RefreshStrategy refresh_strategy{RefreshStrategy::None};
  std::optional<std::string> rate_limit_policy;
  std::optional<std::string> throttling_risk;
  std::string cache_identity_key;
};

}  // namespace kivo::cinema_engine
