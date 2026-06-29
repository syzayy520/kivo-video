#pragma once

namespace kivo::video::playback_graph::runtime {

enum class HotPathViolation {
    None,
    Allocation,
    Blocking,
    Logging,
    MutableGraphAccess,
    FileIo,
    NetworkIo,
    StringFormatting,
    Sleep,
    UnboundedRetry
};

class HotPathContractGuard {
public:
    void enter_hot_path() noexcept {
        in_hot_path_ = true;
        last_violation_ = HotPathViolation::None;
    }

    void leave_hot_path() noexcept {
        in_hot_path_ = false;
    }

    [[nodiscard]] bool record_allocation_attempt() noexcept {
        return record(HotPathViolation::Allocation);
    }

    [[nodiscard]] bool record_blocking_attempt() noexcept {
        return record(HotPathViolation::Blocking);
    }

    [[nodiscard]] bool record_logging_attempt() noexcept {
        return record(HotPathViolation::Logging);
    }

    [[nodiscard]] bool record_mutable_graph_access_attempt() noexcept {
        return record(HotPathViolation::MutableGraphAccess);
    }

    [[nodiscard]] bool record_unbounded_retry_attempt() noexcept {
        return record(HotPathViolation::UnboundedRetry);
    }

    [[nodiscard]] HotPathViolation last_violation() const noexcept {
        return last_violation_;
    }

private:
    [[nodiscard]] bool record(HotPathViolation violation) noexcept {
        if (!in_hot_path_) {
            return true;
        }
        last_violation_ = violation;
        return false;
    }

    bool in_hot_path_{false};
    HotPathViolation last_violation_{HotPathViolation::None};
};

}  // namespace kivo::video::playback_graph::runtime
