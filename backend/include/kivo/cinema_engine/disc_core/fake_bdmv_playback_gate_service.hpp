#pragma once

#include "bdmv_playback_gate_request.hpp"
#include "bdmv_playback_gate_result.hpp"

namespace kivo::cinema_engine {

// Provides deterministic BDMV playback gate results for testing
class FakeBdmvPlaybackGateService {
public:
    // Valid BDMV folder playback allowed
    static BdmvPlaybackGateResult build_valid_bdmv_gate();
    
    // Mounted ISO playback allowed
    static BdmvPlaybackGateResult build_mounted_iso_gate();
    
    // Encrypted disc playback denied
    static BdmvPlaybackGateResult build_encrypted_disc_gate();
    
    // Missing streams playback denied
    static BdmvPlaybackGateResult build_missing_streams_gate();
    
    // Invalid playlist playback denied
    static BdmvPlaybackGateResult build_invalid_playlist_gate();
};

} // namespace kivo::cinema_engine
