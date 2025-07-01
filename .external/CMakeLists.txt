# .external/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

include(FetchContent)

add_compile_definitions(VULKAN_HPP_TYPESAFE_CONVERSION)

# -- tracy: Try to use system Tracy first
set(TRACY_ENABLE ON)
set(TRACY_NO_CONTEXT_SWITCH ON)
set(TRACY_NO_SAMPLING ON)
FetchContent_Declare(
    Tracy
    GIT_REPOSITORY https://github.com/wolfpld/Tracy.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Tracy
     GIT_TAG master
)
FetchContent_MakeAvailable(Tracy)

# -- tinyxml2: Try to use system tinyxml2 first
find_package(tinyxml2 QUIET)
if(NOT tinyxml2_FOUND) 
  message(STATUS "System tinyxml2 not found, fetching from GitHub")
  FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2
  )
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
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
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
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG        v1.3.296    
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(VulkanHeaders)

  FetchContent_Declare(
    VulkanLoader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG        v1.3.296
    GIT_SHALLOW    TRUE
  )
  set(VULKAN_HEADERS_INSTALL_DIR ${vulkanheaders_SOURCE_DIR}/include CACHE PATH "" FORCE)
  FetchContent_MakeAvailable(VulkanLoader)
else()
  message(STATUS "Found system Vulkan")
endif()

# -- GLFW: Try to use system glfw3 first
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
  message(STATUS "System glfw3 not found, fetching from GitHub")
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8 
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw
  )
  FetchContent_MakeAvailable(glfw)
else()
  message(STATUS "Found system glfw3")
endif()

# -- spdlog: Try to use system spdlog first
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  message(STATUS "System spdlog not found, fetching from GitHub")
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
  )
  FetchContent_MakeAvailable(spdlog)
else()
  message(STATUS "Found system spdlog")
endif()

# -- imgui
find_package(imgui QUIET)
if(NOT imgui_FOUND)
  message(STATUS "System imgui not found, fetching from GitHub")
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui
  )
  FetchContent_MakeAvailable(imgui)
else()
  message(STATUS "Found system imgui")
endif()

add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp
  imgui/backends/imgui_impl_glfw.cpp  
  imgui/backends/imgui_impl_vulkan.cpp  
)

target_include_directories(imgui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends>  
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui-filebrowser>
)

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# -- imguizmo
find_package(imguizmo QUIET)
if(NOT imguizmo_FOUND)
  message(STATUS "System imguizmo not found, fetching from GitHub")
  FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imguizmo
  )
  FetchContent_MakeAvailable(imguizmo)
else()
  message(STATUS "Found system imguizmo")
endif()

add_library(imguizmo STATIC
    imguizmo/GraphEditor.cpp
    imguizmo/ImCurveEdit.cpp
    imguizmo/ImGradient.cpp
    imguizmo/ImGuizmo.cpp
    imguizmo/ImSequencer.cpp
)

target_include_directories(imguizmo PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imguizmo>
)

target_link_libraries(imguizmo PUBLIC imgui)
