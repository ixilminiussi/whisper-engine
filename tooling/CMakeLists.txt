cmake_minimum_required(VERSION 3.16)
project(Frost LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force correct LLVM/Clang CMake modules
set(LLVM_DIR "/usr/lib64/cmake/llvm")
set(Clang_DIR "/usr/lib64/cmake/clang")

find_library(CLANG_CPP_LIB NAMES clang-cpp REQUIRED)
find_library(LLVM_LIB NAMES LLVM REQUIRED)

include_directories(${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(frost main.cpp)

target_link_libraries(frost PRIVATE ${CLANG_CPP_LIB} ${LLVM_LIB})
