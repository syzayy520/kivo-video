#pragma once

#include "network_range_reader.hpp"
#include "range_read_request.hpp"
#include "range_read_result.hpp"

namespace kivo::cinema_engine {

// Provides deterministic range read results for testing
class FakeRangeReaderService {
public:
    // Configured network range reader
    static NetworkRangeReader build_range_reader();

    // Successful full range read
    static RangeReadResult build_successful_range_read();

    // Range not satisfiable (HTTP 416)
    static RangeReadResult build_range_not_satisfiable();

    // Partial content (HTTP 206)
    static RangeReadResult build_partial_range_read();

    // Connection error during read
    static RangeReadResult build_range_read_error();
};

}  // namespace kivo::cinema_engine
