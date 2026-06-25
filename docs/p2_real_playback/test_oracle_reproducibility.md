# P2-050 Test Oracle / Reproducibility Skeleton

## Purpose

Defines test oracle and reproducibility skeleton, including hermetic fake testing, manual hardware pair policy, and test reproducibility requirements.

## Scope

- Hermetic fake test requirements
- Manual hardware pair policy
- Test reproducibility standards
- Environment isolation rules
- Test data management

## Family Tree (testing_core/)

```
testing_core/
├── hermetic_fake_requirement.hpp    — Hermetic fake test requirements
├── hardware_pair_policy.hpp         — Manual hardware pair policy
├── test_reproducibility_rule.hpp    — Test reproducibility rules
├── fake_test_oracle_service.hpp     — Fake service for testing
```

## Contract Types

### HermeticFakeRequirement

| Field | Type | Description |
|-------|------|-------------|
| requirement_id | string | Unique requirement identifier |
| fake_type | string | Type of fake (mock, stub, spy, fake) |
| isolation_level | string | Isolation level (process, thread, function) |
| state_reset_required | bool | Whether state reset is required between tests |
| dependency_injection | bool | Whether dependency injection is required |
| description | string | Requirement description |

### HardwarePairPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| hardware_type | string | Hardware type (GPU, audio, network, storage) |
| pairing_required | bool | Whether hardware pairing is required |
| manual_override_allowed | bool | Whether manual override is allowed |
| fallback_to_fake | bool | Whether to fallback to fake if hardware unavailable |
| timeout_ms | int | Timeout for hardware pairing in milliseconds |

### TestReproducibilityRule

| Field | Type | Description |
|-------|------|-------------|
| rule_id | string | Unique rule identifier |
| rule_name | string | Human-readable rule name |
| deterministic_required | bool | Whether deterministic behavior is required |
| seed_control | bool | Whether random seed control is required |
| time_mocking | bool | Whether time mocking is required |
| environment_isolation | bool | Whether environment isolation is required |

### FakeTestOracleService

12 builder methods:
- build_strict_hermetic_requirement()
- build_relaxed_hermetic_requirement()
- build_process_isolated_requirement()
- build_thread_isolated_requirement()
- build_gpu_hardware_pair_policy()
- build_audio_hardware_pair_policy()
- build_strict_reproducibility_rule()
- build_relaxed_reproducibility_rule()
- build_deterministic_test_result()
- build_seed_controlled_test_result()
- build_time_mocked_test_result()
- build_environment_isolated_test_result()

## Tests

12 tests covering:
1. Strict hermetic requirement construction
2. Relaxed hermetic requirement construction
3. Process isolated requirement construction
4. Thread isolated requirement construction
5. GPU hardware pair policy construction
6. Audio hardware pair policy construction
7. Strict reproducibility rule construction
8. Relaxed reproducibility rule construction
9. Deterministic test result construction
10. Seed controlled test result construction
11. Time mocked test result construction
12. Environment isolated test result construction