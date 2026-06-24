# Kivo Video

Kivo Video is being built around `KivoCinemaEngine`, a Windows private-cinema backend core.

The active backend rule is: Direct Play First, no silent fallback, no false Dolby claims, UI only talks to Engine API, third-party libraries stay behind backend adapters, authoritative state has one owner, persistent contracts are versioned, and security/compliance/identity failures fail closed.

The backend work starts with P0 contracts and then P1 fake backend execution, as required by the V1.1 Core Freeze Spec.

