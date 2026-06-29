#include "video/playback_graph/evidence/critical_proof_ring.hpp"

namespace kivo::video::playback_graph::runtime {

CriticalProofRing::CriticalProofRing(std::size_t capacity, std::size_t reserved_critical_slots)
    : capacity_(capacity), reserved_critical_slots_(reserved_critical_slots) {
    proofs_.reserve(capacity_ + reserved_critical_slots_);
}

void CriticalProofRing::set_persistent_sink_available(bool available) noexcept {
    persistent_sink_available_ = available;
}

bool CriticalProofRing::push(CriticalProof proof) noexcept {
    if (!persistent_sink_available_) {
        sink_failure_count_ += 1;
    }
    if (proofs_.size() < capacity_ + reserved_critical_slots_) {
        proofs_.push_back(proof);
        return true;
    }
    return false;
}

bool CriticalProofRing::contains(CriticalProofKind kind) const noexcept {
    for (const auto& proof : proofs_) {
        if (proof.kind == kind) {
            return true;
        }
    }
    return false;
}

std::uint32_t CriticalProofRing::sink_failure_count() const noexcept {
    return sink_failure_count_;
}

std::size_t CriticalProofRing::size() const noexcept {
    return proofs_.size();
}

}  // namespace kivo::video::playback_graph::runtime
