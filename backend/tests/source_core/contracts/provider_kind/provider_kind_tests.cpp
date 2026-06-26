#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"

#include <cassert>
#include <cstdint>

namespace kivo::video::source_core {
static_assert(sizeof(ProviderKind) == 1, "ProviderKind must be single byte");
}  // namespace kivo::video::source_core

using namespace kivo::video::source_core;

int main() {
    auto pk = ProviderKind::local_file;
    assert(pk == ProviderKind::local_file);
    assert(pk != ProviderKind::unknown);
    
    // unknown = 0
    assert(static_cast<std::uint8_t>(ProviderKind::unknown) == 0);
    assert(static_cast<std::uint8_t>(ProviderKind::local_file) == 1);
    
    // All providers have distinct ordinals
    assert(static_cast<int>(ProviderKind::local_file) != static_cast<int>(ProviderKind::smb));
    assert(static_cast<int>(ProviderKind::alist) != static_cast<int>(ProviderKind::emby));
    
    return 0;
}
