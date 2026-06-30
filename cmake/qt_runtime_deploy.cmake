include_guard(GLOBAL)

function(kivo_log_qt_runtime_paths)
  get_filename_component(_qt6_root "${Qt6_DIR}/../../.." ABSOLUTE)
  set(_qt_bin "${_qt6_root}/bin")
  set(_qt_plugins "${_qt6_root}/plugins")
  set(_qt_qml "${_qt6_root}/qml")
  if(NOT WINDEPLOYQT_EXECUTABLE)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6
      HINTS "${_qt_bin}" "${KIVO_QT_ROOT}/bin")
  endif()
  message(STATUS "KIVO Qt6_DIR=${Qt6_DIR}")
  message(STATUS "KIVO Qt bin dir=${_qt_bin}")
  message(STATUS "KIVO Qt plugin dir=${_qt_plugins}")
  message(STATUS "KIVO Qt qml dir=${_qt_qml}")
  message(STATUS "KIVO windeployqt=${WINDEPLOYQT_EXECUTABLE}")
endfunction()

function(kivo_add_qt_runtime_deploy target)
  if(NOT WIN32)
    message(STATUS "kivo_add_qt_runtime_deploy skipped for non-Windows target ${target}")
    return()
  endif()
  if(NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "windeployqt not found; cannot deploy Qt runtime for ${target}")
  endif()

  set(_qml_dir "${CMAKE_SOURCE_DIR}/ui/playback")
  get_filename_component(_qt6_root "${Qt6_DIR}/../../.." ABSOLUTE)
  set(_qt_platforms "${_qt6_root}/plugins/platforms")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
      "$<IF:$<CONFIG:Debug>,--debug,--release>"
      --qmldir "${_qml_dir}"
      --no-translations
      "$<TARGET_FILE:${target}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/cmake/qt.conf.in"
      "$<TARGET_FILE_DIR:${target}>/qt.conf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_qt_platforms}/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
      "$<TARGET_FILE_DIR:${target}>/platforms"
    COMMENT "Deploying Qt runtime for ${target}"
    VERBATIM
  )
endfunction()

function(kivo_add_qt_runtime_test test_name target)
  add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${target}>)
  set_tests_properties(${test_name} PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
  )
endfunction()