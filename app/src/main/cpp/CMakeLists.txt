cmake_minimum_required(VERSION 3.22)
project(mpeandroid CXX)
set(CMAKE_CXX_STANDARD 17)

# The engine is plain C++ and also builds on the desktop (see engine/). It needs the `faust`
# compiler on the machine running Gradle, to turn each .dsp into C++.
set(MPE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../engine engine)

find_package(oboe REQUIRED CONFIG)

add_library(mpeandroid SHARED jni.cpp Synth.cpp)
target_compile_options(mpeandroid PRIVATE -Wall -Wextra -O3)
target_link_libraries(mpeandroid mpe_engine oboe::oboe amidi android log)
