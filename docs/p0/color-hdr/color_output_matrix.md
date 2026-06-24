# Color Output Matrix

ColorOutputMatrix is the authority for HDR and DV-aware user-visible claims.

## Scope

It records:

- input signal
- preserved metadata
- target display class
- output path
- output transfer
- output color space
- tone mapping mode
- gamut mapping mode
- HDR metadata policy
- DV fallback tier
- claim level
- quality tier
- compliance gate ref
- trace id

## Rule

UI HDR and Dolby Vision text must come from ColorOutputMatrix plus ComplianceGateResult.

