# whisper/rendering/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

file(GLOB RENDERING_SOURCES CONFIGURE_DEPENDS *.cpp)

target_include_directories(whisper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(whisper PRIVATE ${RENDERING_SOURCES})
