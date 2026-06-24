add_library(kivo_cinema_engine STATIC
  src/audio_core/fake_audio_service.cpp
  src/color_science_core/fake_color_output_service.cpp
  src/decode_core/fake_decode_path.cpp
  src/decision_ledger_core/decision_ledger_store.cpp
  src/diagnostic_core/diagnostic_redaction_gate.cpp
  src/diagnostic_core/fake_diagnostic_bundle_export.cpp
  src/demux_core/fake_stream_probe.cpp
  src/engine_api/cancellation_token.cpp
  src/engine_api/command_queue.cpp
  src/engine_api/event_bus.cpp
  src/engine_api/state_snapshot_store.cpp
  src/decision_ledger_core/replay_manifest.cpp
  src/fake_backend/fake_engine_api_scenario.cpp
  src/foundation/contract_registry.cpp
  src/fake_backend/fake_session_scenario.cpp
  src/graph_core/fake_graph_runtime.cpp
  src/graph_core/graph_invariant.cpp
  src/network_cache_core/fake_cache_reader.cpp
  src/playback_core/fake_playback_state_machine.cpp
  src/playback_core/playback_invariant.cpp
  src/playback_inspector_core/fake_inspector_service.cpp
  src/policy_core/fake_policy_service.cpp
  src/compliance_core/fake_compliance_service.cpp
  src/feature_gate_core/fake_feature_gate_service.cpp
  src/resource_core/fake_resource_budget_service.cpp
  src/adapter_boundary/adapter_boundary_contract.cpp
  src/capability_core/fake_playback_eligibility_service.cpp
  src/session_core/session_store.cpp
  src/subtitle_core/fake_subtitle_service.cpp
  src/video_render_core/fake_presenter_service.cpp
  src/windows_hdr_core/fake_windows_hdr_service.cpp
)

target_compile_features(kivo_cinema_engine PUBLIC cxx_std_20)

target_include_directories(kivo_cinema_engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(kivo_fake_session_test
  tests/fake_backend/fake_engine_api_scenario_test.cpp
  tests/fake_backend/fake_session_scenario_test.cpp
)

target_link_libraries(kivo_fake_session_test PRIVATE kivo_cinema_engine)

add_executable(kivo_contract_gate_test
  tests/contract_gates/diagnostic_redaction_gate_test.cpp
  tests/contract_gates/playback_invariant_test.cpp
)

target_link_libraries(kivo_contract_gate_test PRIVATE kivo_cinema_engine)

add_executable(kivo_engine_api_test
  tests/engine_api/engine_api_boundary_test.cpp
)

target_link_libraries(kivo_engine_api_test PRIVATE kivo_cinema_engine)

add_executable(kivo_session_ledger_test
  tests/session_ledger/session_ledger_store_test.cpp
)

target_link_libraries(kivo_session_ledger_test PRIVATE kivo_cinema_engine)

add_executable(kivo_playback_core_test
  tests/playback_core/fake_playback_state_machine_test.cpp
)

target_link_libraries(kivo_playback_core_test PRIVATE kivo_cinema_engine)

add_executable(kivo_graph_core_test
  tests/graph_core/fake_graph_runtime_test.cpp
)

target_link_libraries(kivo_graph_core_test PRIVATE kivo_cinema_engine)

add_executable(kivo_media_pipeline_contract_test
  tests/media_pipeline/media_pipeline_contract_test.cpp
)

target_link_libraries(kivo_media_pipeline_contract_test PRIVATE kivo_cinema_engine)

add_executable(kivo_color_hdr_presenter_test
  tests/color_hdr_presenter/color_hdr_presenter_contract_test.cpp
)

target_link_libraries(kivo_color_hdr_presenter_test PRIVATE kivo_cinema_engine)

add_executable(kivo_audio_subtitle_test
  tests/audio_subtitle/audio_subtitle_contract_test.cpp
)

target_link_libraries(kivo_audio_subtitle_test PRIVATE kivo_cinema_engine)

add_executable(kivo_inspector_diagnostic_test
  tests/inspector_diagnostic/inspector_diagnostic_contract_test.cpp
)

target_link_libraries(kivo_inspector_diagnostic_test PRIVATE kivo_cinema_engine)

add_executable(kivo_policy_compliance_resource_test
  tests/policy_compliance_resource/policy_compliance_resource_contract_test.cpp
)

target_link_libraries(kivo_policy_compliance_resource_test PRIVATE kivo_cinema_engine)

add_executable(kivo_adapter_boundary_test
  tests/adapter_boundary/adapter_boundary_test.cpp
)

target_link_libraries(kivo_adapter_boundary_test PRIVATE kivo_cinema_engine)

add_executable(kivo_capability_eligibility_test
  tests/capability_core/playback_eligibility_test.cpp
)

target_link_libraries(kivo_capability_eligibility_test PRIVATE kivo_cinema_engine)

enable_testing()
add_test(NAME kivo_fake_session_test COMMAND kivo_fake_session_test)
add_test(NAME kivo_contract_gate_test COMMAND kivo_contract_gate_test)
add_test(NAME kivo_engine_api_test COMMAND kivo_engine_api_test)
add_test(NAME kivo_session_ledger_test COMMAND kivo_session_ledger_test)
add_test(NAME kivo_playback_core_test COMMAND kivo_playback_core_test)
add_test(NAME kivo_graph_core_test COMMAND kivo_graph_core_test)
add_test(NAME kivo_media_pipeline_contract_test COMMAND kivo_media_pipeline_contract_test)
add_test(NAME kivo_color_hdr_presenter_test COMMAND kivo_color_hdr_presenter_test)
add_test(NAME kivo_audio_subtitle_test COMMAND kivo_audio_subtitle_test)
add_test(NAME kivo_inspector_diagnostic_test COMMAND kivo_inspector_diagnostic_test)
add_test(NAME kivo_policy_compliance_resource_test COMMAND kivo_policy_compliance_resource_test)
add_test(NAME kivo_adapter_boundary_test COMMAND kivo_adapter_boundary_test)
add_test(NAME kivo_capability_eligibility_test COMMAND kivo_capability_eligibility_test)
