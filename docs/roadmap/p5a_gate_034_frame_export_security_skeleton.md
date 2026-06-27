# GATE-034: Frame Export / Debug Dump Security Skeleton

**Task ID**: KIVO-VIDEO-P5-GATE-034-FRAME-EXPORT-SKELETON
**Document Type**: design-skeleton
**Gate Stage**: P5S (security closure)
**Skeleton Stage**: P5A (contract definition only)
**Status**: SKELETON

---

## FrameExportSecurityPolicy (5 rules)

```
FrameExportSecurityPolicy {
    policy_id: string
    debug_dump_disabled_by_default: bool = true
    protected_debug_dump_forbidden: bool = true
    redaction_required: bool = true
    private_path_forbidden: bool = true
    user_consent_required_for_crash_bundle: bool = true
}
```

### Rule Details

1. **DebugDumpDisabledByDefault**: Debug frame dump is OFF by default. Must be explicitly enabled by developer.

2. **ProtectedDebugDumpForbidden**: Protected content (DRM) frames MUST NOT appear in debug dumps. Attempting to dump protected frames produces black placeholder + redaction evidence.

3. **RedactionRequired**: All debug dumps must redact:
   - Private file paths → redacted path
   - Device IDs → hashed
   - Protected content → placeholder

4. **PrivatePathForbidden**: No snapshot/thumbnail/frame export may contain raw private file paths. All paths must be redacted or relative-to-container.

5. **UserConsentRequiredForCrashBundle**: Crash bundles containing frame data require user consent. Auto-upload is disabled by default.

---

## Snapshot/Thumbnail Golden Tests (5 scenarios)

1. **SDR source → SDR snapshot**: Correct colorspace, no tone mapping needed
2. **HDR source → SDR mapped**: Tone map applied, within SDR gamut
3. **HDR source → display-referred**: Display-referred linear-light domain export
4. **P010 source → correct range**: Limited range P010 correctly mapped
5. **Rotation/crop applied**: Geometry pipeline applied to export
6. **Protected content → placeholder**: Black frame + protected content evidence

---

## Forbidden Operations

| # | Operation |
|---|-----------|
| ❌ | Protected debug dump |
| ❌ | Snapshot containing private path |
| ❌ | Protected content in debug dump |
| ❌ | Crash bundle auto-upload without consent |
| ❌ | Frame export without redaction policy applied |

---

**END OF GATE-034 SKELETON**
