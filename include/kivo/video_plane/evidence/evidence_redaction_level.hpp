#pragma once

namespace kivo::video_plane::evidence {

enum class EvidenceRedactionLevel {
    None,           // no redaction needed (debug-only)
    PiiRedacted,    // PII removed
    PathRedacted,   // private paths redacted
    FullRedacted,   // protected content + PII + paths redacted
    OmitPayload     // payload omitted entirely
};

}  // namespace kivo::video_plane::evidence
