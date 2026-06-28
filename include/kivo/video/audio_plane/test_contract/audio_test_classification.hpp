// audio_test_classification.hpp — P6A test_contract: classification (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioTestClassification
// Classification of an audio test. Compile-only enum.
enum class AudioTestClassification {
    Unit = 0,
    Contract = 1,
    Integration = 2,
    Fake = 3,
    Property = 4,
    Fuzz = 5,
    Benchmark = 6,
    Soak = 7,
    Quality = 8,
    Golden = 9
};

}  // namespace kivo::video::audio_plane::test_contract
