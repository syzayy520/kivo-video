#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    // Default evidence is contract-pass-only
    SourceEvidenceSnapshot evidence;
    CHECK_TRUE(evidence.redaction_verified);
    CHECK_TRUE(evidence.schema_validated);
    CHECK_TRUE(evidence.is_contract_pass_only());
    CHECK_TRUE(!evidence.has_runtime_pass());
    
    // Items with contract_pass only: still contract-only
    evidence.items.push_back(SourceEvidenceItem{
        SourceEvidencePassKind::contract_pass,
        SourceEvidenceKind::contract_declared,
        "contract skeleton defined", std::nullopt, std::nullopt, 1
    });
    evidence.items.push_back(SourceEvidenceItem{
        SourceEvidencePassKind::contract_pass,
        SourceEvidenceKind::cache_observed,
        "cache hint observed", std::nullopt, std::nullopt, 2
    });
    CHECK_TRUE(evidence.is_contract_pass_only());
    CHECK_TRUE(!evidence.has_runtime_pass());
    
    // Sequence numbers increase
    CHECK_EQ(evidence.items[0].sequence_number, 1ULL);
    CHECK_EQ(evidence.items[1].sequence_number, 2ULL);
    
    // No runtime_pass items exist in this skeleton
    for (auto& item : evidence.items) {
        CHECK_NEQ(item.pass_kind, SourceEvidencePassKind::runtime_pass);
    }
    return 0;
}
