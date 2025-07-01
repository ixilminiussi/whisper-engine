cmake_minimum_required(VERSION 3.10)
project(Whisper CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

add_compile_definitions(VULKAN_HPP_TYPESAFE_CONVERSION)

cmake_policy(VERSION 3.12...3.31)

# -- tracy: Try to use system Tracy first
set(TRACY_ENABLE ON)
set(TRACY_NO_CONTEXT_SWITCH ON)
set(TRACY_ON_DEMAND ON)
set(TRACY_NO_SAMPLING ON)

find_package(Tracy QUIET)
if(NOT Tracy_FOUND)
  message(STATUS "System Tracy not found, fetching from GitHub")
  FetchContent_Declare(
    Tracy URL https://github.com/wolfpld/tracy/archive/refs/tags/v0.12.2.zip)
  FetchContent_MakeAvailable(Tracy)
else()
  message(STATUS "Found system Tracy")
endif()

# -- tinyxml2: Try to use system tinyxml2 first
find_package(tinyxml2 QUIET)
if(NOT tinyxml2_FOUND)
  message(STATUS "System tinyxml2 not found, fetching from GitHub")
  FetchContent_Declare(
    tinyxml2
    URL https://github.com/leethomason/tinyxml2/archive/refs/tags/11.0.0.zip)
  FetchContent_MakeAvailable(tinyxml2)
else()
  message(STATUS "Found system tinyxml2")
endif()

# -- tinyobjloader: Try to use system tinyobjloader first
find_package(tinyobjloader CONFIG QUIET)
if(NOT tinyobjloader_FOUND)
  message(STATUS "System tinyobjloader not found, fetching from GitHub")
  FetchContent_Declare(
    tinyobjloader
    URL https://github.com/tinyobjloader/tinyobjloader/archive/refs/tags/v1.0.6.zip
  )
  FetchContent_MakeAvailable(tinyobjloader)
else()
  message(STATUS "Found system tinyobjloader")
endif()

# -- Vulkan: Try to use system Vulkan first
find_package(Vulkan QUIET)
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
  message(STATUS "Found system Vulkan")
endif()

# -- GLFW: Try to use system glfw3 first
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
  message(STATUS "System glfw3 not found, fetching from GitHub")
  FetchContent_Declare(
    glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip)
  FetchContent_MakeAvailable(glfw)
else()
  message(STATUS "Found system glfw3")
endif()

# -- spdlog: Try to use system spdlog first
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  message(STATUS "System spdlog not found, fetching from GitHub")
  FetchContent_Declare(
    spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.zip)
  FetchContent_MakeAvailable(spdlog)
else()
  message(STATUS "Found system spdlog")
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
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)

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
set(WSP_FILES "${CMAKE_SOURCE_DIR}/assets/")
set(WSP_EDITOR_FILES "${CMAKE_SOURCE_DIR}/editor/")
add_definitions(-DEDITOR_FILES="${WSP_EDITOR_FILES}")

set(SHADER_FILES "${CMAKE_BINARY_DIR}/shaders/")
add_definitions(-DSHADER_FILES="${SHADER_FILES}")

add_subdirectory(whisper)
add_subdirectory(${GAME})
add_subdirectory(shaders)

# Copy assets file(GLOB WSP_ASSETS "${WSP_FILES}/*") file(COPY ${WSP_ASSETS}
# DESTINATION "${GAME_DIR}/.wsp_assets") file(MAKE_DIRECTORY
# "${GAME_DIR}/.editor")

message(STATUS "Copied whisper assets into game directory")
