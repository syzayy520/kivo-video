cmake_minimum_required(VERSION 3.24)
project(KivoCinemaEngine LANGUAGES CXX)

add_library(kivo_cinema_engine INTERFACE)
target_compile_features(kivo_cinema_engine INTERFACE cxx_std_20)
target_include_directories(kivo_cinema_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
