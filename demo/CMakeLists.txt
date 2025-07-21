# demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

add_executable(demo main.cpp)

target_link_libraries(demo PRIVATE whisper tinyxml2 imgui spdlog)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS *.hpp *.h)

set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

foreach(HEADER ${HEADERS})
  get_filename_component(HEADER_NAME ${HEADER} NAME)
  set(OUTPUT_FILE "${GENERATED_DIR}/${HEADER_NAME}")

  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tooling/frost.py ${HEADER}
            > ${OUTPUT_FILE}
    DEPENDS ${HEADER} ${CMAKE_SOURCE_DIR}/tooling/frost.py
    COMMENT "Generating metadata for ${HEADER_NAME}"
    VERBATIM)

  list(APPEND GENERATED_OUTPUTS ${OUTPUT_FILE})
endforeach()

add_custom_target(generate_metadata_${demo} ALL DEPENDS ${GENERATED_OUTPUTS})
