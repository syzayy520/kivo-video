// iec61937_parser.hpp — P6E: IEC61937 burst framing parser
#pragma once

#include <cstdint>
#include <vector>

namespace kivo::video::audio_plane::backend::runtime::passthrough {

enum class Iec61937BurstType : int32_t {
    Unknown = 0,
    Ac3 = 1,
    Eac3 = 2,
    Dts = 3,
    TrueHd = 4,
    DtsHd = 5,
};

struct Iec61937BurstInfo {
    Iec61937BurstType type{Iec61937BurstType::Unknown};
    uint32_t burst_size{0};
    uint32_t data_rate{0};
    bool valid{false};
};

// Iec61937Parser
// Parses IEC61937 burst frames (S/PDIF encapsulation of compressed audio).
class Iec61937Parser {
public:
    Iec61937Parser() = default;

    // Parse a burst preamble (Pa Pb Pc Pd — 8 bytes, 4 uint16 LE).
    // Returns burst info if valid IEC61937 preamble detected.
    Iec61937BurstInfo parse_preamble(const uint16_t* words, int count) const noexcept;

    // Check if the first two words match the IEC61937 Pa Pb sync pattern.
    bool is_burst_start(const uint16_t* words, int count) const noexcept;

    // Map burst type code to enum.
    static Iec61937BurstType map_burst_type(uint16_t type_code) noexcept;
};

}  // namespace kivo::video::audio_plane::backend::runtime::passthrough
