#pragma once

#include "kivo/cinema_engine/parser_security_core/parser_security_policy.hpp"
#include "kivo/cinema_engine/parser_security_core/quarantine_policy.hpp"
#include "kivo/cinema_engine/parser_security_core/fuzz_test_shell.hpp"

namespace kivo::cinema_engine {

// Fake parser security service for testing.
// Provides static builder functions for parser security policies, quarantine policies, and fuzz shells.
class FakeParserSecurityService {
 public:
    // Parser security policy builders
    static ParserSecurityPolicy build_strict_parser_security_policy();
    static ParserSecurityPolicy build_relaxed_parser_security_policy();
    static ParserSecurityPolicy build_timeout_parser_security_policy();
    static ParserSecurityPolicy build_reject_unknown_boxes_policy();
    
    // Quarantine policy builders
    static QuarantinePolicy build_standard_quarantine_policy();
    static QuarantinePolicy build_aggressive_quarantine_policy();
    static QuarantinePolicy build_minimal_quarantine_policy();
    static QuarantinePolicy build_escalation_quarantine_policy();
    
    // Fuzz test shell builders
    static FuzzTestShell build_basic_fuzz_shell();
    static FuzzTestShell build_coverage_guided_fuzz_shell();
    static FuzzTestShell build_quick_fuzz_shell();
    static FuzzTestShell build_reproduction_fuzz_shell();
};

}  // namespace kivo::cinema_engine