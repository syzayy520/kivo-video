#include "kivo/cinema_engine/parser_security_core/fake_parser_security_service.hpp"

namespace kivo::cinema_engine {

// Parser security policy builders
ParserSecurityPolicy FakeParserSecurityService::build_strict_parser_security_policy() {
    ParserSecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "strict-parser-security-001";
    policy.max_input_size_bytes = 524288000;  // 500MB
    policy.max_depth = 16;
    policy.reject_unknown_boxes = true;
    policy.timeout_ms = 5000;
    policy.require_header_signature = true;
    return policy;
}

ParserSecurityPolicy FakeParserSecurityService::build_relaxed_parser_security_policy() {
    ParserSecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "relaxed-parser-security-001";
    policy.max_input_size_bytes = 2147483648;  // 2GB
    policy.max_depth = 64;
    policy.reject_unknown_boxes = false;
    policy.timeout_ms = 30000;
    policy.require_header_signature = false;
    return policy;
}

ParserSecurityPolicy FakeParserSecurityService::build_timeout_parser_security_policy() {
    ParserSecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "timeout-parser-security-001";
    policy.max_input_size_bytes = 1073741824;  // 1GB
    policy.max_depth = 32;
    policy.reject_unknown_boxes = true;
    policy.timeout_ms = 1000;  // aggressive timeout
    policy.require_header_signature = true;
    return policy;
}

ParserSecurityPolicy FakeParserSecurityService::build_reject_unknown_boxes_policy() {
    ParserSecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "reject-unknown-boxes-001";
    policy.max_input_size_bytes = 1073741824;  // 1GB
    policy.max_depth = 32;
    policy.reject_unknown_boxes = true;
    policy.timeout_ms = 10000;
    policy.require_header_signature = false;
    return policy;
}

// Quarantine policy builders
QuarantinePolicy FakeParserSecurityService::build_standard_quarantine_policy() {
    QuarantinePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "standard-quarantine-001";
    policy.quarantine_duration_seconds = 86400;  // 24h
    policy.auto_delete_quarantined = false;
    policy.max_quarantine_size_mb = 1024;  // 1GB
    policy.notify_on_quarantine = true;
    policy.escalation_threshold = 3;
    return policy;
}

QuarantinePolicy FakeParserSecurityService::build_aggressive_quarantine_policy() {
    QuarantinePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "aggressive-quarantine-001";
    policy.quarantine_duration_seconds = 604800;  // 7 days
    policy.auto_delete_quarantined = true;
    policy.max_quarantine_size_mb = 2048;  // 2GB
    policy.notify_on_quarantine = true;
    policy.escalation_threshold = 1;
    return policy;
}

QuarantinePolicy FakeParserSecurityService::build_minimal_quarantine_policy() {
    QuarantinePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "minimal-quarantine-001";
    policy.quarantine_duration_seconds = 3600;  // 1 hour
    policy.auto_delete_quarantined = true;
    policy.max_quarantine_size_mb = 256;  // 256MB
    policy.notify_on_quarantine = false;
    policy.escalation_threshold = 5;
    return policy;
}

QuarantinePolicy FakeParserSecurityService::build_escalation_quarantine_policy() {
    QuarantinePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "escalation-quarantine-001";
    policy.quarantine_duration_seconds = 259200;  // 3 days
    policy.auto_delete_quarantined = false;
    policy.max_quarantine_size_mb = 4096;  // 4GB
    policy.notify_on_quarantine = true;
    policy.escalation_threshold = 2;
    return policy;
}

// Fuzz test shell builders
FuzzTestShell FakeParserSecurityService::build_basic_fuzz_shell() {
    FuzzTestShell shell;
    shell.metadata.schema_version = "1.0.0";
    shell.shell_id = "basic-fuzz-shell-001";
    shell.max_iterations = 10000;
    shell.crash_reproduction = true;
    shell.coverage_guided = false;
    shell.seed_corpus_path = "/corpus/basic";
    shell.mutation_strategy = "bitflip";
    return shell;
}

FuzzTestShell FakeParserSecurityService::build_coverage_guided_fuzz_shell() {
    FuzzTestShell shell;
    shell.metadata.schema_version = "1.0.0";
    shell.shell_id = "coverage-guided-fuzz-shell-001";
    shell.max_iterations = 100000;
    shell.crash_reproduction = true;
    shell.coverage_guided = true;
    shell.seed_corpus_path = "/corpus/coverage";
    shell.mutation_strategy = "insert";
    return shell;
}

FuzzTestShell FakeParserSecurityService::build_quick_fuzz_shell() {
    FuzzTestShell shell;
    shell.metadata.schema_version = "1.0.0";
    shell.shell_id = "quick-fuzz-shell-001";
    shell.max_iterations = 1000;
    shell.crash_reproduction = false;
    shell.coverage_guided = false;
    shell.seed_corpus_path = "/corpus/quick";
    shell.mutation_strategy = "delete";
    return shell;
}

FuzzTestShell FakeParserSecurityService::build_reproduction_fuzz_shell() {
    FuzzTestShell shell;
    shell.metadata.schema_version = "1.0.0";
    shell.shell_id = "reproduction-fuzz-shell-001";
    shell.max_iterations = 50000;
    shell.crash_reproduction = true;
    shell.coverage_guided = true;
    shell.seed_corpus_path = "/corpus/repro";
    shell.mutation_strategy = "splice";
    return shell;
}

}  // namespace kivo::cinema_engine