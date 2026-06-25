#pragma once

#include <cstdint>
#include <string>

namespace kivo::cinema_engine {

struct LocalFileByteStreamOpenResult {
    bool opened;
    std::uint64_t size_bytes;
    bool seekable;
    std::string error_code;
    std::string inspector_hint;
    std::string trace_id;
};

}  // namespace kivo::cinema_engine