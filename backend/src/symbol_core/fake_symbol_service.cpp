#include "kivo/cinema_engine/symbol_core/fake_symbol_service.hpp"

namespace kivo::cinema_engine {

// Symbol management policy builders
SymbolManagementPolicy FakeSymbolService::build_full_symbol_policy() {
    SymbolManagementPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "full-symbol-001";
    policy.symbol_server_enabled = true;
    policy.symbol_upload_required = true;
    policy.retention_days = 365;
    policy.compression_enabled = true;
    policy.public_symbols_only = false;
    return policy;
}

SymbolManagementPolicy FakeSymbolService::build_public_only_symbol_policy() {
    SymbolManagementPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "public-only-symbol-001";
    policy.symbol_server_enabled = true;
    policy.symbol_upload_required = true;
    policy.retention_days = 180;
    policy.compression_enabled = true;
    policy.public_symbols_only = true;
    return policy;
}

SymbolManagementPolicy FakeSymbolService::build_minimal_symbol_policy() {
    SymbolManagementPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "minimal-symbol-001";
    policy.symbol_server_enabled = false;
    policy.symbol_upload_required = false;
    policy.retention_days = 30;
    policy.compression_enabled = false;
    policy.public_symbols_only = true;
    return policy;
}

}  // namespace kivo::cinema_engine