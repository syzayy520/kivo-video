include_guard(GLOBAL)

function(kivo_add_ffmpeg_runtime_deploy target)
  if(NOT WIN32 OR NOT KIVO_ENABLE_FFMPEG)
    return()
  endif()
  if(NOT FFMPEG_ROOT)
    message(WARNING "FFmpeg runtime deploy skipped for ${target}: FFMPEG_ROOT is not set")
    return()
  endif()

  set(_ffmpeg_bin "${FFMPEG_ROOT}/bin")
  if(NOT IS_DIRECTORY "${_ffmpeg_bin}")
    message(WARNING "FFmpeg runtime deploy skipped for ${target}: ${_ffmpeg_bin} is missing")
    return()
  endif()

  file(GLOB _ffmpeg_runtime_dlls CONFIGURE_DEPENDS "${_ffmpeg_bin}/*.dll")
  if(NOT _ffmpeg_runtime_dlls)
    message(WARNING "FFmpeg runtime deploy skipped for ${target}: no DLLs found in ${_ffmpeg_bin}")
    return()
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${_ffmpeg_runtime_dlls}
      "$<TARGET_FILE_DIR:${target}>"
    COMMENT "Deploying FFmpeg runtime for ${target}"
    VERBATIM
  )
endfunction()
