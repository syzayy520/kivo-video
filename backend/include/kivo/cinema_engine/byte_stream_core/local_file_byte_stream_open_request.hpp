#pragma once

#include <string>

namespace kivo::cinema_engine {

struct LocalFileByteStreamOpenRequest {
    std::string path;
    std::string trace_id;
};

}  // namespace kivo::cinema_engine