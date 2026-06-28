// audio_conversion_evidence.hpp — P6A conversion: conversion evidence contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioConversionEvidence {
    kivo::video::audio_plane::ContractMetadata metadata;
    uint64_t evidence_id{0};
    int32_t kind{0};
    bool sanitization_applied{false};
};
}  // namespace kivo::video::audio_plane::conversion
