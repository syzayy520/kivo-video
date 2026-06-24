# Governance CI Gates

V1.1 needs a small but strict CI gate set.

## Frozen Gates

- contract_registry_gate
- schema_version_gate
- state_owner_gate
- forbidden_include_gate
- adapter_boundary_gate
- event_ordering_gate
- feature_gate_trace_gate
- diagnostic_redaction_gate
- remote_object_identity_gate
- media_badge_claim_gate
- hdr_claim_gate
- audio_claim_gate
- replay_gate
- invariant_gate
- fuzz_gate_minimal

## Current Local Gate

`backend/tools/governance/verify_p1_gates.py` checks the current P0/P1 skeleton without requiring a C++ compiler.

