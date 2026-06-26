#include "kivo/video/providers/local_file/local_file_path_validation.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    CHECK_TRUE(!lf::validate_path("").is_ok());
    CHECK_TRUE(!lf::validate_path("relative\\path").is_ok());
    // symlink and directory tests depend on filesystem state, skip in basic suite
    return 0;
}
