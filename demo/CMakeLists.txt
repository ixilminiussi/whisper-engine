# demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

add_library(demo
main.cpp)

target_link_libraries(demo PUBLIC wsp tinyxml2 imgui spdlog)
