// contract_metadata.hpp — P6A foundation: ContractMetadata for all P6 headers (§6)
// Defined in kivo::video::audio_plane so all sub-namespaces can see it directly.
#pragma once
#include <cstdint>
#include <string_view>
namespace kivo::video::audio_plane {
struct ContractMetadata {
    std::string_view family_name;
    std::string_view header_name;
    std::string_view classification;
    uint32_t version{1};
    bool operator==(const ContractMetadata&) const = default;
};
}  // namespace kivo::video::audio_plane
