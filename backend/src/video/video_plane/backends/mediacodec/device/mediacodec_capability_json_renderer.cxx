#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_json_renderer.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_bit_depth.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_implementation_class.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_profile_namer.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_json_string_escaper.hpp"

#include <sstream>

namespace kivo::video::mediacodec {
namespace {

constexpr const char* kSchema = "kivo.p12a.capabilities/1";
constexpr std::int32_t kHardwareYes = 1;
constexpr std::int32_t kSoftwareOnlyYes = 1;

void write_string_field(std::ostringstream& out, const char* key, const std::string& value,
                        bool trailing_comma) {
    out << "    \"" << key << "\": \"" << escape_json_string(value) << '"';
    out << (trailing_comma ? ",\n" : "\n");
}

void write_device(std::ostringstream& out, const DeviceIdentity& identity) {
    out << "  \"device\": {\n";
    write_string_field(out, "manufacturer", identity.manufacturer, true);
    write_string_field(out, "model", identity.model, true);
    write_string_field(out, "device", identity.device, true);
    write_string_field(out, "soc_model", identity.soc_model, true);
    write_string_field(out, "board_platform", identity.board_platform, true);
    write_string_field(out, "sdk_int", identity.sdk_int, true);
    write_string_field(out, "release", identity.release, true);
    write_string_field(out, "abi", identity.abi, true);
    write_string_field(out, "vulkan", identity.vulkan, false);
    out << "  },\n";
}

void write_summary(std::ostringstream& out, const CapabilityInventory& inventory) {
    out << "  \"summary\": {\n";
    out << "    \"total_records\": " << inventory.raw_record_count << ",\n";
    out << "    \"unique_records\": " << inventory.records.size() << ",\n";
    out << "    \"rejected_records\": " << inventory.rejected_records << ",\n";
    out << "    \"decoder_records\": " << inventory.decoder_records << ",\n";
    out << "    \"hardware_decoder_records\": " << inventory.hardware_decoder_records << ",\n";
    out << "    \"decoder_10bit_records\": " << inventory.tenbit_decoder_records << ",\n";
    out << "    \"distinct_mimes\": " << inventory.mimes.size() << "\n";
    out << "  },\n";
}

void write_mimes(std::ostringstream& out, const CapabilityInventory& inventory) {
    out << "  \"mimes\": [";
    for (std::size_t index = 0; index < inventory.mimes.size(); ++index) {
        if (index != 0) {
            out << ", ";
        }
        out << '"' << escape_json_string(inventory.mimes[index]) << '"';
    }
    out << "],\n";
}

void write_record(std::ostringstream& out, const CodecInventoryRecord& record) {
    out << "    {\"name\": \"" << escape_json_string(record.name) << '"';
    out << ", \"mime\": \"" << escape_json_string(record.mime) << '"';
    out << ", \"role\": \"" << (record.is_encoder ? "encoder" : "decoder") << '"';
    out << ", \"implementation\": \""
        << escape_json_string(std::string(classify_implementation(record.name))) << '"';
    out << ", \"hardware\": " << (record.is_hardware == kHardwareYes ? "true" : "false");
    out << ", \"software_only\": "
        << (record.is_software_only == kSoftwareOnlyYes ? "true" : "false");
    out << ", \"profile\": \"" << escape_json_string(describe_profile(record.mime, record.profile))
        << '"';
    out << ", \"profile_id\": " << record.profile;
    out << ", \"level\": " << record.level;
    out << ", \"level_name\": \"" << escape_json_string(describe_level(record.mime, record.level))
        << '"';
    out << ", \"bit_depth\": " << infer_bit_depth(record.mime, record.profile);
    out << ", \"max_width\": " << record.max_width;
    out << ", \"max_height\": " << record.max_height;
    out << '}';
}

void write_codecs(std::ostringstream& out, const CapabilityInventory& inventory) {
    out << "  \"codecs\": [\n";
    for (std::size_t index = 0; index < inventory.records.size(); ++index) {
        write_record(out, inventory.records[index]);
        out << (index + 1 == inventory.records.size() ? '\n' : ',');
        if (index + 1 != inventory.records.size()) {
            out << '\n';
        }
    }
    out << "  ]\n";
}

}  // namespace

const char* capability_schema_id() {
    return kSchema;
}

std::string render_capability_json(const DeviceIdentity& identity,
                                   const CapabilityInventory& inventory,
                                   std::int64_t generated_at_unix_ms) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"schema\": \"" << kSchema << "\",\n";
    out << "  \"generated_at_unix_ms\": " << generated_at_unix_ms << ",\n";
    write_device(out, identity);
    write_summary(out, inventory);
    write_mimes(out, inventory);
    write_codecs(out, inventory);
    out << "}\n";
    return out.str();
}

}  // namespace kivo::video::mediacodec
