#include <cassert>

#include "kivo/cinema_engine/adapter_boundary/adapter_boundary_contract.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

int main() {
  const auto metadata = kivo::cinema_engine::make_contract_metadata("trace.fake.adapter-boundary", "session.fake.0001");

  // Test AdapterBoundaryContract
  kivo::cinema_engine::AdapterBoundaryContract contract;
  contract.metadata = metadata;
  contract.adapter_boundary_id = "adapter.fake.boundary.0001";
  contract.adapter_type = "video_output";
  contract.supported_operations = {"render", "present", "sync"};
  contract.required_capabilities = {"hardware_acceleration", "hdr_support"};
  contract.boundary_valid = true;

  assert(contract.metadata.schema_version == "1.1");
  assert(contract.adapter_boundary_id == "adapter.fake.boundary.0001");
  assert(contract.adapter_type == "video_output");
  assert(contract.supported_operations.size() == 3);
  assert(contract.required_capabilities.size() == 2);
  assert(contract.boundary_valid == true);

  // Test boundary validation
  kivo::cinema_engine::AdapterBoundaryContract invalid_contract;
  invalid_contract.metadata = metadata;
  invalid_contract.boundary_valid = false;
  assert(invalid_contract.boundary_valid == false);

  return 0;
}
