cmake_minimum_required(VERSION 3.10)
project(Whisper CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconst-qual
        -Wduplicate-decl-specifier
        -Wuninitialized
        -Wshadow
    )
endif()

if(NOT DEFINED GAME)
    set(GAME "demo" CACHE STRING "Choose game module to build (e.g. demo, descent)")
endif()

# Set game directory
set(GAME_DIR "${CMAKE_SOURCE_DIR}/${GAME}/")
add_definitions(-DGAME_FILES="${GAME_DIR}")

include_directories(${CMAKE_SOURCE_DIR})

add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

# Asset and shader paths
set(WSP_FILES "${CMAKE_SOURCE_DIR}/assets/")
set(WSP_EDITOR_FILES "${CMAKE_SOURCE_DIR}/editor/")
add_definitions(-DEDITOR_FILES="${WSP_EDITOR_FILES}")

set(SHADER_FILES "${CMAKE_BINARY_DIR}/shaders/")
add_definitions(-DSHADER_FILES="${SHADER_FILES}")

# Add subdirectories
add_subdirectory(.external)
add_subdirectory(whisper)
add_subdirectory(flow)
add_subdirectory(${GAME})
# add_subdirectory(shaders)

# Copy assets
# file(GLOB WSP_ASSETS "${WSP_FILES}/*")
# file(COPY ${WSP_ASSETS} DESTINATION "${GAME_DIR}/.wsp_assets")
# file(MAKE_DIRECTORY "${GAME_DIR}/.editor")

message(STATUS "Copied whisper assets into game directory")

# Clean target
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove VulkanTest
    COMMENT "Cleaning up VulkanTest"
)
