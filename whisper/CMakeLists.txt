# whisper/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(whisper)

add_library(whisper)

target_include_directories(whisper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB WSP_SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(whisper PRIVATE ${WSP_SOURCES})

target_include_directories(whisper PUBLIC ${Tracy_SOURCE_DIR}/public)

target_compile_definitions(whisper PRIVATE VULKAN_HPP_TYPESAFE_CONVERSION)
target_compile_definitions(whisper PRIVATE TRACY_ENABLE)

add_subdirectory(rendering)

target_link_libraries(
  whisper
  PRIVATE TracyClient
          tinyxml2
          imgui
          tinyobjloader
          spdlog
          glfw
          ${Vulkan_LIBRARIES})

if(UNIX AND NOT APPLE)
  target_link_libraries(whisper PRIVATE dl pthread X11 Xxf86vm Xrandr Xi)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(whisper PRIVATE -Wno-cpp)
endif()
