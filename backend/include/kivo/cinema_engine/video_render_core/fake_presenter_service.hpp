#pragma once

#include "kivo/cinema_engine/color_science_core/color_output_matrix.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_render_core/video_presenter_contract.hpp"
#include "kivo/cinema_engine/windows_hdr_core/windows_hdr_output_contract.hpp"

namespace kivo::cinema_engine {

VideoPresenterContract build_fake_video_presenter_contract(const ContractMetadata& metadata,
                                                           const WindowsHdrOutputContract& hdr_output,
                                                           const ColorOutputMatrix& color_output_matrix);

}  // namespace kivo::cinema_engine

