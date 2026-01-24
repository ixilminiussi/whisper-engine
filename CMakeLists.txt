cmake_minimum_required(VERSION 3.10)
project(Whisper CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE
    Debug
    CACHE STRING "" FORCE)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include(FetchContent)

add_compile_definitions(VULKAN_HPP_TYPESAFE_CONVERSION)
add_compile_definitions(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)

cmake_policy(VERSION 3.12...3.31)

# -- tracy
set(TRACY_ENABLE ON)
set(TRACY_VK_ENABLE ON)
set(TRACY_NO_CONTEXT_SWITCH ON)
set(TRACY_ON_DEMAND ON)
set(TRACY_NO_SAMPLING ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

message(STATUS "Using vendored Tracy")
FetchContent_Declare(
  Tracy
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/tracy)
FetchContent_MakeAvailable(Tracy)

# -- tinyxml2
message(STATUS "Using vendored tinyxml2")
FetchContent_Declare(
  tinyxml2
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/tinyxml2)
FetchContent_MakeAvailable(tinyxml2)

# -- glm
message(STATUS "Using vendored glm")
FetchContent_Declare(
  glm
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/glm)
FetchContent_MakeAvailable(glm)

# -- stb_image
FetchContent_Declare(
  stb_image
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/stb_image)
FetchContent_MakeAvailable(stb_image)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_image_SOURCE_DIR})

# -- tinyexr
message(STATUS "Using vendored tinyexr")
FetchContent_Declare(
  tinyexr
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/tinyexr)
FetchContent_MakeAvailable(tinyexr)

target_include_directories(tinyexr INTERFACE ${tinyexr_SOURCE_DIR})

# -- cgltf
FetchContent_Declare(
  cgltf
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/cgltf)
FetchContent_MakeAvailable(cgltf)

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

# -- Vulkan
message(STATUS "Using vendored Vulkan")
FetchContent_Declare(
  VulkanHeaders
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/vulkanheaders)
FetchContent_MakeAvailable(VulkanHeaders)

FetchContent_Declare(
  VulkanLoader
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/vulkanloader)
set(VULKAN_HEADERS_INSTALL_DIR
  ${vulkanheaders_SOURCE_DIR}/include
  CACHE PATH "" FORCE)
FetchContent_MakeAvailable(VulkanLoader)

# Create an alias for consistency
add_library(Vulkan::Vulkan ALIAS vulkan)

# -- GLFW
message(STATUS "Using vendored glfw3")
FetchContent_Declare(
  glfw
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/glfw)
FetchContent_MakeAvailable(glfw)

# -- spdlog
message(STATUS "Using vendored spdlog")
FetchContent_Declare(
  spdlog
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/spdlog)
FetchContent_MakeAvailable(spdlog)

# -- imgui
FetchContent_Declare(
  imgui
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)

FetchContent_MakeAvailable(imgui)

add_library(
  imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)

target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_DOCKING
                                        IMGUI_ENABLE_VIEWPORTS)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}
                                        ${imgui_SOURCE_DIR}/backends)

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconst-qual
    -Wduplicate-decl-specifier
    -Wuninitialized
    -Wshadow)
endif()

if(NOT DEFINED GAME)
  set(GAME
      "demo"
      CACHE STRING "Choose game module to build (e.g. demo, descent)")
endif()

# Set game directory
set(GAME_DIR "${CMAKE_SOURCE_DIR}/${GAME}/")
add_definitions(-DGAME_FILES="${GAME_DIR}")

include_directories(${CMAKE_SOURCE_DIR})

add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

# Asset and shader paths
set(WSP_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/")
set(WSP_EDITOR_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/editor/")
set(WSP_ENGINE_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/engine-assets/")
add_definitions(-DWSP_EDITOR_ASSETS="${WSP_EDITOR_ASSETS}")
add_definitions(-DWSP_ENGINE_ASSETS="${WSP_ENGINE_ASSETS}")
add_definitions(-DWSP_ASSETS="${WSP_ASSETS}")

set(SHADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/shaders/")
add_definitions(-DSHADER_FILES="${SHADER_FILES}")

add_subdirectory(whisper)
add_subdirectory(${GAME})
add_subdirectory(shaders)

message(STATUS "Copied whisper assets into game directory")
