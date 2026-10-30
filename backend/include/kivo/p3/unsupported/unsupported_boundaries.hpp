#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::unsupported {

/// CloudDriveFamily: cloud drive contract without SDK sprawl.
enum class CloudDriveFamily
{
    google_drive,
    one_drive,
    unsupported
};

/// CloudDriveContractOnlySignal: contract-only cloud drive boundary.
struct CloudDriveContractOnlySignal
{
    std::string schema_version{"p3.unsupported.cloud_drive.v1"};
    CloudDriveFamily family{CloudDriveFamily::unsupported};
    std::string redacted_account_identity{};
    std::string file_identity{};
    std::string auth_mode{};
    std::string quota_evidence{};
    std::string rate_limit_evidence{};
    bool is_unsupported{true};
};

/// BoundaryUnsupportedReason: SMB/UPnP/DLNA boundary.
struct SmbUpnpDlnaBoundary
{
    std::string schema_version{"p3.unsupported.smb_upnp_dlna.v1"};
    std::string share_identity{};
    std::string item_identity{};
    std::string server_identity{};
    bool boundary_defined_not_implemented{true};
};

/// UnsupportedProtocolCandidate: FTP/RTSP/RTMP explicit unsupported signals.
struct UnsupportedProtocolCandidate
{
    std::string schema_version{"p3.unsupported.protocol.v1"};
    std::string protocol_name{};
    std::string reason{"protocol_out_of_p3_scope"};
    std::string deferral_stage{"P4_or_later"};
    bool is_structured_reason{true};
};

} // namespace kivo::p3::unsupported
