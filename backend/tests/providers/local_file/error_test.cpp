#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    lf::LocalFileSessionStore store;

    // Unknown session read
    SourceReadRequest req{SourceSessionId{99999}, std::nullopt, 1};
    auto rr = lf::LocalFileProvider::read(req, store);
    CHECK_TRUE(!rr.is_success());

    // Unknown session close
    auto cerr = lf::LocalFileProvider::close(SourceSessionId{99999}, store);
    CHECK_TRUE(!cerr.is_ok());

    return 0;
}
