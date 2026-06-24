# Diagnostic Bundle Manifest

DiagnosticCore owns diagnostic export packaging and redaction.

## Scope

Diagnostic bundles may include contract refs, ledger refs, selected timeline excerpts, inspector snapshots, redacted traces, and replay manifests.

## Redaction

The manifest records:

- diagnostic bundle id
- redaction profile
- redaction state
- included refs
- excluded secret kinds
- consent requirement

## Fail-Closed Rule

Diagnostic redaction failure fails closed. Developer bundles also must not contain token, cookie, credential, or signed URL material.

