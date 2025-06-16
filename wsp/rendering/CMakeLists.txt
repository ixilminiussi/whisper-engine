# wsp/rendering/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

file(GLOB RENDERING_SOURCES *.cpp)

target_include_directories(wsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(wsp PRIVATE ${RENDERING_SOURCES})
