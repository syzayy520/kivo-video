#pragma once

#include <string>

#include "kivo/cinema_engine/decode_core/decode_result.hpp"
#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

DecodeResult build_successful_decode_result(const ContractMetadata& metadata, const DecodedFrame& frame);
DecodeResult build_eof_decode_result(const ContractMetadata& metadata);
DecodeResult build_needs_more_input_result(const ContractMetadata& metadata);
DecodeResult build_failed_decode_result(const ContractMetadata& metadata, const std::string& error_message);
DecodeResult build_decode_result_with_frame(const ContractMetadata& metadata, const DecodedFrame& frame);

}  // namespace kivo::cinema_engine