#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Default evidence
    SourceEvidenceSnapshot evidence;
    assert(evidence.contract_version == kSourceCoreContractVersion);
    assert(evidence.redaction_verified);
    assert(evidence.schema_validated);
    
    // Set fields
    evidence.source_id = SourceIdentityId{1};
    evidence.kind = ProviderKind::emby;
    evidence.uri_redacted = "<REDACTED>";
    
    assert(evidence.source_id.value == 1);
    assert(evidence.kind == ProviderKind::emby);
    assert(!evidence.uri_redacted.empty());
    
    return 0;
}
