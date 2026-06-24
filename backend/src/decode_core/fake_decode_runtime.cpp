#include "kivo/cinema_engine/decode_core/fake_decode_runtime.hpp"

namespace kivo::cinema_engine {

DecodeResult build_successful_decode_result(const ContractMetadata& metadata, const DecodedFrame& frame) {
    DecodeResult result;
    result.metadata = metadata;
    result.frame = frame;
    result.success = true;
    result.eof = false;
    result.needs_more_input = false;
    return result;
}

DecodeResult build_eof_decode_result(const ContractMetadata& metadata) {
    DecodeResult result;
    result.metadata = metadata;
    result.success = true;
    result.eof = true;
    result.needs_more_input = false;
    result.frame.is_eof = true;
    return result;
}

DecodeResult build_needs_more_input_result(const ContractMetadata& metadata) {
    DecodeResult result;
    result.metadata = metadata;
    result.success = false;
    result.eof = false;
    result.needs_more_input = true;
    return result;
}

DecodeResult build_failed_decode_result(const ContractMetadata& metadata, const std::string& error_message) {
    DecodeResult result;
    result.metadata = metadata;
    result.success = false;
    result.eof = false;
    result.needs_more_input = false;
    result.error_message = error_message;
    return result;
}

DecodeResult build_decode_result_with_frame(const ContractMetadata& metadata, const DecodedFrame& frame) {
    DecodeResult result;
    result.metadata = metadata;
    result.frame = frame;
    result.success = true;
    result.eof = false;
    result.needs_more_input = false;
    return result;
}

}  // namespace kivo::cinema_engine