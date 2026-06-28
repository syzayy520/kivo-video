// iec61937_parser.cpp — P6E: IEC61937 parser implementation
#include "video/audio_plane/runtime/passthrough/iec61937_parser.hpp"

namespace kivo::video::audio_plane::backend::runtime::passthrough {

// IEC61937 sync pattern: Pa=0xF872, Pb=0x4E1F (little-endian uint16)
static constexpr uint16_t IEC61937_PA = 0xF872;
static constexpr uint16_t IEC61937_PB = 0x4E1F;

bool Iec61937Parser::is_burst_start(const uint16_t* words, int count) const noexcept {
    if (!words || count < 2) return false;
    return words[0] == IEC61937_PA && words[1] == IEC61937_PB;
}

Iec61937BurstType Iec61937Parser::map_burst_type(uint16_t type_code) noexcept {
    switch (type_code) {
        case 0x01: return Iec61937BurstType::Ac3;
        case 0x04: return Iec61937BurstType::Eac3;
        case 0x0B: return Iec61937BurstType::Dts;
        case 0x15: return Iec61937BurstType::TrueHd;
        case 0x16: return Iec61937BurstType::DtsHd;
        default: return Iec61937BurstType::Unknown;
    }
}

Iec61937BurstInfo Iec61937Parser::parse_preamble(const uint16_t* words, int count) const noexcept {
    Iec61937BurstInfo info;
    if (!is_burst_start(words, count)) return info;
    if (count < 4) return info;

    // Pc = words[2]: bits 0-1 = data type, bits 2-6 = data rate
    uint16_t pc = words[2];
    uint16_t type_code = pc & 0x07;
    info.type = map_burst_type(type_code);

    // Pd = words[3]: burst size in bytes
    info.burst_size = words[3];

    // Data rate from Pc bits 2-6
    info.data_rate = (pc >> 2) & 0x1F;

    info.valid = (info.type != Iec61937BurstType::Unknown);
    return info;
}

}  // namespace kivo::video::audio_plane::backend::runtime::passthrough
