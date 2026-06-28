# P6G-039: Negative Fixtures (compile-fail tests)
#
# Each fixture .cpp MUST FAIL TO COMPILE. We create executable targets
# EXCLUDED from ALL (so default build skips them). CTest attempts to build
# each target; WILL_FAIL=TRUE means we expect the build to fail.
# If build succeeds (compile OK), test fails (contract violated).

# Fixture 1: SPSC result must NOT be bool (enum class prevents implicit conversion)
add_executable(p6_neg_spsc_bool_compile neg_spsc_bool_conversion.cpp)
target_link_libraries(p6_neg_spsc_bool_compile PRIVATE kivo_audio_plane_contracts)
target_compile_features(p6_neg_spsc_bool_compile PRIVATE cxx_std_23)
set_target_properties(p6_neg_spsc_bool_compile PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)

add_test(NAME p6_neg_spsc_bool_conversion
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
          --target p6_neg_spsc_bool_compile --config Debug
)
set_tests_properties(p6_neg_spsc_bool_conversion PROPERTIES WILL_FAIL TRUE TIMEOUT 30)

# Fixture 2: EncodedAccessUnit must NOT store span (payload_handle_id is uint64_t)
add_executable(p6_neg_span_compile neg_span_in_access_unit.cpp)
target_link_libraries(p6_neg_span_compile PRIVATE kivo_audio_plane_contracts)
target_compile_features(p6_neg_span_compile PRIVATE cxx_std_23)
set_target_properties(p6_neg_span_compile PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)

add_test(NAME p6_neg_span_in_access_unit
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
          --target p6_neg_span_compile --config Debug
)
set_tests_properties(p6_neg_span_in_access_unit PROPERTIES WILL_FAIL TRUE TIMEOUT 30)
