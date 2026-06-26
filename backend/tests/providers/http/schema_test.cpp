#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"
#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "source_core/test_helpers.hpp"
using namespace kivo::video::source_core; using namespace kivo::video::source_core::test;
int main(){CHECK_TRUE(kSourceCoreContractVersion==1); CHECK_EQ(static_cast<int>(ProviderKind::http_file),7); return 0;}
