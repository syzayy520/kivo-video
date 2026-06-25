#pragma once

#include "source_adapter_identity.hpp"
#include "source_adapter_capability.hpp"
#include "source_adapter_failure.hpp"

namespace kivo::cinema_engine {

// Provides deterministic source adapter results for testing
class FakeSourceAdapterService {
public:
    // Local file adapter
    static SourceAdapterIdentity build_local_file_identity();
    static SourceAdapterCapability build_local_file_capability();
    static SourceAdapterFailure build_local_file_failure();

    // WebDAV adapter with range support
    static SourceAdapterIdentity build_webdav_identity();
    static SourceAdapterCapability build_webdav_capability();
    static SourceAdapterFailure build_webdav_failure();

    // AList adapter with direct link
    static SourceAdapterIdentity build_alist_identity();
    static SourceAdapterCapability build_alist_capability();
    static SourceAdapterFailure build_alist_failure();

    // Emby adapter with session auth
    static SourceAdapterIdentity build_emby_identity();
    static SourceAdapterCapability build_emby_capability();
    static SourceAdapterFailure build_emby_failure();

    // Failed adapter (connection failure)
    static SourceAdapterFailure build_connection_failure();
    static SourceAdapterFailure build_auth_failure();
    static SourceAdapterFailure build_timeout_failure();
};

}  // namespace kivo::cinema_engine
