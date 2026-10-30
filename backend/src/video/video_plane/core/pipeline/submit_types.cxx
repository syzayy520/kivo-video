#include "video/video_plane/core/pipeline/submit_types.hpp"

namespace kivo::video::core::pipeline {

const char* submit_step_name(SubmitStep step) noexcept {
    switch (step) {
        case SubmitStep::kReadSample:
            return "read_sample";
        case SubmitStep::kSubmitPending:
            return "submit_pending";
        case SubmitStep::kSignalEos:
            return "signal_eos";
        case SubmitStep::kFinished:
            return "finished";
        case SubmitStep::kAbort:
            return "abort";
    }
    return "unknown";
}

}  // namespace kivo::video::core::pipeline
