#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"
#include "kivo/video/source_core/contracts/uri/redacted_source_uri.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace kivo::video::source_core {

enum class SourceEvidencePassKind : std::uint8_t {
    contract_pass = 0,
    runtime_pass = 1,
    runtime_failed = 2
};

enum class SourceEvidenceKind : std::uint8_t {
    contract_declared = 0,
    uri_redacted = 1,
    identity_constructed = 2,
    capability_declared = 3,
    range_proof = 4,
    provider_runtime_observed = 5,
    cache_observed = 6,
    session_closed = 7,
    source_changed_observed = 8,
    unknown = 9
};

enum class SourceEvidenceOperation : std::uint8_t {
    open = 0,
    read = 1,
    positioned_read = 2,
    seek = 3,
    close = 4,
    direct_input = 5,
    source_changed = 6,
    unknown = 7
};

struct SourceEvidenceItem {
    SourceEvidencePassKind pass_kind{SourceEvidencePassKind::contract_pass};
    SourceEvidenceKind evidence_kind{SourceEvidenceKind::unknown};
    SourceEvidenceOperation operation{SourceEvidenceOperation::unknown};
    std::string safe_description;
    std::optional<RedactedSourceUri> redacted_uri;
    std::optional<ProviderKind> provider_kind;
    std::uint64_t sequence_number{0};
};

struct SourceEvidenceSnapshot {
    std::uint32_t contract_version{kSourceCoreContractVersion};
    SourceIdentityId source_id;
    ProviderKind kind{ProviderKind::unknown};
    std::string uri_redacted;
    SourceCapabilitySnapshot capability;
    bool schema_validated{true};
    bool redaction_verified{true};

    std::vector<SourceEvidenceItem> items;

    bool has_runtime_pass() const {
        for (auto& item : items) {
            if (item.pass_kind == SourceEvidencePassKind::runtime_pass) return true;
        }
        return false;
    }

    bool has_runtime_failed() const {
        for (auto& item : items) {
            if (item.pass_kind == SourceEvidencePassKind::runtime_failed) return true;
        }
        return false;
    }

    bool is_contract_pass_only() const {
        return !has_runtime_pass() && !has_runtime_failed();
    }
};

}  // namespace kivo::video::source_core
