option(KIVO_ENABLE_FFMPEG "Enable FFmpeg-based real media processing" OFF)
option(KIVO_ENABLE_REAL_MEDIA_TESTS "Enable integration tests requiring real media samples" OFF)

include("${CMAKE_SOURCE_DIR}/cmake/ffmpeg_runtime_deploy.cmake")

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
  src/third_party_adapter/ffmpeg_adapter/ffmpeg_custom_io.cpp
  src/third_party_adapter/ffmpeg_adapter/ffmpeg_product_log_filter.cpp
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
    target_link_libraries(kivo_cinema_engine PUBLIC avformat avcodec avutil swresample swscale)
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
kivo_add_ffmpeg_runtime_deploy(kivo_real_probe_runtime_test)

add_executable(kivo_real_demux_runtime_test
  tests/demux_core/real_demux_runtime_test.cpp
)
target_link_libraries(kivo_real_demux_runtime_test PRIVATE kivo_cinema_engine)
kivo_add_ffmpeg_runtime_deploy(kivo_real_demux_runtime_test)

add_executable(kivo_real_decode_runtime_test
  tests/decode_core/real_software_decode_runtime_test.cpp
)
target_link_libraries(kivo_real_decode_runtime_test PRIVATE kivo_cinema_engine)
kivo_add_ffmpeg_runtime_deploy(kivo_real_decode_runtime_test)

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
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/governance/redline_gate/redline_checker.py" --root=${CMAKE_SOURCE_DIR} --scan=delivery
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()

# ─── P5A: Video Plane Contracts (INTERFACE library, 121+ headers) ───

# --- P3 Source Runtime -> Playback-Ready Input contracts ---
add_library(kivo_p3_contracts INTERFACE)
target_include_directories(kivo_p3_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_contracts INTERFACE cxx_std_20)

add_executable(kivo_p3_execution_family_test
  tests/p3/execution/execution_family_tests.cpp
)
target_include_directories(kivo_p3_execution_family_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_execution_family_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_execution_family_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_execution_family_test COMMAND kivo_p3_execution_family_test)
set_tests_properties(kivo_p3_execution_family_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p3_source_model_platform_test
  tests/p3/source_model/source_model_platform_tests.cpp
)
target_include_directories(kivo_p3_source_model_platform_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_source_model_platform_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_source_model_platform_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_source_model_platform_test COMMAND kivo_p3_source_model_platform_test)
set_tests_properties(kivo_p3_source_model_platform_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p3_contracts_test
  tests/p3/contracts/p3_contracts_tests.cpp
)
target_include_directories(kivo_p3_contracts_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_contracts_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_contracts_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_contracts_test COMMAND kivo_p3_contracts_test)
set_tests_properties(kivo_p3_contracts_test PROPERTIES TIMEOUT 10)

add_library(kivo_p3_fake_providers STATIC
  src/p3/providers/fake/fake_providers.cpp
)
target_include_directories(kivo_p3_fake_providers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_fake_providers PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_fake_providers PUBLIC kivo_p3_contracts)

add_executable(kivo_p3_behavior_test
  tests/p3/behavior/p3_behavior_tests.cpp
)
target_include_directories(kivo_p3_behavior_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_behavior_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_behavior_test PRIVATE kivo_p3_fake_providers)
add_test(NAME kivo_p3_behavior_test COMMAND kivo_p3_behavior_test)
set_tests_properties(kivo_p3_behavior_test PROPERTIES TIMEOUT 15)

add_executable(kivo_p3_acceptance_test
  tests/p3/acceptance/p3_acceptance_tests.cpp
)
target_include_directories(kivo_p3_acceptance_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_acceptance_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_acceptance_test PRIVATE kivo_p3_fake_providers)
add_test(NAME kivo_p3_acceptance_test COMMAND kivo_p3_acceptance_test)
set_tests_properties(kivo_p3_acceptance_test PROPERTIES TIMEOUT 15)

add_executable(kivo_p3_completion_test
  tests/p3/completion/p3_completion_tests.cpp
)
target_include_directories(kivo_p3_completion_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p3_completion_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p3_completion_test PRIVATE kivo_p3_contracts)
add_test(NAME kivo_p3_completion_test COMMAND kivo_p3_completion_test)
set_tests_properties(kivo_p3_completion_test PROPERTIES TIMEOUT 15)

if(PYTHON_EXE)
  add_test(NAME kivo_p3_architecture_guard_scan
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/p3/architecture_guard_scanner.py" --root=${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
  set_tests_properties(kivo_p3_architecture_guard_scan PROPERTIES TIMEOUT 15)
endif()

# --- P4 Playback Control Plane contracts ---
add_library(kivo_p4_contracts INTERFACE)
target_include_directories(kivo_p4_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_contracts INTERFACE cxx_std_20)

add_executable(kivo_p4_primitives_revision_test
  tests/playback/control_plane/primitives/p4_primitives_revision_tests.cpp
)
target_include_directories(kivo_p4_primitives_revision_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_primitives_revision_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_primitives_revision_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_primitives_revision_test COMMAND kivo_p4_primitives_revision_test)
set_tests_properties(kivo_p4_primitives_revision_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_revision_control_test
  tests/playback/control_plane/revision/p4_revision_control_tests.cpp
)
target_include_directories(kivo_p4_revision_control_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_revision_control_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_revision_control_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_revision_control_test COMMAND kivo_p4_revision_control_test)
set_tests_properties(kivo_p4_revision_control_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_reader_memory_buffer_test
  tests/playback/control_plane/reader/p4_reader_memory_buffer_tests.cpp
)
target_include_directories(kivo_p4_reader_memory_buffer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_reader_memory_buffer_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_reader_memory_buffer_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_reader_memory_buffer_test COMMAND kivo_p4_reader_memory_buffer_test)
set_tests_properties(kivo_p4_reader_memory_buffer_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_feedback_clock_track_test
  tests/playback/control_plane/feedback/p4_feedback_clock_track_tests.cpp
)
target_include_directories(kivo_p4_feedback_clock_track_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_feedback_clock_track_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_feedback_clock_track_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_feedback_clock_track_test COMMAND kivo_p4_feedback_clock_track_test)
set_tests_properties(kivo_p4_feedback_clock_track_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_recovery_events_diagnostics_test
  tests/playback/control_plane/recovery/p4_recovery_events_diagnostics_tests.cpp
)
target_include_directories(kivo_p4_recovery_events_diagnostics_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_recovery_events_diagnostics_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_recovery_events_diagnostics_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_recovery_events_diagnostics_test COMMAND kivo_p4_recovery_events_diagnostics_test)
set_tests_properties(kivo_p4_recovery_events_diagnostics_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_output_boundary_test
  tests/playback/control_plane/output_boundary/p4_output_boundary_tests.cpp
)
target_include_directories(kivo_p4_output_boundary_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_output_boundary_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_output_boundary_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_output_boundary_test COMMAND kivo_p4_output_boundary_test)
set_tests_properties(kivo_p4_output_boundary_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_negative_compile_test
  tests/playback/control_plane/compile_fail/p4_negative_compile_tests.cpp
)
target_include_directories(kivo_p4_negative_compile_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_negative_compile_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_negative_compile_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_negative_compile_test COMMAND kivo_p4_negative_compile_test)
set_tests_properties(kivo_p4_negative_compile_test PROPERTIES TIMEOUT 10)

add_executable(kivo_p4_behavior_test
  tests/playback/control_plane/p4_behavior_tests.cpp
)
target_include_directories(kivo_p4_behavior_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kivo_p4_behavior_test PRIVATE cxx_std_20)
target_link_libraries(kivo_p4_behavior_test PRIVATE kivo_p4_contracts)
add_test(NAME kivo_p4_behavior_test COMMAND kivo_p4_behavior_test)
set_tests_properties(kivo_p4_behavior_test PROPERTIES TIMEOUT 10)

if(PYTHON_EXE)
  add_test(NAME kivo_p4_architecture_guard_scan
    COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/backend/tools/p4/architecture_guard_scanner.py" "${CMAKE_SOURCE_DIR}/backend/include/kivo/playback/control_plane"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
  set_tests_properties(kivo_p4_architecture_guard_scan PROPERTIES TIMEOUT 15)
endif()

add_library(kivo_video_plane_contracts INTERFACE)
target_include_directories(kivo_video_plane_contracts INTERFACE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_contracts INTERFACE cxx_std_20)
# P5A Link Rule 1: contracts links NOTHING
# No target_link_libraries call — pure header-only contracts

# ─── P5A: Video Plane Contract Tests ───

add_executable(kivo_video_plane_contract_tests
  tests/video_plane/p5a_contract_tests.cpp
)
target_include_directories(kivo_video_plane_contract_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_contract_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_contract_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_contract_tests COMMAND kivo_video_plane_contract_tests)
set_tests_properties(kivo_video_plane_contract_tests PROPERTIES TIMEOUT 30)

# --- P5B: P4 Input Bridge Tests ---

add_executable(kivo_video_plane_input_bridge_tests
  tests/video_plane/p5b_input_bridge_tests.cpp
)
target_include_directories(kivo_video_plane_input_bridge_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_input_bridge_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_input_bridge_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_input_bridge_tests COMMAND kivo_video_plane_input_bridge_tests)
set_tests_properties(kivo_video_plane_input_bridge_tests PROPERTIES TIMEOUT 30)

# --- P5C: FFmpeg Demux Backend Tests ---

add_executable(kivo_video_plane_ffmpeg_demux_tests
  tests/video_plane/p5c_ffmpeg_demux_tests.cpp
)
target_include_directories(kivo_video_plane_ffmpeg_demux_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_ffmpeg_demux_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_ffmpeg_demux_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_ffmpeg_demux_tests COMMAND kivo_video_plane_ffmpeg_demux_tests)
set_tests_properties(kivo_video_plane_ffmpeg_demux_tests PROPERTIES TIMEOUT 30)

# --- P5D: Decoder Contract Tests ---

add_executable(kivo_video_plane_decoder_tests
  tests/video_plane/p5d_decoder_tests.cpp
)
target_include_directories(kivo_video_plane_decoder_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_decoder_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_decoder_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_decoder_tests COMMAND kivo_video_plane_decoder_tests)
set_tests_properties(kivo_video_plane_decoder_tests PROPERTIES TIMEOUT 30)

# --- P5E: Software Decode / Timing Tests ---

add_executable(kivo_video_plane_timing_tests
  tests/video_plane/p5e_timing_tests.cpp
)
target_include_directories(kivo_video_plane_timing_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/tests/video_plane
)
target_compile_features(kivo_video_plane_timing_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_timing_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_timing_tests COMMAND kivo_video_plane_timing_tests)
set_tests_properties(kivo_video_plane_timing_tests PROPERTIES TIMEOUT 30)

# --- P5F: GPU Rendering Tests ---

add_executable(kivo_video_plane_gpu_render_tests
  tests/video_plane/p5f_gpu_render_tests.cpp
)
target_include_directories(kivo_video_plane_gpu_render_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_gpu_render_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_gpu_render_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_gpu_render_tests COMMAND kivo_video_plane_gpu_render_tests)
set_tests_properties(kivo_video_plane_gpu_render_tests PROPERTIES TIMEOUT 30)

# --- P5G: D3D11VA / P010 / GPU Tests ---

add_executable(kivo_video_plane_d3d11va_p010_tests
  tests/video_plane/p5g_d3d11va_p010_tests.cpp
)
target_include_directories(kivo_video_plane_d3d11va_p010_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_d3d11va_p010_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_d3d11va_p010_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_d3d11va_p010_tests COMMAND kivo_video_plane_d3d11va_p010_tests)
set_tests_properties(kivo_video_plane_d3d11va_p010_tests PROPERTIES TIMEOUT 30)

# --- P5H: Color/HDR/Protected Closure Tests ---

add_executable(kivo_video_plane_color_hdr_protected_tests
  tests/video_plane/p5h_color_hdr_protected_tests.cpp
)
target_include_directories(kivo_video_plane_color_hdr_protected_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_color_hdr_protected_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_color_hdr_protected_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_color_hdr_protected_tests COMMAND kivo_video_plane_color_hdr_protected_tests)
set_tests_properties(kivo_video_plane_color_hdr_protected_tests PROPERTIES TIMEOUT 30)

# --- P5S: Security Closure Tests ---

add_executable(kivo_video_plane_security_closure_tests
  tests/video_plane/p5s_security_closure_tests.cpp
)
target_include_directories(kivo_video_plane_security_closure_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_video_plane_security_closure_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_video_plane_security_closure_tests PRIVATE kivo_video_plane_contracts)

add_test(NAME kivo_video_plane_security_closure_tests COMMAND kivo_video_plane_security_closure_tests)
set_tests_properties(kivo_video_plane_security_closure_tests PROPERTIES TIMEOUT 30)

# ─── P6A: Audio Plane Contracts (INTERFACE library, 294 headers, 30 subdirs) ───
# Per V2.0 §3.1: C++23 required. Per §5: public contracts link NOTHING.
# Per §42: no /wasapi/ /ffmpeg/ /qt/ in public include path.

add_library(kivo_audio_plane_contracts INTERFACE)
target_include_directories(kivo_audio_plane_contracts INTERFACE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_audio_plane_contracts INTERFACE cxx_std_23)
# P6A Link Rule: contracts links NOTHING (no WASAPI/FFmpeg/Qt/D3D11)
# P6A does NOT depend on kivo_video_plane_contracts (P5) or kivo_cinema_engine (legacy)

# ─── P6A: Audio Plane Contract Tests ───

add_executable(kivo_audio_plane_contract_tests
  tests/video/audio_plane/p6a_contract_tests.cpp
)
target_include_directories(kivo_audio_plane_contract_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/tests/video/audio_plane
)
target_compile_features(kivo_audio_plane_contract_tests PRIVATE cxx_std_23)
target_link_libraries(kivo_audio_plane_contract_tests PRIVATE kivo_audio_plane_contracts)

add_test(NAME kivo_audio_plane_contract_tests COMMAND kivo_audio_plane_contract_tests)
set_tests_properties(kivo_audio_plane_contract_tests PROPERTIES TIMEOUT 30)

# ─── P6A: Architecture Guard (PublicHeadersStrict mode, §42) ───
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME p6_audio_arch_guard_public_headers_strict
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/backend/tools/governance/p6_audio_arch_guard.py
            --repo-root ${CMAKE_SOURCE_DIR}
            --mode PublicHeadersStrict
  )
  set_tests_properties(p6_audio_arch_guard_public_headers_strict
    PROPERTIES TIMEOUT 30
  )
endif()

# ─── P6G-039: P6A Negative Fixtures (compile-fail tests, §42/§44) ───
add_subdirectory(tests/video/audio_plane/negative)

# ─── P6B: Fake Runtime Tests (header-only fake runtime, links P6A contracts) ───
set(P6B_TEST_SOURCES
  import_bridge_tests
  data_handle_tests
  pool_tests
  generation_tests
  time_tests
  evidence_tests
  decode_tests
  render_tests
  passthrough_tests
)
foreach(src ${P6B_TEST_SOURCES})
  add_executable(kivo_p6b_${src}
    tests/video/audio_plane/p6b_runtime_tests/${src}.cpp
  )
  target_include_directories(kivo_p6b_${src} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/backend/tests/video/audio_plane
  )
  target_compile_features(kivo_p6b_${src} PRIVATE cxx_std_23)
  target_link_libraries(kivo_p6b_${src} PRIVATE kivo_audio_plane_contracts)
  add_test(NAME kivo_p6b_${src} COMMAND kivo_p6b_${src})
  set_tests_properties(kivo_p6b_${src} PROPERTIES TIMEOUT 30)
endforeach()

# ─── P6C: FFmpeg Decode Backend (avcodec-only, guarded by KIVO_ENABLE_FFMPEG) ───
# Per P6C Planning Draft REV 001 §8.1: FFmpeg absent = WARNING + skip, full repo builds normally
if(KIVO_ENABLE_FFMPEG)
  if(NOT FFMPEG_ROOT)
    message(WARNING "KIVO_ENABLE_FFMPEG=ON but FFMPEG_ROOT is not set. P6C tests skipped. Full repo build continues.")
  elseif(NOT EXISTS "${FFMPEG_ROOT}/include/libavcodec/avcodec.h")
    message(WARNING "KIVO_ENABLE_FFMPEG=ON but FFmpeg SDK not found at ${FFMPEG_ROOT}. P6C tests skipped. Full repo build continues.")
  else()
    # P6C shared source files
    set(KIVO_P6C_SOURCES
      src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_backend.cpp
      src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_lifecycle.cpp
      src/video/audio_plane/decode/ffmpeg/ffmpeg_frame_converter.cpp
      src/video/audio_plane/decode/ffmpeg/ffmpeg_codec_private_data.cpp
      src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_evidence_emitter.cpp
    )
    set(KIVO_P6C_INCLUDES
      ${CMAKE_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/backend/include_private
      ${CMAKE_SOURCE_DIR}/backend/src/video/audio_plane/decode/ffmpeg/internal
      ${FFMPEG_ROOT}/include
    )
    set(KIVO_P6C_LIBS
      kivo_audio_plane_contracts
      ${FFMPEG_ROOT}/lib/avcodec.lib
      ${FFMPEG_ROOT}/lib/avutil.lib
    )

    # P6C Test 1: Backend lifecycle + decode (C1-C8, C11, transitions)
    add_executable(kivo_p6c_decode_backend_tests
      tests/video/audio_plane/p6c_ffmpeg_decode_tests/ffmpeg_decode_backend_tests.cpp
      ${KIVO_P6C_SOURCES}
    )
    target_include_directories(kivo_p6c_decode_backend_tests PRIVATE ${KIVO_P6C_INCLUDES})
    target_compile_features(kivo_p6c_decode_backend_tests PRIVATE cxx_std_23)
    if(MSVC)
      target_compile_options(kivo_p6c_decode_backend_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(kivo_p6c_decode_backend_tests PRIVATE ${KIVO_P6C_LIBS})
    kivo_add_ffmpeg_runtime_deploy(kivo_p6c_decode_backend_tests)
    add_test(NAME kivo_p6c_decode_backend_tests COMMAND kivo_p6c_decode_backend_tests)
    set_tests_properties(kivo_p6c_decode_backend_tests PROPERTIES TIMEOUT 60 ENVIRONMENT "PATH=${FFMPEG_ROOT}/bin;%PATH%")

    # P6C Test 2: Frame converter (C9-C12)
    add_executable(kivo_p6c_frame_converter_tests
      tests/video/audio_plane/p6c_ffmpeg_decode_tests/ffmpeg_frame_converter_tests.cpp
      src/video/audio_plane/decode/ffmpeg/ffmpeg_frame_converter.cpp
    )
    target_include_directories(kivo_p6c_frame_converter_tests PRIVATE ${KIVO_P6C_INCLUDES})
    target_compile_features(kivo_p6c_frame_converter_tests PRIVATE cxx_std_23)
    if(MSVC)
      target_compile_options(kivo_p6c_frame_converter_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(kivo_p6c_frame_converter_tests PRIVATE ${KIVO_P6C_LIBS})
    kivo_add_ffmpeg_runtime_deploy(kivo_p6c_frame_converter_tests)
    add_test(NAME kivo_p6c_frame_converter_tests COMMAND kivo_p6c_frame_converter_tests)
    set_tests_properties(kivo_p6c_frame_converter_tests PROPERTIES TIMEOUT 60 ENVIRONMENT "PATH=${FFMPEG_ROOT}/bin;%PATH%")

    # P6C Test 3: Evidence emitter + state mapping (C13-C16)
    add_executable(kivo_p6c_evidence_tests
      tests/video/audio_plane/p6c_ffmpeg_decode_tests/ffmpeg_decode_evidence_tests.cpp
      src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_evidence_emitter.cpp
    )
    target_include_directories(kivo_p6c_evidence_tests PRIVATE ${KIVO_P6C_INCLUDES})
    target_compile_features(kivo_p6c_evidence_tests PRIVATE cxx_std_23)
    if(MSVC)
      target_compile_options(kivo_p6c_evidence_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(kivo_p6c_evidence_tests PRIVATE ${KIVO_P6C_LIBS})
    kivo_add_ffmpeg_runtime_deploy(kivo_p6c_evidence_tests)
    add_test(NAME kivo_p6c_evidence_tests COMMAND kivo_p6c_evidence_tests)
    set_tests_properties(kivo_p6c_evidence_tests PROPERTIES TIMEOUT 60 ENVIRONMENT "PATH=${FFMPEG_ROOT}/bin;%PATH%")
  endif()
endif()

# ─── P6D: WASAPI Output Backend (Windows-only, guarded by KIVO_ENABLE_WASAPI) ───
if(KIVO_ENABLE_WASAPI AND WIN32)
    set(KIVO_P6D_SOURCES
      src/video/audio_plane/output/wasapi/wasapi_output_backend.cpp
      src/video/audio_plane/output/wasapi/wasapi_output_lifecycle.cpp
      src/video/audio_plane/output/wasapi/wasapi_device_enumerator.cpp
      src/video/audio_plane/output/wasapi/wasapi_render_loop.cpp
      src/video/audio_plane/output/wasapi/wasapi_output_evidence_emitter.cpp
    )
    set(KIVO_P6D_INCLUDES
      ${CMAKE_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/backend/include_private
      ${CMAKE_SOURCE_DIR}/backend/src/video/audio_plane/output/wasapi/internal
    )
    set(KIVO_P6D_LIBS
      kivo_audio_plane_contracts
      ole32
      mmdevapi
    )

    # P6D Test 1: Backend lifecycle (D1-D8)
    add_executable(kivo_p6d_backend_tests
      tests/video/audio_plane/p6d_wasapi_output_tests/wasapi_output_backend_tests.cpp
      ${KIVO_P6D_SOURCES}
    )
    target_include_directories(kivo_p6d_backend_tests PRIVATE ${KIVO_P6D_INCLUDES})
    target_compile_features(kivo_p6d_backend_tests PRIVATE cxx_std_23)
    if(MSVC)
      target_compile_options(kivo_p6d_backend_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(kivo_p6d_backend_tests PRIVATE ${KIVO_P6D_LIBS})
    add_test(NAME kivo_p6d_backend_tests COMMAND kivo_p6d_backend_tests)
    set_tests_properties(kivo_p6d_backend_tests PROPERTIES TIMEOUT 60)

    # P6D Test 2: Render loop (D9-D12)
    add_executable(kivo_p6d_render_loop_tests
      tests/video/audio_plane/p6d_wasapi_output_tests/wasapi_render_loop_tests.cpp
      src/video/audio_plane/output/wasapi/wasapi_render_loop.cpp
    )
    target_include_directories(kivo_p6d_render_loop_tests PRIVATE ${KIVO_P6D_INCLUDES})
    target_compile_features(kivo_p6d_render_loop_tests PRIVATE cxx_std_23)
    if(MSVC)
      target_compile_options(kivo_p6d_render_loop_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(kivo_p6d_render_loop_tests PRIVATE ${KIVO_P6D_LIBS})
    add_test(NAME kivo_p6d_render_loop_tests COMMAND kivo_p6d_render_loop_tests)
    set_tests_properties(kivo_p6d_render_loop_tests PROPERTIES TIMEOUT 60)

    # P6D Test 3: Evidence emitter (D13-D15)
    add_executable(kivo_p6d_evidence_tests
      tests/video/audio_plane/p6d_wasapi_output_tests/wasapi_output_evidence_tests.cpp
      src/video/audio_plane/output/wasapi/wasapi_output_evidence_emitter.cpp
    )
    target_include_directories(kivo_p6d_evidence_tests PRIVATE ${KIVO_P6D_INCLUDES})
    target_compile_features(kivo_p6d_evidence_tests PRIVATE cxx_std_23)
    if(MSVC)
      target_compile_options(kivo_p6d_evidence_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(kivo_p6d_evidence_tests PRIVATE ${KIVO_P6D_LIBS})
    add_test(NAME kivo_p6d_evidence_tests COMMAND kivo_p6d_evidence_tests)
    set_tests_properties(kivo_p6d_evidence_tests PROPERTIES TIMEOUT 60)
endif()

# ─── P6E: Resampler / DSP / Clock / Sync Runtime ───
set(KIVO_P6E_INCLUDES
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)

# P6E Test 1: Resampler
add_executable(kivo_p6e_resampler_tests
  tests/video/audio_plane/p6e_runtime_tests/resampler_tests.cpp
  src/video/audio_plane/runtime/resampler/linear_resampler.cpp
)
target_include_directories(kivo_p6e_resampler_tests PRIVATE ${KIVO_P6E_INCLUDES})
target_compile_features(kivo_p6e_resampler_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p6e_resampler_tests PRIVATE /utf-8)
endif()
target_link_libraries(kivo_p6e_resampler_tests PRIVATE kivo_audio_plane_contracts)
add_test(NAME kivo_p6e_resampler_tests COMMAND kivo_p6e_resampler_tests)

# P6E Test 2: DSP
add_executable(kivo_p6e_dsp_tests
  tests/video/audio_plane/p6e_runtime_tests/dsp_tests.cpp
  src/video/audio_plane/runtime/dsp/dither_processor.cpp
)
target_include_directories(kivo_p6e_dsp_tests PRIVATE ${KIVO_P6E_INCLUDES})
target_compile_features(kivo_p6e_dsp_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p6e_dsp_tests PRIVATE /utf-8)
endif()
target_link_libraries(kivo_p6e_dsp_tests PRIVATE kivo_audio_plane_contracts)
add_test(NAME kivo_p6e_dsp_tests COMMAND kivo_p6e_dsp_tests)

# P6E Test 3: Clock + Sync
add_executable(kivo_p6e_clock_sync_tests
  tests/video/audio_plane/p6e_runtime_tests/clock_sync_tests.cpp
  src/video/audio_plane/runtime/clock/audio_clock.cpp
)
target_include_directories(kivo_p6e_clock_sync_tests PRIVATE ${KIVO_P6E_INCLUDES})
target_compile_features(kivo_p6e_clock_sync_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p6e_clock_sync_tests PRIVATE /utf-8)
endif()
target_link_libraries(kivo_p6e_clock_sync_tests PRIVATE kivo_audio_plane_contracts)
add_test(NAME kivo_p6e_clock_sync_tests COMMAND kivo_p6e_clock_sync_tests)

# P6E Test 4: SPSC (header-only template, no .cpp needed)
add_executable(kivo_p6e_spsc_tests
  tests/video/audio_plane/p6e_runtime_tests/spsc_tests.cpp
)
target_include_directories(kivo_p6e_spsc_tests PRIVATE ${KIVO_P6E_INCLUDES})
target_compile_features(kivo_p6e_spsc_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p6e_spsc_tests PRIVATE /utf-8)
endif()
target_link_libraries(kivo_p6e_spsc_tests PRIVATE kivo_audio_plane_contracts)
add_test(NAME kivo_p6e_spsc_tests COMMAND kivo_p6e_spsc_tests)

# P6E Test 5: Gap fill (drift + quantize + noise shaping + passthrough + gapless + device thread)
add_executable(kivo_p6e_gap_tests
  tests/video/audio_plane/p6e_runtime_tests/gap_tests.cpp
  src/video/audio_plane/runtime/dsp/quantize_processor.cpp
  src/video/audio_plane/runtime/passthrough/iec61937_parser.cpp
  src/video/audio_plane/runtime/device_thread/device_thread_runtime.cpp
)
target_include_directories(kivo_p6e_gap_tests PRIVATE ${KIVO_P6E_INCLUDES})
target_compile_features(kivo_p6e_gap_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p6e_gap_tests PRIVATE /utf-8)
endif()
target_link_libraries(kivo_p6e_gap_tests PRIVATE kivo_audio_plane_contracts)
add_test(NAME kivo_p6e_gap_tests COMMAND kivo_p6e_gap_tests)

# --- P6 Local PCM Runtime Bridge ---
add_library(kivo_audio_plane_local_pcm_runtime STATIC
  src/video/audio_plane/runtime/local_pcm/local_pcm_audio_runtime.cpp
)
target_include_directories(kivo_audio_plane_local_pcm_runtime PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_audio_plane_local_pcm_runtime PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(kivo_audio_plane_local_pcm_runtime PRIVATE /utf-8)
endif()
target_link_libraries(kivo_audio_plane_local_pcm_runtime PRIVATE kivo_cinema_engine)

# --- P2: Source Runtime local media bridge for real playback input ---
add_library(kivo_video_source_runtime_local_media STATIC
  src/video/source_runtime/local_media/local_media_directory_queue.cpp
  src/video/source_runtime/local_media/local_media_source_session.cpp
)
target_include_directories(kivo_video_source_runtime_local_media PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_source_runtime_local_media PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(kivo_video_source_runtime_local_media PRIVATE /utf-8)
endif()
target_link_libraries(kivo_video_source_runtime_local_media PRIVATE
  kivo_video_provider_local_file
)

add_executable(kivo_video_source_runtime_local_media_queue_tests
  tests/video/source_runtime/local_media/local_media_directory_queue_tests.cpp
)
target_include_directories(kivo_video_source_runtime_local_media_queue_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_source_runtime_local_media_queue_tests PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(kivo_video_source_runtime_local_media_queue_tests PRIVATE /utf-8)
endif()
target_link_libraries(kivo_video_source_runtime_local_media_queue_tests PRIVATE
  kivo_video_source_runtime_local_media
)
add_test(NAME kivo_video_source_runtime_local_media_queue_tests
  COMMAND kivo_video_source_runtime_local_media_queue_tests)

# --- P5: Video Plane local media production runtime ---
add_library(kivo_video_plane_local_media_runtime STATIC
  src/video/video_plane/runtime/local_media/local_video_first_frame_pipeline.cpp
  src/video/video_plane/runtime/local_media/local_video_d3d11_upload.cpp
  src/video/video_plane/runtime/local_media/local_video_runtime_start.cpp
  src/video/video_plane/runtime/local_media/local_video_runtime_pump.cpp
  src/video/video_plane/runtime/local_media/local_video_runtime_controls.cpp
  src/video/video_plane/runtime/local_media/local_video_rgba_converter.cpp
)
target_include_directories(kivo_video_plane_local_media_runtime PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_plane_local_media_runtime PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(kivo_video_plane_local_media_runtime PRIVATE /utf-8)
endif()
target_link_libraries(kivo_video_plane_local_media_runtime PRIVATE
  kivo_cinema_engine
  kivo_video_source_runtime_local_media
)

# --- P7: Playback Graph ---
add_library(kivo_video_playback_graph STATIC
  src/video/playback_graph/session/playback_session_facade.cpp
  src/video/playback_graph/session/playback_session_local_media_facade.cpp
  src/video/playback_graph/session/playback_session_runtime.cpp
  src/video/playback_graph/session/playback_session_snapshot_queries.cpp
  src/video/playback_graph/session/playback_session_subtitle_policy.cpp
  src/video/playback_graph/session/playback_session_audio_policy.cpp
  src/video/playback_graph/session/playback_session_settings_policy.cpp
  src/video/playback_graph/session/playback_session_window_screen_policy.cpp
  src/video/playback_graph/session/playback_session_playlist_policy.cpp
  src/video/playback_graph/session/playback_session_video_surface_binding.cpp
  src/video/playback_graph/session/playback_session_recovery.cpp
  src/video/playback_graph/session/playback_session_command_query.cpp
  src/video/playback_graph/session/playback_session_clock_sync.cpp
  src/video/playback_graph/session/playback_session_track_inventory.cpp
  src/video/playback_graph/session/playback_session_track_switch.cpp
  src/video/playback_graph/session/playback_session_subtitle_frame.cpp
  src/video/playback_graph/state/playback_graph_state_machine.cpp
  src/video/playback_graph/command/command_lifetime_registry.cpp
  src/video/playback_graph/command/command_conflict_matrix.cpp
  src/video/playback_graph/command/pending_transport_intent.cpp
  src/video/playback_graph/p4_bridge/p4_state_sync_adapter.cpp
  src/video/playback_graph/p4_bridge/p4_timeline_contract_bridge.cpp
  src/video/playback_graph/p8_bridge/subtitle_frame_p8_bridge.cpp
  src/video/playback_graph/coordination/recovery_replay_coordinator.cpp
  src/video/playback_graph/attachment/attachment_operation_registry.cpp
  src/video/playback_graph/generation/generation_stamp_publisher.cpp
  src/video/playback_graph/events/critical_event_mpsc_ring.cpp
  src/video/playback_graph/coordination/buffering_predicates.cpp
  src/video/playback_graph/coordination/seek_coordinator.cpp
  src/video/playback_graph/resources/resource_retire_recycle.cpp
  src/video/playback_graph/coordination/eos_drain_coordinator.cpp
  src/video/playback_graph/coordination/track_switch_coordinator.cpp
  src/video/playback_graph/recovery/recovery_replay_behavior.cpp
  src/video/playback_graph/coordination/av_sync_micro_adjust.cpp
  src/video/playback_graph/coordination/system_resume_coordinator.cpp
  src/video/playback_graph/coordination/protected_content_policy.cpp
  src/video/playback_graph/snapshot/snapshot_store.cpp
  src/video/playback_graph/observer/observer_registry.cpp
  src/video/playback_graph/evidence/critical_proof_ring.cpp
  src/video/playback_graph/metrics/graph_metrics_budget.cpp
  src/video/playback_graph/health/graph_health_monitor.cpp
  src/video/playback_graph/evidence/privacy_salt_lifecycle.cpp
  src/video/playback_graph/evidence/layered_error_taxonomy.cpp
  src/video/playback_graph/local_media/local_media_source_registry.cpp
  src/video/playback_graph/local_media/local_media_audio_dispatch.cpp
  src/video/playback_graph/local_media/local_media_audio_display_filter.cpp
  src/video/playback_graph/local_media/local_media_p3_handoff_bridge.cpp
  src/video/playback_graph/local_media/local_media_subtitle_display_filter.cpp
  src/video/playback_graph/local_media/local_media_continuous_start.cpp
  src/video/playback_graph/local_media/local_media_continuous_playback.cpp
  src/video/playback_graph/session/playback_session_local_media.cpp
  src/video/playback_graph/session/playback_session_local_media_open.cpp
  src/video/playback_graph/session/playback_session_local_media_continuous.cpp
)
target_include_directories(kivo_video_playback_graph
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_playback_graph PUBLIC cxx_std_20)
target_link_libraries(kivo_video_playback_graph PRIVATE
  kivo_video_provider_local_file
  kivo_p3_contracts
  kivo_p4_contracts
  kivo_video_source_runtime_local_media
  kivo_video_plane_local_media_runtime
  kivo_audio_plane_local_pcm_runtime
)
if(KIVO_ENABLE_FFMPEG)
  target_link_libraries(kivo_video_playback_graph PRIVATE kivo_cinema_engine)
  target_compile_definitions(kivo_video_playback_graph PRIVATE KIVO_ENABLE_FFMPEG)
  if(FFMPEG_ROOT)
    target_include_directories(kivo_video_playback_graph PRIVATE ${FFMPEG_ROOT}/include)
    target_link_directories(kivo_video_playback_graph PRIVATE ${FFMPEG_ROOT}/lib)
    target_link_libraries(kivo_video_playback_graph PRIVATE swscale)
  endif()
endif()

add_custom_target(kivo_video_playback_graph_contract_tool)
add_custom_target(kivo_video_playback_graph_architecture_guard)

add_executable(kivo_p7b_public_contract_tests
  tests/video/playback_graph/p7b_public_contract_tests.cpp
)
target_include_directories(kivo_p7b_public_contract_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p7b_public_contract_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7b_public_contract_tests PRIVATE kivo_video_playback_graph)

add_executable(kivo_p7c_command_state_tests
  tests/video/playback_graph/p7c_command_state_tests.cpp
)
target_include_directories(kivo_p7c_command_state_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p7c_command_state_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7c_command_state_tests PRIVATE kivo_video_playback_graph)

add_executable(kivo_p7_local_media_audio_display_filter_tests
  tests/video/playback_graph/p7_local_media_audio_display_filter_tests.cpp
)
target_include_directories(kivo_p7_local_media_audio_display_filter_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p7_local_media_audio_display_filter_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7_local_media_audio_display_filter_tests PRIVATE
  kivo_video_playback_graph
)

add_executable(kivo_p7_local_media_subtitle_display_filter_tests
  tests/video/playback_graph/p7_local_media_subtitle_display_filter_tests.cpp
)
target_include_directories(kivo_p7_local_media_subtitle_display_filter_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p7_local_media_subtitle_display_filter_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7_local_media_subtitle_display_filter_tests PRIVATE
  kivo_video_playback_graph
)

add_executable(kivo_p7d_async_generation_tests
  tests/video/playback_graph/p7d_async_generation_tests.cpp
)
target_include_directories(kivo_p7d_async_generation_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p7d_async_generation_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7d_async_generation_tests PRIVATE kivo_video_playback_graph)

add_executable(kivo_p7e_behavior_coordinator_tests
  tests/video/playback_graph/p7e_behavior_coordinator_tests.cpp
)
target_include_directories(kivo_p7e_behavior_coordinator_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p7e_behavior_coordinator_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7e_behavior_coordinator_tests PRIVATE kivo_video_playback_graph)

add_executable(kivo_p7f_snapshot_evidence_tests
  tests/video/playback_graph/p7f_snapshot_evidence_tests.cpp
)
target_include_directories(kivo_p7f_snapshot_evidence_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p7f_snapshot_evidence_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7f_snapshot_evidence_tests PRIVATE kivo_video_playback_graph)

add_executable(kivo_p7g_public_header_purist_tests
  tests/video/playback_graph/p7g_public_header_purist_tests.cpp
)
target_include_directories(kivo_p7g_public_header_purist_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p7g_public_header_purist_tests PRIVATE cxx_std_20)

add_executable(kivo_p7g_deterministic_harness_tests
  tests/video/playback_graph/p7g_deterministic_harness_tests.cpp
)
target_include_directories(kivo_p7g_deterministic_harness_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
  ${CMAKE_SOURCE_DIR}/backend/tests
)
target_compile_features(kivo_p7g_deterministic_harness_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7g_deterministic_harness_tests PRIVATE kivo_video_playback_graph)

add_executable(kivo_p7h_local_media_first_frame_tests
  tests/video/playback_graph/p7h_local_media_first_frame_tests.cpp
)
target_include_directories(kivo_p7h_local_media_first_frame_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p7h_local_media_first_frame_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p7h_local_media_first_frame_tests PRIVATE kivo_video_player_runtime_adapter)
if(KIVO_ENABLE_FFMPEG)
  target_compile_definitions(kivo_p7h_local_media_first_frame_tests PRIVATE KIVO_ENABLE_FFMPEG)
endif()
kivo_add_ffmpeg_runtime_deploy(kivo_p7h_local_media_first_frame_tests)

add_custom_target(kivo_video_playback_graph_tests
  DEPENDS
    kivo_p7b_public_contract_tests
    kivo_p7c_command_state_tests
    kivo_p7_local_media_audio_display_filter_tests
    kivo_p7_local_media_subtitle_display_filter_tests
    kivo_p7d_async_generation_tests
    kivo_p7e_behavior_coordinator_tests
    kivo_p7f_snapshot_evidence_tests
    kivo_p7g_public_header_purist_tests
    kivo_p7g_deterministic_harness_tests
    kivo_p7h_local_media_first_frame_tests
)
add_test(NAME kivo_p7b_public_contract_tests COMMAND kivo_p7b_public_contract_tests)
set_tests_properties(kivo_p7b_public_contract_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7c_command_state_tests COMMAND kivo_p7c_command_state_tests)
set_tests_properties(kivo_p7c_command_state_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7_local_media_audio_display_filter_tests
  COMMAND kivo_p7_local_media_audio_display_filter_tests)
set_tests_properties(kivo_p7_local_media_audio_display_filter_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7_local_media_subtitle_display_filter_tests
  COMMAND kivo_p7_local_media_subtitle_display_filter_tests)
set_tests_properties(kivo_p7_local_media_subtitle_display_filter_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7d_async_generation_tests COMMAND kivo_p7d_async_generation_tests)
set_tests_properties(kivo_p7d_async_generation_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7e_behavior_coordinator_tests COMMAND kivo_p7e_behavior_coordinator_tests)
set_tests_properties(kivo_p7e_behavior_coordinator_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7f_snapshot_evidence_tests COMMAND kivo_p7f_snapshot_evidence_tests)
set_tests_properties(kivo_p7f_snapshot_evidence_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7g_public_header_purist_tests COMMAND kivo_p7g_public_header_purist_tests)
set_tests_properties(kivo_p7g_public_header_purist_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7g_deterministic_harness_tests COMMAND kivo_p7g_deterministic_harness_tests)
set_tests_properties(kivo_p7g_deterministic_harness_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p7h_local_media_first_frame_tests COMMAND kivo_p7h_local_media_first_frame_tests)
set_tests_properties(kivo_p7h_local_media_first_frame_tests PROPERTIES TIMEOUT 60)

# --- PlayerRuntimeAdapter: QML-to-P7 boundary core ---
add_library(kivo_video_player_runtime_adapter STATIC
  src/video/player_runtime_adapter/core/player_runtime_adapter.cpp
  src/video/player_runtime_adapter/commands/adapter_lifecycle_commands.cpp
  src/video/player_runtime_adapter/commands/adapter_playback_commands.cpp
  src/video/player_runtime_adapter/commands/adapter_seek_commands.cpp
  src/video/player_runtime_adapter/commands/adapter_track_commands.cpp
  src/video/player_runtime_adapter/commands/adapter_policy_commands.cpp
  src/video/player_runtime_adapter/commands/adapter_local_media_commands.cpp
  src/video/player_runtime_adapter/diagnostics/adapter_diagnostics_commands.cpp
  src/video/player_runtime_adapter/p7_bridge/p7_command_result.cpp
  src/video/player_runtime_adapter/state/adapter_state_mapper.cpp
  src/video/player_runtime_adapter/state/adapter_track_inventory_mapper.cpp
  src/video/player_runtime_adapter/state/adapter_subtitle_frame_mapper.cpp
)
target_include_directories(kivo_video_player_runtime_adapter
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_player_runtime_adapter PUBLIC cxx_std_20)
target_link_libraries(kivo_video_player_runtime_adapter PUBLIC kivo_video_playback_graph)

add_executable(kivo_player_runtime_adapter_tests
  tests/video/player_runtime_adapter/player_runtime_adapter_tests.cpp
)
target_include_directories(kivo_player_runtime_adapter_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_player_runtime_adapter_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_player_runtime_adapter_tests PRIVATE kivo_video_player_runtime_adapter)
kivo_add_ffmpeg_runtime_deploy(kivo_player_runtime_adapter_tests)
add_test(NAME kivo_player_runtime_adapter_tests COMMAND kivo_player_runtime_adapter_tests)
set_tests_properties(kivo_player_runtime_adapter_tests PROPERTIES TIMEOUT 30)

# --- P8: Subtitle Plane public ABI ---
add_library(kivo_video_subtitle_plane STATIC
  src/video/subtitle_plane/abi/api_bootstrap.cpp
  src/video/subtitle_plane/abi/api_table_builder.cpp
  src/video/subtitle_plane/core/plane_lifecycle_api.cpp
  src/video/subtitle_plane/core/plane_lifecycle_model.cpp
  src/video/subtitle_plane/core/plane_store.cpp
  src/video/subtitle_plane/core/runtime_caps.cpp
  src/video/subtitle_plane/command/command_api.cpp
  src/video/subtitle_plane/command/command_lifecycle.cpp
  src/video/subtitle_plane/timeline/timeline_revision.cpp
  src/video/subtitle_plane/threading/cancellation_poll.cpp
  src/video/subtitle_plane/threading/close_tombstone_policy.cpp
  src/video/subtitle_plane/discovery/discovery_api.cpp
  src/video/subtitle_plane/source/source_api.cpp
  src/video/subtitle_plane/source/source_descriptor_validation.cpp
  src/video/subtitle_plane/source/external_reader_lifecycle.cpp
  src/video/subtitle_plane/source/embedded_packet_api.cpp
  src/video/subtitle_plane/source/embedded_packet_ingest.cpp
  src/video/subtitle_plane/source/embedded_stream_api.cpp
  src/video/subtitle_plane/source/embedded_stream_end.cpp
  src/video/subtitle_plane/frame/frame_build_api.cpp
  src/video/subtitle_plane/frame/frame_hot_path_guard.cpp
  src/video/subtitle_plane/frame/frame_lease_pool.cpp
  src/video/subtitle_plane/generation/generation_freshness.cpp
  src/video/subtitle_plane/snapshot/snapshot_api.cpp
  src/video/subtitle_plane/snapshot/snapshot_blob_writer.cpp
  src/video/subtitle_plane/snapshot/snapshot_lease_pool.cpp
  src/video/subtitle_plane/text/text_access_api.cpp
  src/video/subtitle_plane/text/font_blob_store.cpp
  src/video/subtitle_plane/text/text_buffer_store.cpp
  src/video/subtitle_plane/observer/observer_api.cpp
  src/video/subtitle_plane/observer/observer_registry.cpp
  src/video/subtitle_plane/evidence/evidence_summary.cpp
  src/video/subtitle_plane/metrics/metrics_api.cpp
  src/video/subtitle_plane/privacy/privacy_api.cpp
  src/video/subtitle_plane/privacy/privacy_redline_policy.cpp
  src/video/subtitle_plane/privacy/redacted_identity.cpp
  src/video/subtitle_plane/resource/resource_token_validation.cpp
  src/video/subtitle_plane/discovery/discovery_priority.cpp
  src/video/subtitle_plane/format/ass_semantics.cpp
  src/video/subtitle_plane/format/format_track_classification.cpp
  src/video/subtitle_plane/format/ssa_semantics.cpp
  src/video/subtitle_plane/format/style_semantics_api.cpp
  src/video/subtitle_plane/format/webvtt_semantics.cpp
)
target_include_directories(kivo_video_subtitle_plane
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_subtitle_plane PUBLIC cxx_std_20)
target_link_libraries(kivo_video_playback_graph PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8b_c_header_compile_test
  tests/video/subtitle_plane/abi/p8_c_header_compile_test.c
)
target_include_directories(kivo_p8b_c_header_compile_test PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(kivo_p8b_c_header_compile_test PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
)

add_executable(kivo_p8b_public_contract_tests
  tests/video/subtitle_plane/abi/p8_public_contract_tests.cpp
)
target_include_directories(kivo_p8b_public_contract_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p8b_public_contract_tests PRIVATE cxx_std_20)

add_executable(kivo_p8b_public_header_purist_tests
  tests/video/subtitle_plane/abi/p8_public_header_purist_tests.cpp
)
target_include_directories(kivo_p8b_public_header_purist_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_definitions(kivo_p8b_public_header_purist_tests PRIVATE
  KIVO_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)
target_compile_features(kivo_p8b_public_header_purist_tests PRIVATE cxx_std_20)

add_executable(kivo_p8c_bootstrap_command_tests
  tests/video/subtitle_plane/core/p8c_bootstrap_command_tests.cpp
)
target_include_directories(kivo_p8c_bootstrap_command_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p8c_bootstrap_command_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8c_bootstrap_command_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8d_source_privacy_tests
  tests/video/subtitle_plane/source/p8d_source_privacy_tests.cpp
)
target_include_directories(kivo_p8d_source_privacy_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p8d_source_privacy_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8d_source_privacy_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8e_style_semantics_tests
  tests/video/subtitle_plane/format/p8e_style_semantics_tests.cpp
)
target_include_directories(kivo_p8e_style_semantics_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p8e_style_semantics_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8e_style_semantics_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8e_frame_lease_tests
  tests/video/subtitle_plane/frame/p8e_frame_lease_tests.cpp
)
target_include_directories(kivo_p8e_frame_lease_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p8e_frame_lease_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8e_frame_lease_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8e_generation_lifecycle_tests
  tests/video/subtitle_plane/generation/p8e_generation_lifecycle_tests.cpp
)
target_include_directories(kivo_p8e_generation_lifecycle_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p8e_generation_lifecycle_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8e_generation_lifecycle_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8f_snapshot_blob_tests
  tests/video/subtitle_plane/snapshot/p8f_snapshot_blob_tests.cpp
)
target_include_directories(kivo_p8f_snapshot_blob_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p8f_snapshot_blob_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8f_snapshot_blob_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8f_observer_tests
  tests/video/subtitle_plane/observer/p8f_observer_tests.cpp
)
target_include_directories(kivo_p8f_observer_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p8f_observer_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8f_observer_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8f_text_font_privacy_tests
  tests/video/subtitle_plane/text/p8f_text_font_privacy_tests.cpp
)
target_include_directories(kivo_p8f_text_font_privacy_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p8f_text_font_privacy_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8f_text_font_privacy_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8g_deterministic_harness_tests
  tests/video/subtitle_plane/test_support/p8g_deterministic_harness_tests.cpp
)
target_include_directories(kivo_p8g_deterministic_harness_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/tests/video/subtitle_plane/test_support
)
target_compile_features(kivo_p8g_deterministic_harness_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8g_deterministic_harness_tests PRIVATE kivo_video_subtitle_plane)

add_executable(kivo_p8g_stress_property_tests
  tests/video/subtitle_plane/test_support/p8g_stress_property_tests.cpp
)
target_include_directories(kivo_p8g_stress_property_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(kivo_p8g_stress_property_tests PRIVATE cxx_std_20)
target_link_libraries(kivo_p8g_stress_property_tests PRIVATE kivo_video_subtitle_plane)

add_custom_target(kivo_video_subtitle_plane_tests
  DEPENDS
    kivo_p8b_c_header_compile_test
    kivo_p8b_public_contract_tests
    kivo_p8b_public_header_purist_tests
    kivo_p8c_bootstrap_command_tests
    kivo_p8d_source_privacy_tests
    kivo_p8e_style_semantics_tests
    kivo_p8e_frame_lease_tests
    kivo_p8e_generation_lifecycle_tests
    kivo_p8f_snapshot_blob_tests
    kivo_p8f_observer_tests
    kivo_p8f_text_font_privacy_tests
    kivo_p8g_deterministic_harness_tests
    kivo_p8g_stress_property_tests
)
add_test(NAME kivo_p8b_c_header_compile_test COMMAND kivo_p8b_c_header_compile_test)
set_tests_properties(kivo_p8b_c_header_compile_test PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8b_public_contract_tests COMMAND kivo_p8b_public_contract_tests)
set_tests_properties(kivo_p8b_public_contract_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8b_public_header_purist_tests COMMAND kivo_p8b_public_header_purist_tests)
set_tests_properties(kivo_p8b_public_header_purist_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8c_bootstrap_command_tests COMMAND kivo_p8c_bootstrap_command_tests)
set_tests_properties(kivo_p8c_bootstrap_command_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8d_source_privacy_tests COMMAND kivo_p8d_source_privacy_tests)
set_tests_properties(kivo_p8d_source_privacy_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8e_style_semantics_tests COMMAND kivo_p8e_style_semantics_tests)
set_tests_properties(kivo_p8e_style_semantics_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8e_frame_lease_tests COMMAND kivo_p8e_frame_lease_tests)
set_tests_properties(kivo_p8e_frame_lease_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8e_generation_lifecycle_tests COMMAND kivo_p8e_generation_lifecycle_tests)
set_tests_properties(kivo_p8e_generation_lifecycle_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8f_snapshot_blob_tests COMMAND kivo_p8f_snapshot_blob_tests)
set_tests_properties(kivo_p8f_snapshot_blob_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8f_observer_tests COMMAND kivo_p8f_observer_tests)
set_tests_properties(kivo_p8f_observer_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8f_text_font_privacy_tests COMMAND kivo_p8f_text_font_privacy_tests)
set_tests_properties(kivo_p8f_text_font_privacy_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8g_deterministic_harness_tests COMMAND kivo_p8g_deterministic_harness_tests)
set_tests_properties(kivo_p8g_deterministic_harness_tests PROPERTIES TIMEOUT 30)
add_test(NAME kivo_p8g_stress_property_tests COMMAND kivo_p8g_stress_property_tests)
set_tests_properties(kivo_p8g_stress_property_tests PROPERTIES TIMEOUT 30)

# --- P11-A: Hardware video plane (GPU device + D3D11VA decode) ---
# Build registration follows docs/design/backend-core/01-family-tree.md.
# Dependency direction is one-way: hwaccel -> device. Never the reverse.

add_library(kivo_video_plane_gpu_device STATIC
  src/video/video_plane/backends/d3d11/device/dxgi_adapter_selector.cpp
  src/video/video_plane/backends/d3d11/device/d3d11_video_device.cpp
)
target_include_directories(kivo_video_plane_gpu_device PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/backend/include
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_video_plane_gpu_device PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(kivo_video_plane_gpu_device PRIVATE /utf-8 /W4)
endif()
target_link_libraries(kivo_video_plane_gpu_device PUBLIC d3d11 dxgi)

if(KIVO_ENABLE_FFMPEG)
  add_library(kivo_video_plane_gpu_hwaccel STATIC
    src/video/video_plane/backends/d3d11/hwaccel/ffmpeg_get_format_policy.cpp
    src/video/video_plane/backends/d3d11/hwaccel/ffmpeg_hwdevice_binding.cpp
    src/video/video_plane/backends/d3d11/hwaccel/ffmpeg_d3d11va_decoder.cpp
  )
  target_include_directories(kivo_video_plane_gpu_hwaccel PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/backend/include
    ${CMAKE_SOURCE_DIR}/backend/include_private
  )
  target_compile_features(kivo_video_plane_gpu_hwaccel PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(kivo_video_plane_gpu_hwaccel PRIVATE /utf-8 /W4)
  endif()
  target_link_libraries(kivo_video_plane_gpu_hwaccel PUBLIC
    kivo_video_plane_gpu_device
    kivo_cinema_engine
  )

  add_executable(kivo_video_plane_get_format_policy_tests
    tests/video_plane/decode_gpu/ffmpeg_get_format_policy_tests.cpp
    src/video/video_plane/backends/d3d11/hwaccel/ffmpeg_get_format_policy.cpp
  )
  target_include_directories(kivo_video_plane_get_format_policy_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/backend/include
    ${CMAKE_SOURCE_DIR}/backend/include_private
  )
  if(FFMPEG_ROOT)
    target_include_directories(kivo_video_plane_get_format_policy_tests PRIVATE
      ${FFMPEG_ROOT}/include
    )
  endif()
  target_compile_features(kivo_video_plane_get_format_policy_tests PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(kivo_video_plane_get_format_policy_tests PRIVATE /utf-8)
  endif()
  # Deliberately does not link the FFmpeg import libraries: the policy is pure
  # enum logic and must stay verifiable even when the SDK libs are unavailable.
  add_test(NAME kivo_video_plane_get_format_policy_tests
    COMMAND kivo_video_plane_get_format_policy_tests)
  set_tests_properties(kivo_video_plane_get_format_policy_tests PROPERTIES TIMEOUT 30)

  # --- P11-A2: real-source hardware decode probe ---
  # Separated from the policy test on purpose: this target must link FFmpeg,
  # while the policy test must not, so a broken SDK can never hide the policy
  # regression behind a link failure.
  add_executable(kivo_hw_decode_probe
    src/video/video_plane/probe/probe_options.cpp
    src/video/video_plane/probe/media_source.cpp
    src/video/video_plane/probe/probe_report.cpp
    src/video/video_plane/probe/software_baseline.cpp
    src/video/video_plane/probe/decode_probe.cpp
    src/video/video_plane/probe/main.cpp
  )
  target_include_directories(kivo_hw_decode_probe PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/backend/include
    ${CMAKE_SOURCE_DIR}/backend/include_private
  )
  target_compile_features(kivo_hw_decode_probe PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(kivo_hw_decode_probe PRIVATE /utf-8 /W4)
  endif()
  target_link_libraries(kivo_hw_decode_probe PRIVATE
    kivo_video_plane_gpu_hwaccel
  )
  # Explicit FFmpeg import libraries: the probe links avformat/avcodec directly
  # (demux + software baseline), and relying on transitive propagation through a
  # static library is fragile. Same style as the P6C targets above.
  if(FFMPEG_ROOT)
    target_link_directories(kivo_hw_decode_probe PRIVATE ${FFMPEG_ROOT}/lib)
    target_link_libraries(kivo_hw_decode_probe PRIVATE
      avformat avcodec avutil swresample swscale)
  endif()
endif()

# ─── P12-C2: 可移植 SPSC 音频环形缓冲 ───
# 刻意不在任何 if(WIN32)/if(ANDROID) 里：
# 这份实现零平台依赖，桌面端（含 WASAPI）、Android（AAudio）、iOS（AudioUnit）
# 共用同一份代码与同一组并发测试。想按平台分叉之前先问：分叉能解决什么？
set(KIVO_P12C_CORE_AUDIO_SOURCES
  src/video/video_plane/core/audio/spsc_audio_ring_buffer.cxx
  src/video/video_plane/core/audio/pcm_format_converter.cxx
  # 252：5.1 → 立体声下混。人声几乎全在 FC，交给平台做 6→2 会把人声压掉。
  src/video/video_plane/core/audio/pcm_channel_downmix.cxx
  src/video/video_plane/core/audio/clock/audio_clock_types.cxx
  src/video/video_plane/core/audio/clock/monotonic_clock_guard.cxx
  src/video/video_plane/core/audio/clock/frame_position_extrapolator.cxx
  src/video/video_plane/core/audio/clock/system_clock_source.cxx
  # P13-AB：探针用的时钟台阶注入。放在 core/audio/clock/ 而不是测试目录，
  # 因为它的注入点是 `AudioClockSource` 这层接缝 —— 探针与生产走同一条时钟链，
  # 差别只在"包不包这一层"。
  src/video/video_plane/core/audio/clock/clock_offset_source.cxx
)

add_library(kivo_p12c_core_audio STATIC ${KIVO_P12C_CORE_AUDIO_SOURCES})
target_include_directories(kivo_p12c_core_audio PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12c_core_audio PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c_core_audio PRIVATE /utf-8 /W4)
endif()

# 并发正确性必须能在没有声卡、没有 Android 设备的机器上复现，
# 因此这组测试刻意不链接任何平台库，只链接核心缓冲本身。
add_executable(kivo_p12c_ring_buffer_tests
  tests/video/audio_plane/p12c_ring_buffer_tests/spsc_audio_ring_buffer_tests.cpp
)
target_link_libraries(kivo_p12c_ring_buffer_tests PRIVATE kivo_p12c_core_audio)
target_compile_features(kivo_p12c_ring_buffer_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c_ring_buffer_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12c_ring_buffer_tests COMMAND kivo_p12c_ring_buffer_tests)
set_tests_properties(kivo_p12c_ring_buffer_tests PROPERTIES TIMEOUT 120)

# ─── P12-C3: 可移植 PCM 转换 ───
# 纯函数，没有并发也没有平台依赖，但它是"数据进设备前最后一次形状改变"：
# 比例错 → 音量/削波错；交错错 → 左右互换；放过半帧 → 整段变噪声。
# 这类缺陷不会崩、不会报错，只能靠断言守。
add_executable(kivo_p12c_pcm_tests
  tests/video/audio_plane/p12c_pcm_tests/pcm_format_converter_tests.cpp
)
target_link_libraries(kivo_p12c_pcm_tests PRIVATE kivo_p12c_core_audio)
target_compile_features(kivo_p12c_pcm_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c_pcm_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12c_pcm_tests COMMAND kivo_p12c_pcm_tests)
set_tests_properties(kivo_p12c_pcm_tests PROPERTIES TIMEOUT 60)

# ─── 252: 5.1 → 立体声下混矩阵 ───
# 人声是否还在，全看这张矩阵：FC 必须以 0.7071 同时进左右。
# 系数错了不会崩、不会报错，只会"听着人声小一点"，所以单独一组断言守住。
add_executable(kivo_p12c_downmix_tests
  tests/video/audio_plane/p12c_pcm_tests/pcm_channel_downmix_tests.cpp
)
target_link_libraries(kivo_p12c_downmix_tests PRIVATE kivo_p12c_core_audio)
target_compile_features(kivo_p12c_downmix_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c_downmix_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12c_downmix_tests COMMAND kivo_p12c_downmix_tests)
set_tests_properties(kivo_p12c_downmix_tests PROPERTIES TIMEOUT 60)

# ─── P12-C4: 音频主时钟 ───
# 三类异常（时间戳倒退 / 跳变 / 切换采样率）与回退三档全部落在纯逻辑层，
# 所以这组测试不需要设备、不需要声卡 —— 它测的是"判断"，不是"硬件"。
# 只编不依赖 <aaudio/AAudio.h> 的三个单元；真正读时间戳的那个文件留在 Android 侧。
add_executable(kivo_p12c4_clock_tests
  tests/video/audio_plane/p12c4_clock_tests/audio_clock_tests.cpp
  src/video/video_plane/backends/aaudio/clock/aaudio_clock_source.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_clock_trace.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_clock_tail.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.cxx
)
target_link_libraries(kivo_p12c4_clock_tests PRIVATE kivo_p12c_core_audio)
target_compile_features(kivo_p12c4_clock_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c4_clock_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12c4_clock_tests COMMAND kivo_p12c4_clock_tests)
set_tests_properties(kivo_p12c4_clock_tests PROPERTIES TIMEOUT 60)

# ─── P12-C5: 呈现节拍器 ───
# 节拍器本身零平台依赖（只用 steady_clock + int64 纳秒），所以同一份源
# 在 MSVC 与 arm64 NDK 上编，测试不需要设备。它依赖 C4 的时钟接口，
# 但纯视频时注入的是 SystemClockSource —— 不是"节拍依赖音频"。
set(KIVO_P12C_CORE_PACE_SOURCES
  src/video/video_plane/core/pace/pace_action.cxx
  src/video/video_plane/core/pace/pace_policy.cxx
  src/video/video_plane/core/pace/pace_histogram.cxx
  src/video/video_plane/core/pace/pace_stats.cxx
  src/video/video_plane/core/pace/sampling_mode.cxx
  src/video/video_plane/core/pace/pace_drift_series.cxx
  src/video/video_plane/core/pace/pace_drift_trend.cxx
  src/video/video_plane/core/pace/present_gap.cxx
  src/video/video_plane/core/pace/pace_drift_window.cxx
  src/video/video_plane/core/pace/pace_window_stats.cxx
  src/video/video_plane/core/pace/pace_hac.cxx
  src/video/video_plane/core/pace/pace_halves_agreement.cxx
  # P14-L：派生量的可测性。由 `pace_stats.cxx` 与 `frame_pacer.cxx` 共用 ——
  # 理由字符串是静态字面量，放在一个单元里才不会两处各写一句话。
  src/video/video_plane/core/pace/pace_measurability.cxx
  src/video/video_plane/core/pace/pace_worst_episode.cxx
  src/video/video_plane/core/pace/pace_drift_budget.cxx
  # P14-U：参考系的第二份。由 `pace_stats.cxx` 直接调用 ——
  # 漏注册不会在桌面暴露，只在 arm64 链接时报 `undefined symbol`。
  src/video/video_plane/core/pace/pace_lead_trace.cxx
  src/video/video_plane/core/pace/pace_wall_offset.cxx
  # P14-Y：不连续那本账。与上面两行同一个坑 —— 漏注册桌面照样编过，
  # 只有 arm64 链接时才会报 `undefined symbol`。
  src/video/video_plane/core/pace/pace_discontinuity_ledger.cxx
  src/video/video_plane/core/pace/pace_gate_scale.cxx
  src/video/video_plane/core/pace/pace_now.cxx
  src/video/video_plane/core/pace/clock_position_tracker.cxx
  src/video/video_plane/core/pace/frame_pacer.cxx
)

add_library(kivo_p12c_core_pace STATIC ${KIVO_P12C_CORE_PACE_SOURCES})
target_link_libraries(kivo_p12c_core_pace PUBLIC kivo_p12c_core_audio)
target_include_directories(kivo_p12c_core_pace PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12c_core_pace PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c_core_pace PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p12c5_pace_tests
  tests/video/pace/p12c5_pace_tests/frame_pacer_tests.cpp
)
target_link_libraries(kivo_p12c5_pace_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p12c5_pace_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c5_pace_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12c5_pace_tests COMMAND kivo_p12c5_pace_tests)
set_tests_properties(kivo_p12c5_pace_tests PROPERTIES TIMEOUT 60)

add_executable(kivo_pause_resume_pace_tests
  tests/video/pace/pause_resume_tests/main.cpp
)
target_link_libraries(kivo_pause_resume_pace_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_pause_resume_pace_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_pause_resume_pace_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_pause_resume_pace_tests COMMAND kivo_pause_resume_pace_tests)
set_tests_properties(kivo_pause_resume_pace_tests PROPERTIES TIMEOUT 30)

add_executable(kivo_timestamp_cadence_tests
  tests/video/pace/timestamp_cadence_tests/main.cpp
  src/video/video_plane/backends/aaudio/clock/aaudio_clock_source.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_clock_trace.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_clock_tail.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.cxx
  src/video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.cxx
)
target_link_libraries(kivo_timestamp_cadence_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_timestamp_cadence_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_timestamp_cadence_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_timestamp_cadence_tests COMMAND kivo_timestamp_cadence_tests)
set_tests_properties(kivo_timestamp_cadence_tests PROPERTIES TIMEOUT 30)

# ─── P13-D: 抽稀序列与漂移趋势 ───
# 两个纯计算族（抽稀 + 回归），零平台依赖，因此能在桌面上把逻辑钉死：
# 喂一条 1000 ppm 的直线必须算回 1000，喂 30fps 与 60fps 的同片长必须采到同样多的点。
# 真机上要验的是"设备给的 lateness 长什么样"，那部分只能靠 `docs/evidence/p13d/`。
add_executable(kivo_p13d_drift_tests
  tests/video/pace/p13d_drift_tests/drift_series_tests.cpp
)
target_link_libraries(kivo_p13d_drift_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13d_drift_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13d_drift_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13d_drift_tests COMMAND kivo_p13d_drift_tests)
set_tests_properties(kivo_p13d_drift_tests PROPERTIES TIMEOUT 60)

# ─── P14-A: 三条以前没有的尺子（G2 / G3 / G4）───
# 全部是计数与纯算术，零平台依赖（CPU 那条在 Windows 上返回"未测到"）。
#
# 分配钩子**单独一个库**：`operator new` 是进程级的，谁链上谁生效。
# 把它塞进这个默认库，会让每一个桌面单测都带上这把尺子 ——
# 那时测到的就不是"播放稳态"，而是"测试框架的分配"。
set(KIVO_P14A_METRICS_SOURCES
  src/video/video_plane/core/metrics/cpu_time_facts.cxx
  src/video/video_plane/core/metrics/alloc_probe.cxx
  src/video/video_plane/core/metrics/buffer_lease_facts.cxx
)

add_library(kivo_p14a_metrics STATIC ${KIVO_P14A_METRICS_SOURCES})
target_include_directories(kivo_p14a_metrics PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p14a_metrics PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14a_metrics PRIVATE /utf-8 /W4)
endif()

# 全局 operator new 替换：**只给需要它的目标显式链**。见上面注释。
add_library(kivo_p14a_alloc_hook STATIC
  src/video/video_plane/core/metrics/alloc_probe_hook.cxx
)
target_link_libraries(kivo_p14a_alloc_hook PUBLIC kivo_p14a_metrics)
target_compile_features(kivo_p14a_alloc_hook PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14a_alloc_hook PRIVATE /utf-8)
endif()

add_executable(kivo_p14a_metrics_tests
  tests/video/metrics/p14a_metrics_tests/metrics_facts_tests.cpp
)
target_link_libraries(kivo_p14a_metrics_tests PRIVATE kivo_p14a_metrics)
target_compile_features(kivo_p14a_metrics_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14a_metrics_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14a_metrics_tests COMMAND kivo_p14a_metrics_tests)
set_tests_properties(kivo_p14a_metrics_tests PROPERTIES TIMEOUT 60)

# ─── P13-E: 分位数的采样口径词汇 ───
# 三个名字是线格式（写进 JSON、被 Python 校验器读、被状态卡表格引用），
# 所以"不许改名"必须由测试守，不能只靠注释守。
add_executable(kivo_p13e_sampling_mode_tests
  tests/video/pace/p13e_sampling_mode_tests/sampling_mode_tests.cpp
)
target_link_libraries(kivo_p13e_sampling_mode_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13e_sampling_mode_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13e_sampling_mode_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13e_sampling_mode_tests COMMAND kivo_p13e_sampling_mode_tests)
set_tests_properties(kivo_p13e_sampling_mode_tests PROPERTIES TIMEOUT 60)

# ─── P13-F: 最差那一下在哪儿 ───
# `max` 只说最差多大，说不出在片头还是片尾 —— 而这两者对结论的影响完全相反。
# 位置属于序列，直方图按定义没有，所以极值由 `DriftSeries` 累计维护后上报。
# 这一组守的是"有符号最大"、"index 0 基"、"环形覆盖后仍在"这三条容易写错的口径。
add_executable(kivo_p13f_worst_tests
  tests/video/pace/p13f_worst_tests/worst_tests.cpp
)
target_link_libraries(kivo_p13f_worst_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13f_worst_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13f_worst_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13f_worst_tests COMMAND kivo_p13f_worst_tests)
set_tests_properties(kivo_p13f_worst_tests PROPERTIES TIMEOUT 60)

# ─── P13-G: 抽稀的最小可用门槛 ───
# 1 个样本的抽稀块看起来完全合法（四个分位数相等是唯一破绽），
# 而缺 pts 的流正好会产出它。这组测试把"如果发生会长成什么样"钉死，
# 并把 `kPerFrame` 曾经不可达的那条链用随机序列扫出来。
add_executable(kivo_p13g_decimated_floor_tests
  tests/video/pace/p13g_decimated_floor_tests/decimated_floor_tests.cpp
)
target_link_libraries(kivo_p13g_decimated_floor_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13g_decimated_floor_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13g_decimated_floor_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13g_decimated_floor_tests COMMAND kivo_p13g_decimated_floor_tests)
set_tests_properties(kivo_p13g_decimated_floor_tests PROPERTIES TIMEOUT 60)

# ─── P13-H: 累计媒体跨度（`observed_hz` 的分母） ───
# 守的是一条口径不匹配：`accepted` 累计，分母却取窗口跨度（封顶 102.35 s）。
# 8 s / 35 s 片源上两者相等，所以缺陷只在没人跑过的时长里存在。
add_executable(kivo_p13h_media_span_tests
  tests/video/pace/p13h_media_span_tests/media_span_tests.cpp
)
target_link_libraries(kivo_p13h_media_span_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13h_media_span_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13h_media_span_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13h_media_span_tests COMMAND kivo_p13h_media_span_tests)
set_tests_properties(kivo_p13h_media_span_tests PROPERTIES TIMEOUT 60)

# ─── P13-I: 窗口的身份（覆盖 ≠ 跨段） ───
# 守两件事：① `truncated` 上一版拿整场累计数比容量，seek 之后谎报覆盖；
# ② 趋势描述的那批点必须自带分布，否则"这段抖多少"与"这段漂多少"是两批样本。
add_executable(kivo_p13i_window_tests
  tests/video/pace/p13i_window_tests/window_tests.cpp
)
target_link_libraries(kivo_p13i_window_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13i_window_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13i_window_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13i_window_tests COMMAND kivo_p13i_window_tests)
set_tests_properties(kivo_p13i_window_tests PROPERTIES TIMEOUT 60)

# ─── P13-J: 漂移的误差棒 ───
# 守四件事：① 0.25 ms 桶中心会把 29~250 µs 的真实差异塌成同一个数；
# ② 斜率必须带标准误（r2 = 5.5e-05 时六位小数全是噪声）；
# ③ CI 含 0 时不许说看得见；④ 两个点算不出 SE，那时 SE=0 是"算不出"不是"没有误差"。
add_executable(kivo_p13j_confidence_tests
  tests/video/pace/p13j_confidence_tests/confidence_tests.cpp
)
target_link_libraries(kivo_p13j_confidence_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13j_confidence_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13j_confidence_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13j_confidence_tests COMMAND kivo_p13j_confidence_tests)
set_tests_properties(kivo_p13j_confidence_tests PROPERTIES TIMEOUT 60)

# ─── P13-K: 自相关校正（HAC / Newey–West）───
# 守四件事：① AR(1) 正相关残差下 HAC 的 SE 必须明显大于 IID 的；
# ② 白噪声下两者接近（膨胀比来自自相关，不来自实现）；③ 同一份数据能被
# 独立朴素实现逐位复算；④ 半程斜率分得开"持续漂移"和"中途跳一下"。
add_executable(kivo_p13k_hac_tests
  tests/video/pace/p13k_hac_tests/hac_tests.cpp
)
target_link_libraries(kivo_p13k_hac_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13k_hac_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13k_hac_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13k_hac_tests COMMAND kivo_p13k_hac_tests)
set_tests_properties(kivo_p13k_hac_tests PROPERTIES TIMEOUT 60)

# ─── P13-L: 累积漂移 + 额度判读 ───
# 守四件事：① 分得开 ≠ 要紧（CI 不含 0 而累积只有十几微秒 ⇒ within_budget）；
# ② 反过来也成立（CI 含 0 而远端积起来越过额度 ⇒ over_budget）；
# ③ 上界只增不减；④ 算不出来一律 unknown，不许伪装成 within_budget。
add_executable(kivo_p13l_budget_tests
  tests/video/pace/p13l_budget_tests/budget_tests.cpp
)
target_link_libraries(kivo_p13l_budget_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13l_budget_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13l_budget_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13l_budget_tests COMMAND kivo_p13l_budget_tests)
set_tests_properties(kivo_p13l_budget_tests PROPERTIES TIMEOUT 60)

# ─── P13-M: 段边界的权威是锚点重建 ───
# 守四件事：① 切段后一个**前进**的 pts 也能起新段（P13-L 缺陷的正解）；
# ② pts 倒退的兜底路径不许被拆掉；③ 切段不动任何"整场"计数；
# ④ 段内空洞不切段，但必须出版（`largest_gap_cells` / `contiguous`）。
add_executable(kivo_p13m_segment_tests
  tests/video/pace/p13m_segment_tests/segment_tests.cpp
)
target_link_libraries(kivo_p13m_segment_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13m_segment_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13m_segment_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13m_segment_tests COMMAND kivo_p13m_segment_tests)
set_tests_properties(kivo_p13m_segment_tests PROPERTIES TIMEOUT 60)

# ─── P13-N: 分位数必须自己说出精度 ───
# 守五件事：① 桶中心不是量出来的值（125 000 vs 真值 73 000）；
# ② 桶号与值同源（秩边界上不许挪一格）；③ 可能范围含那个值且被样本钳窄；
# ④ 溢出/端点返回精确值、范围退化成点；⑤ 整批参照与 p50 同秩口径。
add_executable(kivo_p13n_quantile_tests
  tests/video/pace/p13n_quantile_tests/quantile_tests.cpp
)
target_link_libraries(kivo_p13n_quantile_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13n_quantile_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13n_quantile_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13n_quantile_tests COMMAND kivo_p13n_quantile_tests)
set_tests_properties(kivo_p13n_quantile_tests PROPERTIES TIMEOUT 60)

# ─── P12-C6: 送包时序状态机 ───
# 并发本身在桌面上测不了（没有 MediaCodec），所以把送包段唯一有分支的那部分
# 抽成纯状态机再测：它守的是"输入满不许读下一包"和"EOS 只发一次"这两条
# 踩过就丢数据的规矩。线程间的一致性由真机计数器闭合来守。
set(KIVO_P12C_CORE_PIPELINE_SOURCES
  src/video/video_plane/core/decode/async_sample_reader.cxx
  src/video/video_plane/core/pipeline/submit_types.cxx
  src/video/video_plane/core/pipeline/submit_sequencer.cxx
)

add_library(kivo_p12c_core_pipeline STATIC ${KIVO_P12C_CORE_PIPELINE_SOURCES})
target_include_directories(kivo_p12c_core_pipeline PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12c_core_pipeline PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c_core_pipeline PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p12c6_pipeline_tests
  tests/video/pipeline/p12c6_pipeline_tests/submit_sequencer_tests.cpp
)
target_link_libraries(kivo_p12c6_pipeline_tests PRIVATE kivo_p12c_core_pipeline)
target_compile_features(kivo_p12c6_pipeline_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12c6_pipeline_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12c6_pipeline_tests COMMAND kivo_p12c6_pipeline_tests)
set_tests_properties(kivo_p12c6_pipeline_tests PROPERTIES TIMEOUT 60)

add_executable(kivo_async_sample_reader_tests
  tests/video/decode/async_sample_reader_tests/main.cpp)
target_link_libraries(kivo_async_sample_reader_tests PRIVATE kivo_p12c_core_pipeline)
target_compile_features(kivo_async_sample_reader_tests PRIVATE cxx_std_23)
add_test(NAME kivo_async_sample_reader_tests COMMAND kivo_async_sample_reader_tests)
set_tests_properties(kivo_async_sample_reader_tests PROPERTIES TIMEOUT 20)

# ─── P12-D4: 会话收尾顺序 ───
# 顺序被提成了一份 constexpr 表（`session_teardown_plan.hpp`，纯头文件），
# `session_stop` / `session_destroy` 都只是它的消费者。所以这里不需要静态库，
# 只要把头文件目录给到测试。
#
# 为什么这组测试值得独立一个目标：它守的不是某个算法的边界，而是
# "资源谁先死"这条跨模块约束 —— 改它的人多半是在重构 JNI 而不是在写解码逻辑，
# 放在 pace / pipeline 的测试里会被当成无关噪声跳过。
add_executable(kivo_p12d4_lifecycle_tests
  tests/video/session/p12d4_lifecycle_tests/session_teardown_tests.cpp
)
target_include_directories(kivo_p12d4_lifecycle_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12d4_lifecycle_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12d4_lifecycle_tests PRIVATE /utf-8 /W4)
endif()
add_test(NAME kivo_p12d4_lifecycle_tests COMMAND kivo_p12d4_lifecycle_tests)
set_tests_properties(kivo_p12d4_lifecycle_tests PROPERTIES TIMEOUT 60)

# ─── P13-A: 帧率能力（codec × 分辨率） ───
# 解析与判决都是纯函数（不碰 Android），所以能在桌面上钉死。
# 判决一旦错，整张"4K60 能不能"的结论就是错的 —— 而它会决定造什么规格的片源。
add_library(kivo_p13a_framerate STATIC
  src/video/video_plane/backends/mediacodec/device/mediacodec_framerate_capability.cxx
)
target_include_directories(kivo_p13a_framerate PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p13a_framerate PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13a_framerate PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p13a_framerate_tests
  tests/video/device/p13a_framerate_tests/framerate_tests.cpp
)
target_link_libraries(kivo_p13a_framerate_tests PRIVATE kivo_p13a_framerate)
target_compile_features(kivo_p13a_framerate_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13a_framerate_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13a_framerate_tests COMMAND kivo_p13a_framerate_tests)
set_tests_properties(kivo_p13a_framerate_tests PROPERTIES TIMEOUT 60)

# ─── P12-F3B: 轨道加密判定 ───
# 判定的输入是平台给的 `crypto-mode`，规则是纯函数 —— 所以它能在桌面上被钉住。
# 取值那一层（`mediaformat_encryption.cxx`）碰 `AMediaFormat`，只在 Android 上编。
set(KIVO_P12F3B_ENCRYPTION_SOURCES
  src/video/video_plane/backends/mediacodec/source/encryption_facts.cxx
)
add_library(kivo_p12f3b_encryption STATIC ${KIVO_P12F3B_ENCRYPTION_SOURCES})
target_include_directories(kivo_p12f3b_encryption PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12f3b_encryption PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12f3b_encryption PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p12f3b_encryption_tests
  tests/video/source/p12f3b_encryption_tests/encryption_facts_tests.cpp
)
target_link_libraries(kivo_p12f3b_encryption_tests PRIVATE kivo_p12f3b_encryption)
target_compile_features(kivo_p12f3b_encryption_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12f3b_encryption_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12f3b_encryption_tests COMMAND kivo_p12f3b_encryption_tests)
set_tests_properties(kivo_p12f3b_encryption_tests PROPERTIES TIMEOUT 60)

# ─── 210: DV-A 识别 + 透传 ───
# 判定层的输入是平台键（mime / profile / level / bl_compatibility_id），规则是纯函数
# —— 桌面钉住判定，真机只负责键值（`mediaformat_dv_io.cxx` 碰 `AMediaFormat`，只在
# Android 侧编译，注册在 `android/app/src/main/cpp/CMakeLists.txt`）。
# 实测依据（键编码 / 无 DV 解码器）：`docs/evidence/p210/p210-track-dump-*.json`。
set(KIVO_P210_DV_SOURCES
  src/video/video_plane/backends/mediacodec/decode/dv_config_facts.cxx
)
add_library(kivo_p210_dv STATIC ${KIVO_P210_DV_SOURCES})
target_include_directories(kivo_p210_dv PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p210_dv PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p210_dv PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p210_dv_tests
  tests/video/decode/p210_dv_tests/dv_config_facts_tests.cpp
)
target_link_libraries(kivo_p210_dv_tests PRIVATE kivo_p210_dv)
target_compile_features(kivo_p210_dv_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p210_dv_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p210_dv_tests COMMAND kivo_p210_dv_tests)
set_tests_properties(kivo_p210_dv_tests PROPERTIES TIMEOUT 60)

# ─── 211: 软解 fd 输入回调 ───
# 回调语义（顺序读/四种 seek/EOF/length 钳制/offset 基准）是纯逻辑，
# 桌面用真实 fd 钉住；FFmpeg 头在本单元不出现（adapter 公共头零泄漏），
# 所以 Android 侧无需 KIVO_ENABLE_FFMPEG 门控即可编译。
set(KIVO_P211_FD_SOURCES
  src/video/video_plane/runtime/local_media/ffmpeg_fd_callbacks.cxx
)
add_library(kivo_p211_fd STATIC ${KIVO_P211_FD_SOURCES})
target_include_directories(kivo_p211_fd PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
  ${CMAKE_SOURCE_DIR}/backend/include
)
target_compile_features(kivo_p211_fd PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p211_fd PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p211_fd_callbacks_tests
  tests/video/local_media/ffmpeg_fd_callbacks_tests.cpp
)
target_link_libraries(kivo_p211_fd_callbacks_tests PRIVATE kivo_p211_fd)
target_compile_features(kivo_p211_fd_callbacks_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p211_fd_callbacks_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p211_fd_callbacks_tests COMMAND kivo_p211_fd_callbacks_tests)
set_tests_properties(kivo_p211_fd_callbacks_tests PROPERTIES TIMEOUT 60)

# ─── 211: 软解旁路会话（端到端，KIVO_ENABLE_FFMPEG 门控）───
# 桌面真 FFmpeg 跑完整链路：open → start → 计数 sink 收帧 → stop → 证据。
# 用 vs-debug-ffmpeg 预设构建（FFMPEG_ROOT 由预设提供）；无 FFmpeg 时整块跳过，
# 全仓构建照常（P6C 先例）。呈现 sink 走接口注入，桌面用计数 sink——
# ANativeWindow sink 只在 Android 侧编译（android/app/src/main/cpp/CMakeLists.txt）。
if(KIVO_ENABLE_FFMPEG AND FFMPEG_ROOT AND EXISTS "${FFMPEG_ROOT}/include/libavcodec/avcodec.h")
  add_executable(kivo_p211_local_session_tests
    tests/video/local_media/ffmpeg_local_playback_session_tests.cpp
    src/video/video_plane/runtime/local_media/ffmpeg_local_playback_session.cxx
    src/video/video_plane/runtime/local_media/ffmpeg_fd_callbacks.cxx
  )
  target_include_directories(kivo_p211_local_session_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/backend/include
    ${CMAKE_SOURCE_DIR}/backend/include_private
    ${FFMPEG_ROOT}/include
  )
  target_compile_features(kivo_p211_local_session_tests PRIVATE cxx_std_23)
  if(MSVC)
    target_compile_options(kivo_p211_local_session_tests PRIVATE /utf-8)
  endif()
  target_link_libraries(kivo_p211_local_session_tests PRIVATE
    kivo_cinema_engine
  )
  kivo_add_ffmpeg_runtime_deploy(kivo_p211_local_session_tests)
  add_test(NAME kivo_p211_local_session_tests COMMAND kivo_p211_local_session_tests)
  set_tests_properties(kivo_p211_local_session_tests PROPERTIES TIMEOUT 120
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    ENVIRONMENT "PATH=${FFMPEG_ROOT}/bin;%PATH%")
endif()

# ─── 212: 字幕位图合成（纯函数，零依赖）───
# 混合公式 + alpha 方向（VSFilter 惯例：color 低字节 0=不透明）的桌面钉子。
# libass 桥（libass_renderer）只在 Android 侧链接（dist 是 arm64），
# 其中的逻辑部分全部下沉到本单元。
add_executable(kivo_p212_overlay_blend_tests
  tests/video/subtitle_plane/overlay_blend_tests.cpp
)
target_include_directories(kivo_p212_overlay_blend_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p212_overlay_blend_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p212_overlay_blend_tests PRIVATE /utf-8 /W4)
endif()
add_test(NAME kivo_p212_overlay_blend_tests COMMAND kivo_p212_overlay_blend_tests)
set_tests_properties(kivo_p212_overlay_blend_tests PROPERTIES TIMEOUT 60)

# ─── P12-F1: seek 预算判定 ───
# G8（首帧 < 150 ms）的判定是纯函数：它只吃"四个耗时 + 两个位置"，不碰平台。
# 抽出来的理由与 C5/C6 相同：判定规则要能在桌面上被钉住，真机只负责提供数字。
set(KIVO_P12F_CORE_SEEK_SOURCES
  src/video/video_plane/core/seek/seek_budget.cxx
)
add_library(kivo_p12f_core_seek STATIC ${KIVO_P12F_CORE_SEEK_SOURCES})
target_include_directories(kivo_p12f_core_seek PUBLIC
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12f_core_seek PUBLIC cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12f_core_seek PRIVATE /utf-8 /W4)
endif()

add_executable(kivo_p12f1_seek_tests
  tests/video/core/p12f1_seek_tests/seek_budget_tests.cpp
)
target_link_libraries(kivo_p12f1_seek_tests PRIVATE kivo_p12f_core_seek)
target_compile_features(kivo_p12f1_seek_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12f1_seek_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p12f1_seek_tests COMMAND kivo_p12f1_seek_tests)
set_tests_properties(kivo_p12f1_seek_tests PROPERTIES TIMEOUT 60)

# ─── P12-E1: 颜色格式名与位深判定 ───
# 被测的是两个纯函数文件：整数 → 名字/位深，以及"这一轮拿到了什么"的判定。
# 它们没有平台依赖（只用 <cstdio>/<string>），所以桌面直接编，不必造一个静态库。
#
# 为什么值得单独一个目标：NDK 不导出 COLOR_Format* 常量，那些数值是手写的 ——
# 写错不会有编译错误，只会让日志和证据说瞎话。这组测试就是钉住它们。
add_executable(kivo_p12e1_color_tests
  tests/video/decode/p12e1_color_tests/color_facts_tests.cpp
  src/video/video_plane/backends/mediacodec/decode/color_format_names.cxx
  src/video/video_plane/backends/mediacodec/decode/video_color_facts.cxx
)
target_include_directories(kivo_p12e1_color_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12e1_color_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12e1_color_tests PRIVATE /utf-8 /W4)
endif()
add_test(NAME kivo_p12e1_color_tests COMMAND kivo_p12e1_color_tests)
set_tests_properties(kivo_p12e1_color_tests PROPERTIES TIMEOUT 60)

# ─── P12-E2: HDR 分类 ───
# 被测的仍然是纯函数：颜色键常量（从 android.jar 反查后手写）+ `classify_hdr()`。
# 平台侧那层（`mediaformat_color_io`）编不进桌面 —— `AMediaFormat` 只在 Android 上存在。
# 它的正确性由设备端 `kivo_hdr_probe` 的 A/B 两次 configure 证明（见 38-p12e2-status.md）。
add_executable(kivo_p12e2_hdr_tests
  tests/video/decode/p12e2_hdr_tests/hdr_classify_tests.cpp
  src/video/video_plane/backends/mediacodec/decode/color_format_names.cxx
  src/video/video_plane/backends/mediacodec/decode/video_color_facts.cxx
)
target_include_directories(kivo_p12e2_hdr_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p12e2_hdr_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p12e2_hdr_tests PRIVATE /utf-8 /W4)
endif()
add_test(NAME kivo_p12e2_hdr_tests COMMAND kivo_p12e2_hdr_tests)
set_tests_properties(kivo_p12e2_hdr_tests PROPERTIES TIMEOUT 60)

# ─── P13-O: 两个半程斜率的一致性 ───
# 判的是"差多少算差"：`gap > 1.96 × pooled` 为不一致。
# 这组测试的重点不在正例，而在三条"会把失败伪装成通过"的路：
# 一侧没估出来、SE 是 NaN、SE 是负数 —— 三者都必须落到 unknown。
add_executable(kivo_p13o_halves_tests
  tests/video/pace/p13o_halves_tests/halves_tests.cpp
)
target_link_libraries(kivo_p13o_halves_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13o_halves_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13o_halves_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13o_halves_tests COMMAND kivo_p13o_halves_tests)
set_tests_properties(kivo_p13o_halves_tests PROPERTIES TIMEOUT 60)

# P13-P：最差帧的前后邻帧。重点同样不在正例，而在两条"会把未知伪装成已知"的路：
# 不带 DueStep 时 supplied 必须为 0（否则 `due_step_frames == 0` 分不出"没位移"与"没接线"），
# 新最差必须丢掉旧最差的尾巴（否则"收敛用了 8 帧"会凭空成立）。
add_executable(kivo_p13p_episode_tests
  tests/video/pace/p13p_episode_tests/episode_tests.cpp
)
target_link_libraries(kivo_p13p_episode_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p13p_episode_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p13p_episode_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p13p_episode_tests COMMAND kivo_p13p_episode_tests)
set_tests_properties(kivo_p13p_episode_tests PROPERTIES TIMEOUT 60)

# ─── P14-R: 会话的收尾方式（completed / interrupted / failed） ───
#
# 纯头文件、纯 constexpr，不碰 Android —— 派生规则与那三个词都能在桌面上钉死。
#
# 为什么值得一个独立目标：这一格是**两份报告共用的协议**（`decode/19` 与 `sync/29`），
# 而它最容易出的错不是"算错"，是"两处各写一遍然后各自演化"和"复制粘贴忘改名字"。
# 前者单测抓不到（两份都能跑），后者会静默失效（判据的字符串白名单照样通过）。
# t5 逐字比对、t7 两两不同，就是为这两件事各钉一颗钉子。
add_executable(kivo_p14r_end_reason_tests
  tests/video/session/p14r_end_reason_tests/end_reason_tests.cpp
)
target_include_directories(kivo_p14r_end_reason_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/backend/include_private
)
target_compile_features(kivo_p14r_end_reason_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14r_end_reason_tests PRIVATE /utf-8 /W4)
endif()
add_test(NAME kivo_p14r_end_reason_tests COMMAND kivo_p14r_end_reason_tests)
set_tests_properties(kivo_p14r_end_reason_tests PROPERTIES TIMEOUT 60)

# ─── P14-T: 钟比墙快了多少（`clock_lead_ns`） ───
#
# 一格里装的是两个已出版的跨度之差，看起来不值一个目标。值得，因为：
#   ① 它是 P14-S 那 4 份"成因未查"的答案，算错了报出来的仍然是"合理的小时数"；
#   ② 它住在 `core::pace`，而**报告层只在 Android 侧编译** —— 不在桌面钉住，
#      这一格就只被真机测过一次（P14-S 那轮手工算的 19 502 ns）；
#   ③ 方向（正 = 钟快）是判据归因的全部依据，反了会把两种病报成一句话。
add_executable(kivo_p14t_clock_lead_tests
  tests/video/pace/p14t_clock_lead_tests/clock_lead_tests.cpp
)
target_link_libraries(kivo_p14t_clock_lead_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p14t_clock_lead_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14t_clock_lead_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14t_clock_lead_tests COMMAND kivo_p14t_clock_lead_tests)
set_tests_properties(kivo_p14t_clock_lead_tests PROPERTIES TIMEOUT 60)

# ---- P14-U：参考系的第二份 ------------------------------------------------
#
# 这一组测的不是"钟偏了多少"（上一组已经钉住了），而是"**怎么偏的**"：
# 一次跳，还是一路跑偏。两者在末值上完全同形 —— 400 ms 的阶跃和 6000 ppm 的
# 匀速跑偏在 8 秒后都能给出 400 ms 的末值，而修法一个是抓原始时间戳、
# 一个是改采样率换算。区分它们只有一步之差的 `max_step_ns` 这一格。
#
# 另有三条只有这一个目标能守：
#   ① 缺帧长时**不许**落到 `ramped`（缺参数给出的结论，方向指向硬件坏了）；
#   ② `surface_at_time` 档一个样本都不许喂（喂 0 会造出 `steady` 的假观测）；
#   ③ `PaceStats` 的接线真的接上了 —— 纯类测试测不出"忘了在 note_presented 里调用"。
add_executable(kivo_p14u_lead_trace_tests
  tests/video/pace/p14u_lead_trace_tests/lead_trace_tests.cpp
)
target_link_libraries(kivo_p14u_lead_trace_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p14u_lead_trace_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14u_lead_trace_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14u_lead_trace_tests COMMAND kivo_p14u_lead_trace_tests)
set_tests_properties(kivo_p14u_lead_trace_tests PROPERTIES TIMEOUT 60)

# P15-A2：跨度按段。窗口量按段、生命周期量按场 —— 这一组守那条界线。
add_executable(kivo_p15a2_span_segments_tests
  tests/video/pace/p15a2_span_segments_tests/span_segments_tests.cpp
)
target_link_libraries(kivo_p15a2_span_segments_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p15a2_span_segments_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p15a2_span_segments_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p15a2_span_segments_tests COMMAND kivo_p15a2_span_segments_tests)
set_tests_properties(kivo_p15a2_span_segments_tests PROPERTIES TIMEOUT 60)

# P14-V：门限的量纲。纯换算，`kivo_p12c_core_pace` 里已经带着 `pace_gate_scale.cxx`。
add_executable(kivo_p14v_gate_scale_tests
  tests/video/pace/p14v_gate_scale_tests/gate_scale_tests.cpp
)
target_link_libraries(kivo_p14v_gate_scale_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p14v_gate_scale_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14v_gate_scale_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14v_gate_scale_tests COMMAND kivo_p14v_gate_scale_tests)
set_tests_properties(kivo_p14v_gate_scale_tests PROPERTIES TIMEOUT 60)

# ─── P14-Y: 钟的不连续那本账 ───
# 纯账本 + tracker 集成，零平台依赖：喂一笔"假事件"就能验账，
# 不必构造时钟 —— "账对不对"与"分类对不对"因此可以分开验。
add_executable(kivo_p14y_discontinuity_tests
  tests/video/pace/p14y_discontinuity_tests/discontinuity_tests.cpp
)
target_link_libraries(kivo_p14y_discontinuity_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p14y_discontinuity_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14y_discontinuity_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14y_discontinuity_tests COMMAND kivo_p14y_discontinuity_tests)
set_tests_properties(kivo_p14y_discontinuity_tests PROPERTIES TIMEOUT 60)

# ─── P14-Z: 台阶还不还（探针两臂）───
# 同一份假时钟跑两臂：A 臂门还它一次，B 臂门一次不动而账照记。
# 没有台阶的一轮两臂必须逐位相同 —— 探针不许动无关轮次。
add_executable(kivo_p14z_step_repay_tests
  tests/video/pace/p14z_step_repay_tests/step_repay_tests.cpp
)
target_link_libraries(kivo_p14z_step_repay_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p14z_step_repay_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14z_step_repay_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14z_step_repay_tests COMMAND kivo_p14z_step_repay_tests)
set_tests_properties(kivo_p14z_step_repay_tests PROPERTIES TIMEOUT 60)

# ─── P14-AA: at_time 档的跨度与 DueStep ───
# scheduled 也要喂跨度；presented 的口径不动；混着喂报 mixed。
add_executable(kivo_p14aa_scheduled_span_tests
  tests/video/pace/p14aa_scheduled_span_tests/scheduled_span_tests.cpp
)
target_link_libraries(kivo_p14aa_scheduled_span_tests PRIVATE kivo_p12c_core_pace)
target_compile_features(kivo_p14aa_scheduled_span_tests PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(kivo_p14aa_scheduled_span_tests PRIVATE /utf-8)
endif()
add_test(NAME kivo_p14aa_scheduled_span_tests COMMAND kivo_p14aa_scheduled_span_tests)
set_tests_properties(kivo_p14aa_scheduled_span_tests PROPERTIES TIMEOUT 60)
