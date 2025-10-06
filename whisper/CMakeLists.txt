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

target_link_libraries(
  whisper
  PRIVATE TracyClient
          tinyxml2
          imgui
          cgltf
          spdlog
          glm::glm
          glfw
          ${Vulkan_LIBRARIES})

if(UNIX AND NOT APPLE)
  target_link_libraries(whisper PRIVATE dl pthread X11 Xxf86vm Xrandr Xi)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(whisper PRIVATE -Wno-cpp)
endif()

file(GLOB_RECURSE ALL_HEADERS CONFIGURE_DEPENDS *.hpp *.h)

set(HEADERS "")
foreach(header IN LISTS ALL_HEADERS)
  if(NOT header MATCHES "/\\.generated/")
    list(APPEND HEADERS ${header})
  endif()
endforeach()

set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

foreach(HEADER ${HEADERS})
  get_filename_component(HEADER_NAME ${HEADER} NAME_WE)
  set(OUTPUT_FILE "${GENERATED_DIR}/${HEADER_NAME}.generated.hpp")

  # Use a unique custom command *only if it's not already declared*
  if(NOT TARGET frost_gen_${HEADER_NAME})
    add_custom_command(
      OUTPUT ${OUTPUT_FILE}
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tooling/frost.py
              ${HEADER} > ${OUTPUT_FILE}
      DEPENDS ${HEADER} ${CMAKE_SOURCE_DIR}/tooling/frost.py
      COMMENT "Generating metadata for ${HEADER_NAME}")
    add_custom_target(frost_gen_${HEADER_NAME} DEPENDS ${OUTPUT_FILE})
    add_dependencies(whisper frost_gen_${HEADER_NAME})
  endif()
endforeach()
