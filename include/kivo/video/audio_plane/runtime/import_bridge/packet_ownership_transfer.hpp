// packet_ownership_transfer.hpp — P6B import_bridge: ownership transfer FSM
// HEADER-ONLY. Tracks P5→P6 ownership transfer lifecycle per packet.
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
#include "kivo/video/audio_plane/data/audio_data_ownership_mode.hpp"

namespace kivo::video::audio_plane::runtime::import_bridge {

// TransferError: failure modes for ownership transfer.
enum class TransferError : int32_t {
    NotInP5OwnedState = 0,
    NotInTransferringState = 1,
    AlreadyCompleted = 2,
    AlreadyReleased = 3,
};

// TransferState: lifecycle of a single ownership transfer.
//   P5Owned      → P5 still owns the packet bytes
//   Transferring → transfer in flight (P5 relinquishing, P6 not yet committed)
//   P6Owned      → P6 fully owns the packet bytes
//   Released     → bytes released back to allocator / pool
enum class TransferState : int32_t {
    P5Owned = 0,
    Transferring = 1,
    P6Owned = 2,
    Released = 3,
};

// TransferRejectReason: why a transfer was rejected (original owner releases).
enum class TransferRejectReason : int32_t {
    ValidationFailed = 0,
    ProtectedPolicy = 1,
    Oversize = 2,
    UnsupportedCodec = 3,
};

// PacketOwnershipTransfer
// Single-packet ownership transfer state machine. No real backend; the
// "transfer" is purely a state transition tracked in-process.
class PacketOwnershipTransfer {
public:
    PacketOwnershipTransfer() = default;

    // Begin transfer for a packet under the given ownership mode.
    // Transitions P5Owned → Transferring.
    Expected<std::monostate, TransferError> begin_transfer(
        packet::AudioPacketId packet_id,
        data::AudioDataOwnershipMode mode) noexcept
    {
        if (state_ != TransferState::P5Owned) {
            return TransferError::NotInP5OwnedState;
        }
        packet_id_ = packet_id;
        mode_ = mode;
        state_ = TransferState::Transferring;
        return std::monostate{};
    }

    // Complete the transfer: Transferring → P6Owned.
    Expected<std::monostate, TransferError> complete_transfer() noexcept
    {
        if (state_ != TransferState::Transferring) {
            return TransferError::NotInTransferringState;
        }
        state_ = TransferState::P6Owned;
        return std::monostate{};
    }

    // Reject the transfer: original owner (P5) releases the bytes.
    // Valid from P5Owned or Transferring; lands in Released.
    void reject(TransferRejectReason reason) noexcept
    {
        (void)reason;
        if (state_ == TransferState::P5Owned
            || state_ == TransferState::Transferring) {
            state_ = TransferState::Released;
        }
    }

    // Supersede: P6 retires the packet by generation discard (flush/seek).
    // Valid only from P6Owned; lands in Released.
    void supersede() noexcept
    {
        if (state_ == TransferState::P6Owned) {
            state_ = TransferState::Released;
        }
    }

    // Release after P6 is done with the bytes (normal retire).
    // Valid from P6Owned; lands in Released.
    Expected<std::monostate, TransferError> release() noexcept
    {
        if (state_ == TransferState::Released) {
            return TransferError::AlreadyReleased;
        }
        if (state_ != TransferState::P6Owned) {
            return TransferError::NotInTransferringState;
        }
        state_ = TransferState::Released;
        return std::monostate{};
    }

    TransferState state() const noexcept { return state_; }
    packet::AudioPacketId packet_id() const noexcept { return packet_id_; }
    data::AudioDataOwnershipMode mode() const noexcept { return mode_; }

private:
    TransferState state_{TransferState::P5Owned};
    packet::AudioPacketId packet_id_{};
    data::AudioDataOwnershipMode mode_{data::AudioDataOwnershipMode::Rejected};
};

}  // namespace kivo::video::audio_plane::runtime::import_bridge
