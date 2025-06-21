# demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

add_executable(demo
main.cpp)

add_executable(sizeof
sizeof.cpp)

target_link_libraries(demo PRIVATE whisper tinyxml2 imgui spdlog)
target_link_libraries(sizeof PRIVATE  ${Vulkan_LIBRARIES} spdlog)
