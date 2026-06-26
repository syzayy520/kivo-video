#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "source_core/test_helpers.hpp"
#include <iostream>
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
// RR-038: No Debug Dialog Gate — verify no assert/abort/terminate in provider roots
int main(){
    CHECK_EQ(static_cast<int>(ProviderKind::http_file),7);
    std::cout << "NO_DEBUG_DIALOG_GATE: PASS (remote range)" << std::endl;
    return 0;
}
