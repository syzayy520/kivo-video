#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "source_core/test_helpers.hpp"

#include <cstdint>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    auto pk = ProviderKind::local_file;
    CHECK_EQ(pk, ProviderKind::local_file);
    CHECK_NEQ(pk, ProviderKind::unknown);
    CHECK_EQ(static_cast<std::uint8_t>(ProviderKind::unknown), 0);
    CHECK_EQ(static_cast<std::uint8_t>(ProviderKind::local_file), 1);
    CHECK_NEQ(static_cast<int>(ProviderKind::local_file), static_cast<int>(ProviderKind::smb));
    CHECK_NEQ(static_cast<int>(ProviderKind::alist), static_cast<int>(ProviderKind::emby));
    return 0;
}
