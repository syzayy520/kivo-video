cmake_minimum_required(VERSION 3.28)

project(KivoVideo LANGUAGES C CXX)

enable_testing()

if(MSVC)
  add_compile_options(/utf-8)
endif()

add_subdirectory(backend)
add_subdirectory(ui)
