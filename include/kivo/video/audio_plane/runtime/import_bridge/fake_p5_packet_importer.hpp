// fake_p5_packet_importer.hpp — P6B import_bridge: fake P5→P6 packet importer
// HEADER-ONLY fake runtime. NO real P5 demux / no backend calls.
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/boundary/p5_audio_packet_import_view.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_import_result.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
#include "kivo/video/audio_plane/data/audio_data_ownership_mode.hpp"
#include "kivo/video/audio_plane/data/audio_sync_byte_view.hpp"
#include "kivo/video/audio_plane/data/audio_owned_block_handle.hpp"
#include "kivo/video/audio_plane/data/audio_pool_slot_handle.hpp"

namespace kivo::video::audio_plane::runtime::import_bridge {

// ImportError: failure modes for the fake importer.
enum class ImportError : int32_t {
    NotInitialized = 0,
    InvalidOwnershipMode = 1,
    Closed = 2,
    DrainingRejectsNewPackets = 3,
    GenerationStale = 4,
};

// ImporterState: lifecycle of the import bridge.
enum class ImporterState : int32_t {
    NotInitialized = 0,
    Ready = 1,
    Importing = 2,
    Draining = 3,
    Closed = 4,
};

// FakeP5PacketImporter
// Deterministic, in-process fake of the P5→P6 packet import bridge.
// Handles three ownership modes without any real backend:
//   - SyncBorrowedByteView → copy bytes into a local buffer
//   - MoveOwnedBlock       → take ownership of the block handle
//   - PoolSlotOwnedByP6    → bind to a P6 pool slot
class FakeP5PacketImporter {
public:
    FakeP5PacketImporter() = default;

    // Initialize the importer; transitions NotInitialized → Ready.
    void initialize() noexcept {
        if (state_ == ImporterState::NotInitialized) {
            state_ = ImporterState::Ready;
        }
    }

    // Begin drain; transitions Importing → Draining. New packets rejected.
    void begin_drain() noexcept {
        if (state_ == ImporterState::Importing) {
            state_ = ImporterState::Draining;
        }
    }

    // Close the importer; final state.
    void close() noexcept {
        state_ = ImporterState::Closed;
    }

    ImporterState state() const noexcept { return state_; }

    // Import a packet view under the given ownership mode.
    // Deterministic: no real P5 demux is invoked.
    Expected<packet::AudioPacketImportResult, ImportError> import(
        const boundary::P5AudioPacketImportView& view,
        data::AudioDataOwnershipMode mode) noexcept
    {
        if (state_ == ImporterState::NotInitialized) {
            return ImportError::NotInitialized;
        }
        if (state_ == ImporterState::Closed) {
            return ImportError::Closed;
        }
        if (state_ == ImporterState::Draining) {
            return ImportError::DrainingRejectsNewPackets;
        }
        if (view.stream_id == 0 || view.track_id == 0) {
            return packet::AudioPacketImportResult::RejectedInvalidGeneration;
        }
        if (view.encrypted_or_protected) {
            return packet::AudioPacketImportResult::RejectedProtectedPolicy;
        }

        state_ = ImporterState::Importing;
        last_packet_id_ = packet::AudioPacketId{view.packet_id};

        switch (mode) {
            case data::AudioDataOwnershipMode::SyncBorrowedByteView:
                // Borrowed view: P6 copies bytes; no ownership transfer.
                ++copied_count_;
                return packet::AudioPacketImportResult::AcceptedCopied;
            case data::AudioDataOwnershipMode::MoveOwnedBlock:
                // P6 takes ownership of the block handle.
                ++moved_count_;
                return packet::AudioPacketImportResult::AcceptedMoved;
            case data::AudioDataOwnershipMode::PoolSlotOwnedByP6:
                // P6 binds the packet to a pool slot it owns.
                ++pool_slot_count_;
                return packet::AudioPacketImportResult::AcceptedPoolSlot;
            case data::AudioDataOwnershipMode::CopiedIntoP6PacketPool:
                ++copied_count_;
                return packet::AudioPacketImportResult::AcceptedCopied;
            case data::AudioDataOwnershipMode::ExternalOpaque:
                return ImportError::InvalidOwnershipMode;
            case data::AudioDataOwnershipMode::Rejected:
                return packet::AudioPacketImportResult::RejectedInvalidOwnership;
        }
        return ImportError::InvalidOwnershipMode;
    }

    // Accessors for test/diagnostic inspection (fake, single-threaded).
    uint64_t copied_count() const noexcept { return copied_count_; }
    uint64_t moved_count() const noexcept { return moved_count_; }
    uint64_t pool_slot_count() const noexcept { return pool_slot_count_; }
    packet::AudioPacketId last_packet_id() const noexcept { return last_packet_id_; }

private:
    ImporterState state_{ImporterState::NotInitialized};
    uint64_t copied_count_{0};
    uint64_t moved_count_{0};
    uint64_t pool_slot_count_{0};
    packet::AudioPacketId last_packet_id_{};
};

}  // namespace kivo::video::audio_plane::runtime::import_bridge
