#pragma once

#include "kivo/cinema_engine/color_science_core/color_output_matrix.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/windows_hdr_core/windows_hdr_output_contract.hpp"

namespace kivo::cinema_engine {

WindowsHdrOutputContract build_fake_windows_hdr_output_contract(const ContractMetadata& metadata,
                                                                const ColorOutputMatrix& color_output_matrix);

}  // namespace kivo::cinema_engine

