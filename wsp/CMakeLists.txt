# wsp/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(wsp)

add_library(wsp)

target_include_directories(wsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB WSP_SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(wsp PRIVATE ${WSP_SOURCES})

target_include_directories(wsp PUBLIC ${CMAKE_SOURCE_DIR}/.external/stb)

target_compile_definitions(wsp PRIVATE VULKAN_HPP_TYPESAFE_CONVERSION)

add_subdirectory(rendering)

target_link_libraries(wsp PRIVATE tinyxml2 imgui imguizmo tinyobjloader spdlog glfw ${Vulkan_LIBRARIES})

if(UNIX AND NOT APPLE)
  target_link_libraries(wsp PRIVATE dl pthread X11 Xxf86vm Xrandr Xi)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(wsp PRIVATE -Wno-cpp)
endif()
