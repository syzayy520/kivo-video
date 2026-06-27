option(KIVO_ENABLE_FFMPEG "Enable FFmpeg-based real media processing" OFF)
option(KIVO_ENABLE_REAL_MEDIA_TESTS "Enable integration tests requiring real media samples" OFF)

add_library(kivo_cinema_engine STATIC
  src/audio_core/fake_audio_service.cpp
  src/color_science_core/fake_color_output_service.cpp
  src/decode_core/fake_decode_path.cpp
  src/decode_core/fake_decode_runtime.cpp
  src/decode_core/fake_frame_queue_service.cpp
  src/decision_ledger_core/decision_ledger_store.cpp
  src/diagnostic_core/diagnostic_redaction_gate.cpp
  src/diagnostic_core/fake_diagnostic_bundle_export.cpp
  src/demux_core/fake_stream_probe.cpp
  src/demux_core/fake_probe_service.cpp
  src/demux_core/fake_demux_runtime.cpp
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
  src/playback_core/playback_session_state_machine.cpp
  src/preference_core/fake_preference_service.cpp
  src/media_timeline_core/fake_timebase_service.cpp
  src/media_timeline_core/fake_master_clock_service.cpp
  src/engine_api/fake_playback_command_service.cpp
  src/source_core/fake_file_identity_service.cpp
  src/source_core/fake_local_file_source_service.cpp
  src/source_core/fake_source_adapter_service.cpp
  src/source_core/fake_source_health_service.cpp
  src/source_core/fake_webdav_profile_service.cpp
  src/source_core/fake_webdav_range_playback_service.cpp
  src/source_core/fake_alist_profile_service.cpp
  src/source_core/fake_alist_direct_link_service.cpp
  src/source_core/fake_emby_session_service.cpp
  src/source_core/fake_emby_direct_play_service.cpp
  src/playback_inspector_core/fake_inspector_service.cpp
  src/playback_inspector_core/fake_inspector_runtime_service.cpp
  src/policy_core/fake_policy_service.cpp
  src/compliance_core/fake_compliance_service.cpp
  src/feature_gate_core/fake_feature_gate_service.cpp
  src/resource_core/fake_resource_budget_service.cpp
  src/resource_core/fake_memory_budget_service.cpp
  src/scheduling_core/fake_scheduling_service.cpp
  src/adapter_boundary/adapter_boundary_contract.cpp
  src/capability_core/fake_playback_eligibility_service.cpp
  src/session_core/session_store.cpp
  src/subtitle_core/fake_subtitle_service.cpp
  src/subtitle_core/fake_subtitle_runtime_service.cpp
  src/video_render_core/fake_presenter_service.cpp
  src/video_render_core/fake_sdr_upload_service.cpp
  src/video_render_core/fake_visual_quality_service.cpp
  src/video_render_core/fake_flip_presenter_service.cpp
  src/audio_core/fake_audio_endpoint_service.cpp
  src/quality_core/fake_user_perceived_quality_service.cpp
  src/windows_hdr_core/fake_windows_hdr_service.cpp
  src/disc_core/fake_bdmv_probe_service.cpp
  src/disc_core/fake_bdmv_playlist_duplicate_service.cpp
  src/disc_core/fake_bdmv_main_title_service.cpp
  src/disc_core/fake_bdmv_playback_gate_service.cpp
  src/security_core/fake_network_security_service.cpp
  src/network_cache_core/fake_range_reader_service.cpp
  src/network_cache_core/fake_segment_cache_service.cpp
  src/network_cache_core/fake_remote_direct_play_gate_service.cpp
  src/color_science_core/fake_hdr_color_service.cpp
  src/governance_core/fake_claim_vocabulary_gate_service.cpp
  src/diagnostic_core/fake_telemetry_sampling_service.cpp
  src/governance_core/fake_schema_migration_service.cpp
  src/storage_core/fake_database_resilience_service.cpp
  src/diagnostic_core/fake_crash_dump_privacy_service.cpp
  src/compliance_core/fake_dependency_security_service.cpp
  src/governance_core/fake_product_claim_service.cpp
  src/feature_gate_core/fake_feature_flag_service.cpp
  src/testing_core/fake_test_oracle_service.cpp
  src/parser_security_core/fake_parser_security_service.cpp
  src/build_core/fake_build_config_service.cpp
  src/signing_core/fake_signing_service.cpp
  src/symbol_core/fake_symbol_service.cpp
  src/update_core/fake_update_service.cpp
  src/recovery_core/fake_recovery_fallback_service.cpp
  src/release_core/fake_release_readiness_service.cpp
  src/integration_core/fake_integration_gate_service.cpp
  src/byte_stream_core/local_file_byte_stream_impl.cpp
  src/demux_core/real_probe_runtime.cpp
  src/demux_core/real_demux_runtime.cpp
  src/decode_core/real_software_decode_runtime.cpp
  src/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.cpp
  src/video_upload/d3d11_device_context.cpp
  src/video_upload/d3d11_texture_upload.cpp
  src/presenter/presenter_boundary_shell.cpp
  src/audio_output/decoded_audio_frame_converter.cpp
  src/audio_output/wasapi_shared_pcm_writer.cpp
)

target_compile_features(kivo_cinema_engine PUBLIC cxx_std_20)

target_include_directories(kivo_cinema_engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(KIVO_ENABLE_FFMPEG)
  target_compile_definitions(kivo_cinema_engine PUBLIC KIVO_ENABLE_FFMPEG)
  if(FFMPEG_ROOT)
    message(STATUS "FFmpeg SDK root: ${FFMPEG_ROOT}")
    target_include_directories(kivo_cinema_engine PUBLIC ${FFMPEG_ROOT}/include)
    target_link_directories(kivo_cinema_engine PUBLIC ${FFMPEG_ROOT}/lib)
    target_link_libraries(kivo_cinema_engine PUBLIC avformat avcodec avutil swresample)
  else()
    message(WARNING "KIVO_ENABLE_FFMPEG=ON but FFMPEG_ROOT is not set. FFmpeg symbols will not be linked.")
  endif()
endif()

# Windows platform libraries for D3D11 and WASAPI
if(WIN32)
  target_link_libraries(kivo_cinema_engine PUBLIC
    d3d11
    dxgi
    ole32
    oleaut32
    mmdevapi
    mfplat
    mfuuid
  )
endif()

if(KIVO_ENABLE_REAL_MEDIA_TESTS)
  target_compile_definitions(kivo_cinema_engine PUBLIC KIVO_ENABLE_REAL_MEDIA_TESTS)
endif()

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

add_executable(kivo_inspector_runtime_test
  tests/playback_inspector_core/inspector_runtime_test.cpp
)
target_link_libraries(kivo_inspector_runtime_test PRIVATE kivo_cinema_engine)

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

add_executable(kivo_playback_state_machine_test
  tests/playback_core/playback_session_state_machine_test.cpp
)

target_link_libraries(kivo_capability_eligibility_test PRIVATE kivo_cinema_engine)

target_link_libraries(kivo_playback_state_machine_test PRIVATE kivo_cinema_engine)

add_executable(kivo_preference_test
  tests/preference_core/preference_snapshot_test.cpp
)
target_link_libraries(kivo_preference_test PRIVATE kivo_cinema_engine)

add_executable(kivo_timebase_test
  tests/media_timeline_core/timebase_normalization_test.cpp
)
target_link_libraries(kivo_timebase_test PRIVATE kivo_cinema_engine)

add_executable(kivo_master_clock_test
  tests/media_timeline_core/master_clock_av_sync_test.cpp
)
target_link_libraries(kivo_master_clock_test PRIVATE kivo_cinema_engine)

add_executable(kivo_playback_command_test
  tests/engine_api/playback_command_test.cpp
)
target_link_libraries(kivo_playback_command_test PRIVATE kivo_cinema_engine)

add_executable(kivo_file_identity_test
  tests/source_core/file_identity_test.cpp
)
target_link_libraries(kivo_file_identity_test PRIVATE kivo_cinema_engine)

add_executable(kivo_local_file_source_test
  tests/source_core/local_file_source_test.cpp
)
target_link_libraries(kivo_local_file_source_test PRIVATE kivo_cinema_engine)

add_executable(kivo_probe_service_test
  tests/demux_core/probe_service_test.cpp
)
target_link_libraries(kivo_probe_service_test PRIVATE kivo_cinema_engine)

add_executable(kivo_frame_queue_test
  tests/decode_core/frame_queue_test.cpp
)
target_link_libraries(kivo_frame_queue_test PRIVATE kivo_cinema_engine)

add_executable(kivo_memory_budget_test
  tests/resource_core/memory_budget_test.cpp
)
target_link_libraries(kivo_memory_budget_test PRIVATE kivo_cinema_engine)

add_executable(kivo_scheduling_policy_test
  tests/scheduling_core/scheduling_policy_test.cpp
)
target_link_libraries(kivo_scheduling_policy_test PRIVATE kivo_cinema_engine)

add_executable(kivo_demux_runtime_test
  tests/demux_core/demux_runtime_test.cpp
)
target_link_libraries(kivo_demux_runtime_test PRIVATE kivo_cinema_engine)

add_executable(kivo_decode_runtime_test
  tests/decode_core/decode_runtime_test.cpp
)
target_link_libraries(kivo_decode_runtime_test PRIVATE kivo_cinema_engine)

add_executable(kivo_sdr_upload_visual_quality_test
  tests/video_render_core/sdr_upload_visual_quality_test.cpp
)
target_link_libraries(kivo_sdr_upload_visual_quality_test PRIVATE kivo_cinema_engine)

add_executable(kivo_flip_presenter_first_frame_test
  tests/video_render_core/flip_presenter_first_frame_test.cpp
)
target_link_libraries(kivo_flip_presenter_first_frame_test PRIVATE kivo_cinema_engine)

add_executable(kivo_audio_endpoint_test
  tests/audio_core/audio_endpoint_test.cpp
)
target_link_libraries(kivo_audio_endpoint_test PRIVATE kivo_cinema_engine)

add_executable(kivo_user_perceived_quality_test
  tests/quality_core/user_perceived_quality_test.cpp
)
target_link_libraries(kivo_user_perceived_quality_test PRIVATE kivo_cinema_engine)

add_executable(kivo_local_file_playback_gate_test
  tests/end_to_end/local_file_playback_gate_test.cpp
)
target_link_libraries(kivo_local_file_playback_gate_test PRIVATE kivo_cinema_engine)

add_executable(kivo_bdmv_probe_test
  tests/disc_core/bdmv_probe_test.cpp
)
target_link_libraries(kivo_bdmv_probe_test PRIVATE kivo_cinema_engine)

add_executable(kivo_bdmv_playlist_duplicate_test
  tests/disc_core/bdmv_playlist_duplicate_test.cpp
)
target_link_libraries(kivo_bdmv_playlist_duplicate_test PRIVATE kivo_cinema_engine)

add_executable(kivo_bdmv_main_title_test
  tests/disc_core/bdmv_main_title_test.cpp
)
target_link_libraries(kivo_bdmv_main_title_test PRIVATE kivo_cinema_engine)

add_executable(kivo_bdmv_playback_gate_test
  tests/disc_core/bdmv_playback_gate_test.cpp
)
target_link_libraries(kivo_bdmv_playback_gate_test PRIVATE kivo_cinema_engine)

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
add_test(NAME kivo_inspector_runtime_test COMMAND kivo_inspector_runtime_test)
add_test(NAME kivo_policy_compliance_resource_test COMMAND kivo_policy_compliance_resource_test)
add_test(NAME kivo_adapter_boundary_test COMMAND kivo_adapter_boundary_test)
add_test(NAME kivo_capability_eligibility_test COMMAND kivo_capability_eligibility_test)
add_test(NAME kivo_playback_state_machine_test COMMAND kivo_playback_state_machine_test)
add_test(NAME kivo_preference_test COMMAND kivo_preference_test)
add_test(NAME kivo_timebase_test COMMAND kivo_timebase_test)
add_test(NAME kivo_master_clock_test COMMAND kivo_master_clock_test)
add_test(NAME kivo_playback_command_test COMMAND kivo_playback_command_test)
add_test(NAME kivo_file_identity_test COMMAND kivo_file_identity_test)
add_test(NAME kivo_local_file_source_test COMMAND kivo_local_file_source_test)
add_test(NAME kivo_probe_service_test COMMAND kivo_probe_service_test)
add_test(NAME kivo_frame_queue_test COMMAND kivo_frame_queue_test)
add_test(NAME kivo_memory_budget_test COMMAND kivo_memory_budget_test)
add_test(NAME kivo_scheduling_policy_test COMMAND kivo_scheduling_policy_test)
add_test(NAME kivo_demux_runtime_test COMMAND kivo_demux_runtime_test)
add_test(NAME kivo_decode_runtime_test COMMAND kivo_decode_runtime_test)
add_test(NAME kivo_sdr_upload_visual_quality_test COMMAND kivo_sdr_upload_visual_quality_test)
add_test(NAME kivo_flip_presenter_first_frame_test COMMAND kivo_flip_presenter_first_frame_test)
add_test(NAME kivo_audio_endpoint_test COMMAND kivo_audio_endpoint_test)
add_test(NAME kivo_user_perceived_quality_test COMMAND kivo_user_perceived_quality_test)
add_test(NAME kivo_local_file_playback_gate_test COMMAND kivo_local_file_playback_gate_test)
add_test(NAME kivo_bdmv_probe_test COMMAND kivo_bdmv_probe_test)
add_test(NAME kivo_bdmv_playlist_duplicate_test COMMAND kivo_bdmv_playlist_duplicate_test)
add_test(NAME kivo_bdmv_main_title_test COMMAND kivo_bdmv_main_title_test)
add_test(NAME kivo_bdmv_playback_gate_test COMMAND kivo_bdmv_playback_gate_test)

add_executable(kivo_source_adapter_test
  tests/source_core/source_adapter_test.cpp
)
target_link_libraries(kivo_source_adapter_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_source_adapter_test COMMAND kivo_source_adapter_test)

add_executable(kivo_network_security_test
  tests/security_core/network_security_test.cpp
)
target_link_libraries(kivo_network_security_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_network_security_test COMMAND kivo_network_security_test)

add_executable(kivo_source_health_test
  tests/source_core/source_health_test.cpp
)
target_link_libraries(kivo_source_health_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_source_health_test COMMAND kivo_source_health_test)

add_executable(kivo_range_reader_test
  tests/network_cache_core/range_reader_test.cpp
)
target_link_libraries(kivo_range_reader_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_range_reader_test COMMAND kivo_range_reader_test)

add_executable(kivo_segment_cache_test
  tests/network_cache_core/segment_cache_test.cpp
)
target_link_libraries(kivo_segment_cache_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_segment_cache_test COMMAND kivo_segment_cache_test)

add_executable(kivo_webdav_profile_test
  tests/source_core/webdav_profile_test.cpp
)
target_link_libraries(kivo_webdav_profile_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_webdav_profile_test COMMAND kivo_webdav_profile_test)

add_executable(kivo_webdav_range_playback_test
  tests/source_core/webdav_range_playback_test.cpp
)
target_link_libraries(kivo_webdav_range_playback_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_webdav_range_playback_test COMMAND kivo_webdav_range_playback_test)

add_executable(kivo_alist_profile_test
  tests/source_core/alist_profile_test.cpp
)
target_link_libraries(kivo_alist_profile_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_alist_profile_test COMMAND kivo_alist_profile_test)

add_executable(kivo_alist_direct_link_test
  tests/source_core/alist_direct_link_test.cpp
)
target_link_libraries(kivo_alist_direct_link_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_alist_direct_link_test COMMAND kivo_alist_direct_link_test)

add_executable(kivo_emby_session_test
  tests/source_core/emby_session_test.cpp
)
target_link_libraries(kivo_emby_session_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_emby_session_test COMMAND kivo_emby_session_test)

add_executable(kivo_emby_direct_play_test
  tests/source_core/emby_direct_play_test.cpp
)
target_link_libraries(kivo_emby_direct_play_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_emby_direct_play_test COMMAND kivo_emby_direct_play_test)

add_executable(kivo_remote_direct_play_gate_test
  tests/network_cache_core/remote_direct_play_gate_test.cpp
)
target_link_libraries(kivo_remote_direct_play_gate_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_remote_direct_play_gate_test COMMAND kivo_remote_direct_play_gate_test)

add_executable(kivo_subtitle_runtime_test
  tests/subtitle_core/subtitle_runtime_test.cpp
)
target_link_libraries(kivo_subtitle_runtime_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_subtitle_runtime_test COMMAND kivo_subtitle_runtime_test)

add_executable(kivo_hdr_color_runtime_test
  tests/color_science_core/hdr_color_runtime_test.cpp
)
target_link_libraries(kivo_hdr_color_runtime_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_hdr_color_runtime_test COMMAND kivo_hdr_color_runtime_test)

add_executable(kivo_claim_vocabulary_gate_test
  tests/governance_core/claim_vocabulary_gate_test.cpp
)
target_link_libraries(kivo_claim_vocabulary_gate_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_claim_vocabulary_gate_test COMMAND kivo_claim_vocabulary_gate_test)

add_executable(kivo_telemetry_sampling_test
  tests/diagnostic_core/telemetry_sampling_test.cpp
)
target_link_libraries(kivo_telemetry_sampling_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_telemetry_sampling_test COMMAND kivo_telemetry_sampling_test)

add_executable(kivo_schema_migration_test
  tests/governance_core/schema_migration_test.cpp
)
target_link_libraries(kivo_schema_migration_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_schema_migration_test COMMAND kivo_schema_migration_test)

add_executable(kivo_database_resilience_test
  tests/storage_core/database_resilience_test.cpp
)
target_link_libraries(kivo_database_resilience_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_database_resilience_test COMMAND kivo_database_resilience_test)

add_executable(kivo_crash_dump_privacy_test
  tests/diagnostic_core/crash_dump_privacy_test.cpp
)
target_link_libraries(kivo_crash_dump_privacy_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_crash_dump_privacy_test COMMAND kivo_crash_dump_privacy_test)

add_executable(kivo_dependency_security_test
  tests/compliance_core/dependency_security_test.cpp
)
target_link_libraries(kivo_dependency_security_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_dependency_security_test COMMAND kivo_dependency_security_test)

add_executable(kivo_product_claim_test
  tests/governance_core/product_claim_test.cpp
)
target_link_libraries(kivo_product_claim_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_product_claim_test COMMAND kivo_product_claim_test)

add_executable(kivo_feature_flag_test
  tests/feature_gate_core/feature_flag_test.cpp
)
target_link_libraries(kivo_feature_flag_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_feature_flag_test COMMAND kivo_feature_flag_test)

add_executable(kivo_test_oracle_test
  tests/testing_core/test_oracle_test.cpp
)
target_link_libraries(kivo_test_oracle_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_test_oracle_test COMMAND kivo_test_oracle_test)

add_executable(kivo_parser_security_test
  tests/parser_security_core/parser_security_test.cpp
)
target_link_libraries(kivo_parser_security_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_parser_security_test COMMAND kivo_parser_security_test)

add_executable(kivo_build_signing_symbol_update_test
  tests/build_signing_symbol_update/build_signing_symbol_update_test.cpp
)
target_link_libraries(kivo_build_signing_symbol_update_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_build_signing_symbol_update_test COMMAND kivo_build_signing_symbol_update_test)

add_executable(kivo_recovery_fallback_test
  tests/recovery_core/recovery_fallback_test.cpp
)
target_link_libraries(kivo_recovery_fallback_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_recovery_fallback_test COMMAND kivo_recovery_fallback_test)

add_executable(kivo_release_readiness_test
  tests/release_core/release_readiness_test.cpp
)
target_link_libraries(kivo_release_readiness_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_release_readiness_test COMMAND kivo_release_readiness_test)

add_executable(kivo_integration_gate_test
  tests/integration_core/integration_gate_test.cpp
)
target_link_libraries(kivo_integration_gate_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_integration_gate_test COMMAND kivo_integration_gate_test)

# ─── P2-REAL-001: Real media processing test targets ───

add_executable(kivo_real_probe_runtime_test
  tests/demux_core/real_probe_runtime_test.cpp
)
target_link_libraries(kivo_real_probe_runtime_test PRIVATE kivo_cinema_engine)

add_executable(kivo_real_demux_runtime_test
  tests/demux_core/real_demux_runtime_test.cpp
)
target_link_libraries(kivo_real_demux_runtime_test PRIVATE kivo_cinema_engine)

add_executable(kivo_real_decode_runtime_test
  tests/decode_core/real_software_decode_runtime_test.cpp
)
target_link_libraries(kivo_real_decode_runtime_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_real_probe_runtime_test COMMAND kivo_real_probe_runtime_test)
add_test(NAME kivo_real_demux_runtime_test COMMAND kivo_real_demux_runtime_test)
add_test(NAME kivo_real_decode_runtime_test COMMAND kivo_real_decode_runtime_test)

# ─── P2-REAL-001: Integration test targets (require KIVO_ENABLE_REAL_MEDIA_TESTS) ───

add_executable(kivo_local_file_byte_stream_integration_test
  tests/integration_local/local_file_byte_stream_integration_test.cpp
)
target_link_libraries(kivo_local_file_byte_stream_integration_test PRIVATE kivo_cinema_engine)

add_executable(kivo_real_probe_integration_test
  tests/integration_local/real_probe_integration_test.cpp
)
target_link_libraries(kivo_real_probe_integration_test PRIVATE kivo_cinema_engine)

add_executable(kivo_real_demux_decode_integration_test
  tests/integration_local/real_demux_decode_integration_test.cpp
)
target_link_libraries(kivo_real_demux_decode_integration_test PRIVATE kivo_cinema_engine)

if(KIVO_ENABLE_REAL_MEDIA_TESTS)
  add_test(NAME kivo_local_file_byte_stream_integration_test COMMAND kivo_local_file_byte_stream_integration_test)
  add_test(NAME kivo_real_probe_integration_test COMMAND kivo_real_probe_integration_test)
  add_test(NAME kivo_real_demux_decode_integration_test COMMAND kivo_real_demux_decode_integration_test)
endif()

# ─── V10: D3D11 first frame test ───

add_executable(kivo_d3d11_first_frame_test
  tests/video_upload/d3d11_first_frame_test.cpp
)
target_link_libraries(kivo_d3d11_first_frame_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_d3d11_first_frame_test COMMAND kivo_d3d11_first_frame_test)

# ─── V10: Presenter boundary first frame test ───

add_executable(kivo_presenter_boundary_first_frame_test
  tests/presenter/presenter_boundary_first_frame_test.cpp
)
target_link_libraries(kivo_presenter_boundary_first_frame_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_presenter_boundary_first_frame_test COMMAND kivo_presenter_boundary_first_frame_test)
set_tests_properties(kivo_presenter_boundary_first_frame_test PROPERTIES
  SKIP_REGULAR_EXPRESSION "SKIP: D3D11 device not available"
)

# ─── V10: Audio output tests ───

add_executable(kivo_decoded_audio_frame_converter_test
  tests/audio_output/decoded_audio_frame_converter_test.cpp
)
target_link_libraries(kivo_decoded_audio_frame_converter_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_decoded_audio_frame_converter_test COMMAND kivo_decoded_audio_frame_converter_test)

add_executable(kivo_wasapi_shared_pcm_writer_test
  tests/audio_output/wasapi_shared_pcm_writer_test.cpp
)
target_link_libraries(kivo_wasapi_shared_pcm_writer_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_wasapi_shared_pcm_writer_test COMMAND kivo_wasapi_shared_pcm_writer_test)

# ─── V10: Clock AV sync test ───

add_executable(kivo_clock_av_sync_test
  tests/clock/clock_av_sync_test.cpp
)
target_link_libraries(kivo_clock_av_sync_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_clock_av_sync_test COMMAND kivo_clock_av_sync_test)

# ─── V10-013: Seek / Flush / Generation Isolation ───

add_executable(kivo_seek_flush_generation_test
  tests/seek/seek_flush_generation_test.cpp
)
target_link_libraries(kivo_seek_flush_generation_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_seek_flush_generation_test COMMAND kivo_seek_flush_generation_test)

# ─── V10-014: Pause / Resume / Stop / Close Lifecycle ───

add_executable(kivo_lifecycle_test
  tests/presenter/lifecycle_test.cpp
)
target_link_libraries(kivo_lifecycle_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_lifecycle_test COMMAND kivo_lifecycle_test)

# ─── V10-015: Threading / COM / Device Ownership Proof ───

add_executable(kivo_threading_ownership_test
  tests/presenter/threading_ownership_test.cpp
)
target_link_libraries(kivo_threading_ownership_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_threading_ownership_test COMMAND kivo_threading_ownership_test)

# ─── V10-016: Inspector / DecisionLedger / ErrorMapping Runtime Proof ───

add_executable(kivo_inspector_decision_ledger_test
  tests/presenter/inspector_decision_ledger_test.cpp
)
target_link_libraries(kivo_inspector_decision_ledger_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_inspector_decision_ledger_test COMMAND kivo_inspector_decision_ledger_test)

# ─── V10-017: Machine-Readable Evidence Schema ───

add_executable(kivo_evidence_schema_test
  tests/presenter/evidence_schema_test.cpp
)
target_link_libraries(kivo_evidence_schema_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_evidence_schema_test COMMAND kivo_evidence_schema_test)

# ─── V10-018: Failure Injection Coverage ───

add_executable(kivo_failure_injection_test
  tests/presenter/failure_injection_test.cpp
)
target_link_libraries(kivo_failure_injection_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_failure_injection_test COMMAND kivo_failure_injection_test)

# ─── V10-019: Sample Gate ───

add_executable(kivo_sample_gate_test
  tests/presenter/sample_gate_test.cpp
)
target_link_libraries(kivo_sample_gate_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_sample_gate_test COMMAND kivo_sample_gate_test)

# ─── V10-020: Local Minimal Playback End-To-End Gate ───

add_executable(kivo_local_playback_e2e_test
  tests/presenter/local_playback_e2e_test.cpp
)
target_include_directories(kivo_local_playback_e2e_test PRIVATE tests/support)
target_link_libraries(kivo_local_playback_e2e_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_local_playback_e2e_test COMMAND kivo_local_playback_e2e_test)
set_tests_properties(kivo_local_playback_e2e_test PROPERTIES
  ENVIRONMENT "KIVO_SAMPLE_H264_AAC_MP4=$ENV{KIVO_SAMPLE_H264_AAC_MP4}"
  SKIP_REGULAR_EXPRESSION
  "Classification: (BLOCKED_ENV|BLOCKED_SAMPLE_TOO_SHORT|BLOCKED_SAMPLE_INVALID)"
)

# ─── V10-021: 30s Local Playback Micro Soak Gate ───

add_executable(kivo_micro_soak_test
  tests/presenter/micro_soak_test.cpp
)
target_include_directories(kivo_micro_soak_test PRIVATE tests/support)
target_link_libraries(kivo_micro_soak_test PRIVATE kivo_cinema_engine)

add_test(NAME kivo_micro_soak_test COMMAND kivo_micro_soak_test)
set_tests_properties(kivo_micro_soak_test PROPERTIES
  ENVIRONMENT "KIVO_SAMPLE_H264_AAC_MP4=$ENV{KIVO_SAMPLE_H264_AAC_MP4}"
  SKIP_REGULAR_EXPRESSION
  "Classification: (BLOCKED_ENV|BLOCKED_SAMPLE_TOO_SHORT|BLOCKED_SAMPLE_INVALID)"
)

# ─── P2 Source Core Skeleton Targets ───

# Contract contracts (header-only INTERFACE)
add_library(kivo_video_source_core_contracts INTERFACE)
target_include_directories(kivo_video_source_core_contracts INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Runtime contracts (header-only INTERFACE, depends on contracts)
add_library(kivo_video_source_core_runtime INTERFACE)
target_include_directories(kivo_video_source_core_runtime INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(kivo_video_source_core_runtime INTERFACE kivo_video_source_core_contracts)

# Contract tests — one executable per family
set(SC_CONTRACT_TESTS
  provider_kind
  uri
  identity
  capability
  auth
  redaction
  error
  evidence
  decision_input
)
foreach(test_family ${SC_CONTRACT_TESTS})
  add_executable(kivo_video_source_core_contract_${test_family}_test
    tests/source_core/contracts/${test_family}/${test_family}_tests.cpp
  )
  target_include_directories(kivo_video_source_core_contract_${test_family}_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_compile_options(kivo_video_source_core_contract_${test_family}_test PRIVATE /std:c++17)
  target_link_libraries(kivo_video_source_core_contract_${test_family}_test PRIVATE kivo_video_source_core_contracts)
  add_test(NAME kivo_video_source_core_contract_${test_family}_test
    COMMAND kivo_video_source_core_contract_${test_family}_test
  )
  set_tests_properties(kivo_video_source_core_contract_${test_family}_test PROPERTIES TIMEOUT 10)
endforeach()

# Runtime tests — one executable per family
set(SC_RUNTIME_TESTS session open read seek cancel timeout error)
foreach(test_family ${SC_RUNTIME_TESTS})
  add_executable(kivo_video_source_core_runtime_${test_family}_test
    tests/source_core/runtime/${test_family}/${test_family}_tests.cpp
  )
  target_include_directories(kivo_video_source_core_runtime_${test_family}_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_compile_options(kivo_video_source_core_runtime_${test_family}_test PRIVATE /std:c++17)
  target_link_libraries(kivo_video_source_core_runtime_${test_family}_test PRIVATE kivo_video_source_core_runtime)
  add_test(NAME kivo_video_source_core_runtime_${test_family}_test
    COMMAND kivo_video_source_core_runtime_${test_family}_test
  )
  set_tests_properties(kivo_video_source_core_runtime_${test_family}_test PROPERTIES TIMEOUT 10)
endforeach()

# Provider placeholder INTERFACE targets (no runtime, no source files)
# local_file has been promoted to real implementation
foreach(provider http webdav alist emby cloud_drive hls)
  add_library(kivo_video_providers_${provider} INTERFACE)
  target_include_directories(kivo_video_providers_${provider} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(kivo_video_providers_${provider} INTERFACE
    kivo_video_source_core_contracts
  )
endforeach()

# ─── Local File Provider (real implementation) ───
add_library(kivo_video_provider_local_file STATIC
  src/providers/local_file/local_file_path_validation.cpp
  src/providers/local_file/local_file_opaque_digest.cpp
  src/providers/local_file/local_file_redaction.cpp
  src/providers/local_file/local_file_session_store.cpp
  src/providers/local_file/local_file_provider.cpp
)
target_include_directories(kivo_video_provider_local_file PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(kivo_video_provider_local_file PRIVATE /std:c++17)
target_link_libraries(kivo_video_provider_local_file PUBLIC
  kivo_video_source_core_runtime
)

# ─── Local File Provider Tests ───
set(LF_TESTS open read zero_read seek error close redaction evidence direct_input path_validation schema basic_thread_safety)
foreach(test_name ${LF_TESTS})
  add_executable(kivo_video_provider_local_file_${test_name}_test
    tests/providers/local_file/${test_name}_test.cpp
  )
  target_include_directories(kivo_video_provider_local_file_${test_name}_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_compile_options(kivo_video_provider_local_file_${test_name}_test PRIVATE /std:c++17)
  target_link_libraries(kivo_video_provider_local_file_${test_name}_test PRIVATE
    kivo_video_provider_local_file
  )
  add_test(NAME kivo_video_provider_local_file_${test_name}_test
    COMMAND kivo_video_provider_local_file_${test_name}_test
  )
  set_tests_properties(kivo_video_provider_local_file_${test_name}_test PROPERTIES TIMEOUT 10)
endforeach()

# ─── HTTP Range Provider ───
add_library(kivo_video_provider_http_range STATIC
  src/providers/http/http_url.cpp
  src/providers/http/content_range_parser.cpp
  src/providers/http/http_range_provider.cpp
)
target_include_directories(kivo_video_provider_http_range PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(kivo_video_provider_http_range PRIVATE /std:c++17)
target_link_libraries(kivo_video_provider_http_range PUBLIC kivo_video_source_core_runtime)

set(HR_TESTS open read seek close redaction direct_input basic_thread_safety empty_416 no_range unknown_length one_byte zero_read evidence schema redirect auth rate_limit compressed_range source_changed validator body_cap probe access_ref cancel timeout positioned_read concurrent_read extended_probe probe_ambiguous cancel_mid_chunk rapid_close connection_reuse redirect_limit connection_key auth_isolation tombstone_clear opaque_validator secret_query_redaction tombstone_sensitive_clear)
foreach(test_name ${HR_TESTS})
  add_executable(kivo_video_provider_http_range_${test_name}_test
    tests/providers/http/${test_name}_test.cpp
  )
  target_include_directories(kivo_video_provider_http_range_${test_name}_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_compile_options(kivo_video_provider_http_range_${test_name}_test PRIVATE /std:c++17)
  target_link_libraries(kivo_video_provider_http_range_${test_name}_test PRIVATE kivo_video_provider_http_range)
  add_test(NAME kivo_video_provider_http_range_${test_name}_test
    COMMAND kivo_video_provider_http_range_${test_name}_test
  )
  set_tests_properties(kivo_video_provider_http_range_${test_name}_test PROPERTIES TIMEOUT 10)
endforeach()

# ─── WebDAV Bridge ───
add_library(kivo_video_provider_webdav STATIC
  src/providers/webdav/webdav_bridge.cpp
)
target_include_directories(kivo_video_provider_webdav PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(kivo_video_provider_webdav PRIVATE /std:c++17)
target_link_libraries(kivo_video_provider_webdav PUBLIC kivo_video_provider_http_range)

set(WD_TESTS open read seek close redaction direct_input propfind collection auth href_match propfind_malformed large_xml_cap propfind_fallback positioned_read auth_redaction)
foreach(test_name ${WD_TESTS})
  add_executable(kivo_video_provider_webdav_${test_name}_test
    tests/providers/webdav/${test_name}_test.cpp
  )
  target_include_directories(kivo_video_provider_webdav_${test_name}_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_compile_options(kivo_video_provider_webdav_${test_name}_test PRIVATE /std:c++17)
  target_link_libraries(kivo_video_provider_webdav_${test_name}_test PRIVATE kivo_video_provider_webdav)
  add_test(NAME kivo_video_provider_webdav_${test_name}_test
    COMMAND kivo_video_provider_webdav_${test_name}_test
  )
  set_tests_properties(kivo_video_provider_webdav_${test_name}_test PROPERTIES TIMEOUT 10)
endforeach()

# ─── No Debug Dialog Gate (Remote Range) ───
add_executable(kivo_no_debug_dialog_gate_test
  tests/providers/no_debug_dialog_test.cpp
)
target_include_directories(kivo_no_debug_dialog_gate_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_options(kivo_no_debug_dialog_gate_test PRIVATE /std:c++17)
target_link_libraries(kivo_no_debug_dialog_gate_test PRIVATE kivo_video_provider_webdav)
add_test(NAME kivo_no_debug_dialog_gate_test COMMAND kivo_no_debug_dialog_gate_test)
set_tests_properties(kivo_no_debug_dialog_gate_test PROPERTIES TIMEOUT 10)

# ─── Malicious Evidence JSON Test ───
add_executable(kivo_malicious_evidence_test
  tests/evidence/malicious_evidence_test.cpp
)
target_include_directories(kivo_malicious_evidence_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(kivo_malicious_evidence_test PRIVATE /std:c++17)
add_test(NAME kivo_malicious_evidence_test COMMAND kivo_malicious_evidence_test)
set_tests_properties(kivo_malicious_evidence_test PROPERTIES TIMEOUT 10)

# ─── Redaction Sentinel Test ───
add_executable(kivo_redaction_sentinel_test
  tests/providers/redaction_sentinel_test.cpp
)
target_include_directories(kivo_redaction_sentinel_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_options(kivo_redaction_sentinel_test PRIVATE /std:c++17)
target_link_libraries(kivo_redaction_sentinel_test PRIVATE kivo_video_provider_webdav)
add_test(NAME kivo_redaction_sentinel_test COMMAND kivo_redaction_sentinel_test)
set_tests_properties(kivo_redaction_sentinel_test PROPERTIES TIMEOUT 10)

# ─── Evidence JSON Validator Test ───
add_executable(kivo_evidence_json_validator_test
  tests/evidence/evidence_json_validator_test.cpp
)
target_include_directories(kivo_evidence_json_validator_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(kivo_evidence_json_validator_test PRIVATE /std:c++17)
add_test(NAME kivo_evidence_json_validator_test COMMAND kivo_evidence_json_validator_test)
set_tests_properties(kivo_evidence_json_validator_test PROPERTIES TIMEOUT 10)

# ─── P2 Schema Gate Validator ───
find_program(PYTHON_EXE python)
if(PYTHON_EXE)
  add_test(NAME kivo_schema_gate_self_test
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/governance/schema_gate/validator/schema_gate_validator.py" --self-test
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
  add_test(NAME kivo_redline_gate_self_test
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/governance/redline_gate/redline_checker.py" --self-test
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
  add_test(NAME kivo_redline_gate_production_scan
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/governance/redline_gate/redline_checker.py" --root=${CMAKE_SOURCE_DIR} --scan=all
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()

# ─── P3 Contract Baseline (M0) ───
add_library(kivo_p3_contracts INTERFACE)
target_include_directories(kivo_p3_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_contracts INTERFACE cxx_std_20)

# ─── P3 Execution Family Tests ───
add_executable(kivo_p3_execution_family_test tests/p3/execution/execution_family_tests.cpp)
target_include_directories(kivo_p3_execution_family_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_execution_family_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_execution_family_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_execution_family_test COMMAND kivo_p3_execution_family_test)
set_tests_properties(kivo_p3_execution_family_test PROPERTIES TIMEOUT 10)

# ─── P3 Source Model + Platform Contracts Tests ───
add_executable(kivo_p3_source_model_platform_test tests/p3/source_model/source_model_platform_tests.cpp)
target_include_directories(kivo_p3_source_model_platform_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_source_model_platform_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_source_model_platform_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_source_model_platform_test COMMAND kivo_p3_source_model_platform_test)
set_tests_properties(kivo_p3_source_model_platform_test PROPERTIES TIMEOUT 10)

# ─── P3 Full Contracts Tests (M1: provider/sessions/security/delivery/probe/decision/handoff/evidence/unsupported) ───
add_executable(kivo_p3_contracts_test tests/p3/contracts/p3_contracts_tests.cpp)
target_include_directories(kivo_p3_contracts_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_contracts_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_contracts_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_contracts_test COMMAND kivo_p3_contracts_test)
set_tests_properties(kivo_p3_contracts_test PROPERTIES TIMEOUT 10)

# ─── P3 Fake Provider Library ───
add_library(kivo_p3_fake_providers STATIC
  src/p3/providers/fake/fake_providers.cpp
)
target_include_directories(kivo_p3_fake_providers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_fake_providers PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_fake_providers PUBLIC kivo_p3_contracts)

# ─── P3 Behavior Tests (FC-011/035/036/039/044/047/048/049/052/066/069/070) ───
add_executable(kivo_p3_behavior_test tests/p3/behavior/p3_behavior_tests.cpp)
target_include_directories(kivo_p3_behavior_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_behavior_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_behavior_test PRIVATE kivo_p3_fake_providers)
add_test(NAME kivo_p3_behavior_test COMMAND kivo_p3_behavior_test)
set_tests_properties(kivo_p3_behavior_test PROPERTIES TIMEOUT 15)

# ─── P3 Architecture Guard Scanner Test ───
find_program(PYTHON_EXE python)
if(PYTHON_EXE)
  add_test(NAME kivo_p3_architecture_guard_scan
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/p3/architecture_guard_scanner.py" --root=${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
  set_tests_properties(kivo_p3_architecture_guard_scan PROPERTIES TIMEOUT 15)
endif()
