cmake_minimum_required(VERSION 3.28)

option(KIVO_ENABLE_PLAYBACK_SHELL "Build Qt playback shell executable" ON)

if(NOT KIVO_ENABLE_PLAYBACK_SHELL)
  return()
endif()

if(NOT DEFINED KIVO_QT_ROOT)
  set(KIVO_QT_ROOT "C:/tmp/KivoMusicQt_ascii_lyrics_normalizer/.qt/6.8.3/msvc2022_64"
      CACHE PATH "Qt 6 installation prefix for playback shell")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${KIVO_QT_ROOT}")

find_package(Qt6 6.5 COMPONENTS Core Gui Qml Quick QUIET)
if(NOT Qt6_FOUND)
  message(STATUS "Qt6 not found at ${KIVO_QT_ROOT}; skipping kivo_video_playback_shell")
  return()
endif()

include("${CMAKE_SOURCE_DIR}/cmake/qt_runtime_deploy.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ffmpeg_runtime_deploy.cmake")
kivo_log_qt_runtime_paths()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(kivo_video_playback_shell
  shell/playback_shell_main.cpp
  shell/playback_shell_runtime_context.cpp
  shell/playback_shell_evidence_options.cpp
  shell/playback_shell_runtime_adapter_bridge.cpp
  shell/playback_shell_snapshot_mapper.cpp
  shell/playback_shell_qml_registration.cpp
  shell/playback_shell_video_launch_support.cpp
  shell/playback_shell_manual_media_open_controller.cpp
  shell/playback_shell_normal_media_launch.cpp
  shell/playback_shell_interaction_evidence.cpp
  shell/playback_shell_playback_evidence.cpp
  shell/playback_shell_continuous_playback_evidence.cpp
  shell/playback_shell_continuous_evidence_record.cpp
  shell/playback_shell_real_local_evidence.cpp
  shell/playback_shell_backend_layer_status.cpp
  shell/playback_shell_media_resolver.cpp
  shell/playback_shell_video_surface_geometry.cpp
  shell/playback_shell_video_surface_item.cpp
  shell/playback_shell_qml_resources.qrc
  playback/playback_qml_resources.qrc
)

target_include_directories(kivo_video_playback_shell PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)

target_compile_features(kivo_video_playback_shell PRIVATE cxx_std_20)

target_link_libraries(kivo_video_playback_shell PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
  kivo_video_player_runtime_adapter
)

kivo_add_qt_runtime_deploy(kivo_video_playback_shell)
kivo_add_ffmpeg_runtime_deploy(kivo_video_playback_shell)

add_executable(kivo_playback_shell_qml_load_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_shell_qml_load_test.cpp
  shell/playback_shell_qml_registration.cpp
  shell/playback_shell_qml_resources.qrc
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_shell_qml_load_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_shell_qml_load_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_shell_qml_load_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_shell_qml_load_test)

kivo_add_qt_runtime_test(kivo_playback_shell_qml_load_test kivo_playback_shell_qml_load_test)

add_executable(kivo_playback_shell_interaction_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_shell_interaction_test.cpp
)
target_compile_features(kivo_playback_shell_interaction_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_shell_interaction_test PRIVATE Qt6::Core)

add_test(NAME kivo_playback_shell_interaction_test
  COMMAND $<TARGET_FILE:kivo_playback_shell_interaction_test>)
set_tests_properties(kivo_playback_shell_interaction_test PROPERTIES
  TIMEOUT 60
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:kivo_playback_shell_interaction_test>"
)

add_executable(kivo_playback_shell_video_surface_geometry_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_shell_video_surface_geometry_test.cpp
  shell/playback_shell_video_surface_geometry.cpp
)
target_include_directories(kivo_playback_shell_video_surface_geometry_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_shell_video_surface_geometry_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_shell_video_surface_geometry_test PRIVATE Qt6::Core)

add_test(NAME kivo_playback_shell_video_surface_geometry_test
  COMMAND $<TARGET_FILE:kivo_playback_shell_video_surface_geometry_test>)
set_tests_properties(kivo_playback_shell_video_surface_geometry_test PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:kivo_playback_shell_video_surface_geometry_test>"
)

add_executable(kivo_playback_snapshot_mapper_track_label_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_snapshot_mapper_track_label_test.cpp
  shell/playback_shell_snapshot_mapper.cpp
)
target_include_directories(kivo_playback_snapshot_mapper_track_label_test PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_snapshot_mapper_track_label_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_snapshot_mapper_track_label_test PRIVATE Qt6::Core)

add_test(NAME kivo_playback_snapshot_mapper_track_label_test
  COMMAND $<TARGET_FILE:kivo_playback_snapshot_mapper_track_label_test>)
set_tests_properties(kivo_playback_snapshot_mapper_track_label_test PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:kivo_playback_snapshot_mapper_track_label_test>"
)

add_executable(kivo_playback_snapshot_binding_stale_options_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_snapshot_binding_stale_options_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_snapshot_binding_stale_options_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_snapshot_binding_stale_options_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_snapshot_binding_stale_options_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_snapshot_binding_stale_options_test)

kivo_add_qt_runtime_test(kivo_playback_snapshot_binding_stale_options_test
  kivo_playback_snapshot_binding_stale_options_test)

add_executable(kivo_playback_settings_sheet_visibility_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_settings_sheet_visibility_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_settings_sheet_visibility_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_settings_sheet_visibility_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_settings_sheet_visibility_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_settings_sheet_visibility_test)

kivo_add_qt_runtime_test(kivo_playback_settings_sheet_visibility_test
  kivo_playback_settings_sheet_visibility_test)

add_executable(kivo_playback_sheet_shortcut_visibility_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_sheet_shortcut_visibility_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_sheet_shortcut_visibility_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_sheet_shortcut_visibility_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_sheet_shortcut_visibility_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_sheet_shortcut_visibility_test)

kivo_add_qt_runtime_test(kivo_playback_sheet_shortcut_visibility_test
  kivo_playback_sheet_shortcut_visibility_test)

add_executable(kivo_playback_timeline_seekability_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_timeline_seekability_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_timeline_seekability_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_timeline_seekability_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_timeline_seekability_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_timeline_seekability_test)

kivo_add_qt_runtime_test(kivo_playback_timeline_seekability_test
  kivo_playback_timeline_seekability_test)

add_executable(kivo_playback_input_hidden_shortcut_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_input_hidden_shortcut_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_input_hidden_shortcut_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_input_hidden_shortcut_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_input_hidden_shortcut_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_input_hidden_shortcut_test)

kivo_add_qt_runtime_test(kivo_playback_input_hidden_shortcut_test
  kivo_playback_input_hidden_shortcut_test)

add_executable(kivo_playback_overlay_sheet_gate_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_overlay_sheet_gate_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_overlay_sheet_gate_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_overlay_sheet_gate_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_overlay_sheet_gate_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_overlay_sheet_gate_test)

kivo_add_qt_runtime_test(kivo_playback_overlay_sheet_gate_test
  kivo_playback_overlay_sheet_gate_test)

add_executable(kivo_playback_status_chrome_visibility_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_status_chrome_visibility_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_status_chrome_visibility_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_status_chrome_visibility_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_status_chrome_visibility_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_status_chrome_visibility_test)

kivo_add_qt_runtime_test(kivo_playback_status_chrome_visibility_test
  kivo_playback_status_chrome_visibility_test)

add_executable(kivo_playback_seek_availability_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_seek_availability_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_seek_availability_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_seek_availability_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_seek_availability_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_seek_availability_test)

kivo_add_qt_runtime_test(kivo_playback_seek_availability_test
  kivo_playback_seek_availability_test)

add_executable(kivo_playback_context_menu_state_gate_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_context_menu_state_gate_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_context_menu_state_gate_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_context_menu_state_gate_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_context_menu_state_gate_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_context_menu_state_gate_test)

kivo_add_qt_runtime_test(kivo_playback_context_menu_state_gate_test
  kivo_playback_context_menu_state_gate_test)

add_executable(kivo_playback_input_modal_state_gate_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_input_modal_state_gate_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_input_modal_state_gate_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_input_modal_state_gate_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_input_modal_state_gate_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_input_modal_state_gate_test)

kivo_add_qt_runtime_test(kivo_playback_input_modal_state_gate_test
  kivo_playback_input_modal_state_gate_test)

add_executable(kivo_playback_fullscreen_gesture_state_gate_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_fullscreen_gesture_state_gate_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_fullscreen_gesture_state_gate_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_fullscreen_gesture_state_gate_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_fullscreen_gesture_state_gate_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_fullscreen_gesture_state_gate_test)

kivo_add_qt_runtime_test(kivo_playback_fullscreen_gesture_state_gate_test
  kivo_playback_fullscreen_gesture_state_gate_test)

add_executable(kivo_playback_more_hub_visibility_test
  ${CMAKE_SOURCE_DIR}/tests/ui_shell/playback_more_hub_visibility_test.cpp
  playback/playback_qml_resources.qrc
)
target_include_directories(kivo_playback_more_hub_visibility_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_compile_features(kivo_playback_more_hub_visibility_test PRIVATE cxx_std_20)
target_link_libraries(kivo_playback_more_hub_visibility_test PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
)

kivo_add_qt_runtime_deploy(kivo_playback_more_hub_visibility_test)

kivo_add_qt_runtime_test(kivo_playback_more_hub_visibility_test
  kivo_playback_more_hub_visibility_test)
