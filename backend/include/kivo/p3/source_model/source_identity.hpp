#pragma once

#include <cstdint>
#include <string>
#include "source_kinds.hpp"

namespace kivo::p3::source_model {

/// SourceOriginIdentity describes item origin identity.
/// schema_version: p3.source_model.origin_identity.v1
struct SourceOriginIdentity
{
    std::string schema_version{"p3.source_model.origin_identity.v1"};
    std::string origin_id{};
    SourceOriginKind kind{SourceOriginKind::local_file};
    std::string redacted_display_name{};
};

/// ProviderIdentity describes provider identity.
struct ProviderIdentity
{
    std::string schema_version{"p3.source_model.provider_identity.v1"};
    std::string provider_id{};
    ProviderFamily family{ProviderFamily::unsupported};
    std::string redacted_host{};
};

/// SourceTransportIdentity describes transport identity.
struct SourceTransportIdentity
{
    std::string schema_version{"p3.source_model.transport_identity.v1"};
    std::string transport_id{};
    SourceTransportKind kind{SourceTransportKind::unsupported};
};

/// MediaDeliveryIdentity describes delivery identity.
struct MediaDeliveryIdentity
{
    std::string schema_version{"p3.source_model.delivery_identity.v1"};
    std::string delivery_id{};
    MediaDeliveryKind kind{MediaDeliveryKind::unsupported};
};

/// SourceCapability describes source capability.
struct SourceCapability
{
    std::string schema_version{"p3.source_model.capability.v1"};
    bool supports_range_read{false};
    bool supports_seek{false};
    bool supports_positioned_read{false};
    bool supports_cancel{false};
    int64_t content_length{0};
    ContainerFormat container_hint{ContainerFormat::unknown};
};

} // namespace kivo::p3::source_model
