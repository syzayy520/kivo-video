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
kivo_log_qt_runtime_paths()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(kivo_video_playback_shell
  shell/playback_shell_main.cpp
  shell/playback_shell_runtime_context.cpp
  shell/playback_shell_runtime_adapter_bridge.cpp
  shell/playback_shell_snapshot_mapper.cpp
  shell/playback_shell_qml_registration.cpp
  shell/playback_shell_interaction_evidence.cpp
  shell/playback_shell_playback_evidence.cpp
  shell/playback_shell_media_resolver.cpp
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