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

cmake_policy(VERSION 3.12...3.31)

# -- tracy: Try to use system Tracy first
set(TRACY_ENABLE ON)
set(TRACY_NO_CONTEXT_SWITCH ON)
set(TRACY_ON_DEMAND ON)
set(TRACY_NO_SAMPLING ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

find_package(Tracy QUIET)
message(CHECK_START "Find Tracy")
if(NOT Tracy_FOUND)
  message(STATUS "System Tracy not found, fetching from GitHub")
  FetchContent_Declare(
    Tracy URL https://github.com/wolfpld/tracy/archive/refs/tags/v0.12.2.zip)
  FetchContent_MakeAvailable(Tracy)
else()
  message(CHECK_PASS "Found system Tracy")
endif()

# -- tinyxml2: Try to use system tinyxml2 first
find_package(tinyxml2 QUIET)
message(CHECK_START "Find tinyxml2")
if(NOT tinyxml2_FOUND)
  message(STATUS "System tinyxml2 not found, fetching from GitHub")
  FetchContent_Declare(
    tinyxml2
    URL https://github.com/leethomason/tinyxml2/archive/refs/tags/11.0.0.zip)
  FetchContent_MakeAvailable(tinyxml2)
else()
  message(CHECK_PASS "Found system tinyxml2")
endif()

# -- glm: Try to use system glm first
find_package(glm QUIET)
message(CHECK_START "Find glm")
if(NOT glm_FOUND)
  message(STATUS "System glm not found, fetching from GitHub")
  FetchContent_Declare(
    glm URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip)
  FetchContent_MakeAvailable(glm)
else()
  message(CHECK_PASS "Found system glm")
endif()

# -- stb_image
FetchContent_Declare(
  stb_image
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master)
FetchContent_MakeAvailable(stb_image)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_image_SOURCE_DIR})

# -- tinyexr: Try to use system tinyexr first
find_package(tinyexr QUIET)
message(CHECK_START "Find tinyexr")
if(NOT tinyexr_FOUND)
  message(STATUS "System tinyexr not found, fetching from GitHub")
  FetchContent_Declare(
    tinyexr URL https://github.com/syoyo/tinyexr/archive/refs/tags/v1.0.12.zip)
  FetchContent_MakeAvailable(tinyexr)
else()
  message(CHECK_PASS "Found system tinyexr")
endif()

target_include_directories(tinyexr INTERFACE ${tinyexr_SOURCE_DIR})

# -- cgltf
FetchContent_Declare(
  cgltf URL https://github.com/jkuhlmann/cgltf/archive/refs/tags/v1.15.zip)
FetchContent_MakeAvailable(cgltf)

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

# -- Vulkan: Try to use system Vulkan first
find_package(Vulkan QUIET)
message(CHECK_START "Find Vulkan")
if(NOT Vulkan_FOUND)
  message(STATUS "System Vulkan not found, fetching from GitHub")
  FetchContent_Declare(
    VulkanHeaders
    URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.320.zip
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(VulkanHeaders)

  FetchContent_Declare(
    VulkanLoader
    URL https://github.com/KhronosGroup/Vulkan-Loader/archive/refs/tags/v1.4.320.zip
    GIT_SHALLOW TRUE)
  set(VULKAN_HEADERS_INSTALL_DIR
      ${vulkanheaders_SOURCE_DIR}/include
      CACHE PATH "" FORCE)
  FetchContent_MakeAvailable(VulkanLoader)
else()
  message(CHECK_PASS "Found system Vulkan")
endif()

# -- GLFW: Try to use system glfw3 first
find_package(glfw3 QUIET)
message(CHECK_START "Find glfw3")
if(NOT glfw3_FOUND)
  message(STATUS "System glfw3 not found, fetching from GitHub")
  FetchContent_Declare(
    glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip)
  FetchContent_MakeAvailable(glfw)
else()
  message(CHECK_PASS "Found system glfw3")
endif()

# -- spdlog: Try to use system spdlog first
find_package(spdlog QUIET)
message(CHECK_START "Find spdlog")
if(NOT spdlog_FOUND)
  message(STATUS "System spdlog not found, fetching from GitHub")
  FetchContent_Declare(
    spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.zip)
  FetchContent_MakeAvailable(spdlog)
else()
  message(CHECK_PASS "Found system spdlog")
endif()

# -- imgui
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.0-docking.zip)

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
