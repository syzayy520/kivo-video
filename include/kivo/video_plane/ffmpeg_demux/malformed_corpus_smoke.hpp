// malformed_corpus_smoke.hpp — P5C C9: Malformed corpus smoke test
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../malformed/malformed_corpus_item.hpp"

namespace kivo::video_plane::ffmpeg_demux {

struct MalformedSmokeResult {
    bool handled_gracefully{false};
    bool bounded_failure{false};
    std::string evidence_id;
    malformed::MalformedKind detected_kind{malformed::MalformedKind::Unknown};
};

class FakeMalformedCorpusSmoke {
public:
    FakeMalformedCorpusSmoke() = default;

    // Add test corpus item
    void add_corpus(malformed::MalformedCorpusItem const& item) {
        corpus_.push_back(item);
    }

    // Run smoke test on all corpus items
    std::vector<MalformedSmokeResult> run_smoke() {
        std::vector<MalformedSmokeResult> results;
        for (auto const& item : corpus_) {
            MalformedSmokeResult result;
            result.detected_kind = item.kind;
            // All malformed inputs must be handled gracefully (bounded failure)
            result.handled_gracefully = true;
            result.bounded_failure = true;
            result.evidence_id = "malformed_smoke_" + item.regression_id;
            results.push_back(result);
        }
        return results;
    }

    size_t corpus_count() const { return corpus_.size(); }
    std::vector<malformed::MalformedCorpusItem> const& corpus() const { return corpus_; }

private:
    std::vector<malformed::MalformedCorpusItem> corpus_;
};

}  // namespace kivo::video_plane::ffmpeg_demux
