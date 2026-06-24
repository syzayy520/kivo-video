#include "kivo/cinema_engine/source_core/fake_file_identity_service.hpp"

namespace kivo::cinema_engine {

LocalFileIdentity build_stable_file_identity(std::string path) {
  LocalFileIdentity identity;
  identity.metadata.schema_version = "1.1";
  identity.file_path = std::move(path);
  identity.volume_serial = "VOL-001";
  identity.file_index = 12345;
  identity.file_size = 1024 * 1024;  // 1MB
  identity.last_modified_timestamp = 1719360000;  // 2024-06-26
  identity.file_hash = "sha256:abc123";
  identity.identity_stable = true;
  return identity;
}

LocalFileIdentity build_unstable_file_identity(std::string path) {
  LocalFileIdentity identity;
  identity.metadata.schema_version = "1.1";
  identity.file_path = std::move(path);
  identity.volume_serial = "VOL-TMP";
  identity.file_index = 99999;
  identity.file_size = 0;
  identity.last_modified_timestamp = 0;
  identity.identity_stable = false;
  return identity;
}

LocalFileIdentity build_file_identity_with_ids(
    std::string path, std::string volume_serial, uint64_t file_index) {
  LocalFileIdentity identity;
  identity.metadata.schema_version = "1.1";
  identity.file_path = std::move(path);
  identity.volume_serial = std::move(volume_serial);
  identity.file_index = file_index;
  identity.file_size = 2048;
  identity.last_modified_timestamp = 1719360000;
  identity.identity_stable = true;
  return identity;
}

}  // namespace kivo::cinema_engine
