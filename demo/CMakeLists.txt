# demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

add_executable(demo
main.cpp)

target_link_libraries(demo PRIVATE whisper tinyxml2 imgui spdlog)
