cmake_minimum_required(VERSION 3.21)
project(kivo_ai_spike CXX)

# T-0 引擎压力冒烟的可执行（独立于 APK；产物 adb push 到 /data/local/tmp 直接跑）。
# llama 静态库来自 build.sh 的 dist（先跑 third_party/llama.cpp-android/build.sh）。

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED LLAMA_DIST_DIR)
    set(LLAMA_DIST_DIR "C:/re-tools/llama.cpp-android/dist")
endif()

add_executable(kivo_ai_spike spike_main.cpp)

target_include_directories(kivo_ai_spike PRIVATE "${LLAMA_DIST_DIR}/include")
target_link_libraries(kivo_ai_spike
    "${LLAMA_DIST_DIR}/lib/libllama.a"
    "${LLAMA_DIST_DIR}/lib/libggml.a"
    "${LLAMA_DIST_DIR}/lib/libggml-base.a"
    "${LLAMA_DIST_DIR}/lib/libggml-cpu.a"
)