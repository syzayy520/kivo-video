#pragma once

#include "kivo/cinema_engine/source_core/byte_stream_contract.hpp"

namespace kivo::cinema_engine {

// Build a local file byte stream.
ByteStreamContract build_local_file_stream(std::string stream_id, int64_t file_size);

// Build a network range stream.
ByteStreamContract build_network_range_stream(std::string stream_id, int64_t content_length);

// Build a cache segment stream.
ByteStreamContract build_cache_segment_stream(std::string stream_id, int64_t segment_size);

}  // namespace kivo::cinema_engine
