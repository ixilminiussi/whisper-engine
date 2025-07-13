# shaders/CMakeLists.txt shaders/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Where your .vert/.frag live:
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# Where you want the .spv to end up:
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# 1) Glob all .vert/.frag under SOURCE_DIR
file(GLOB SHADER_FILES "${SHADER_SOURCE_DIR}/*.vert"
     "${SHADER_SOURCE_DIR}/*.frag")

set(COMPILED_SHADERS)

foreach(SHADER_PATH IN LISTS SHADER_FILES)
  get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)

  set(OUTPUT_PATH "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

  add_custom_command(
    OUTPUT "${OUTPUT_PATH}"
    COMMAND glslc -I "${SHADER_SOURCE_DIR}" "${SHADER_PATH}" -o "${OUTPUT_PATH}"
    DEPENDS "${SHADER_PATH}"
    COMMENT "Compiling shader ${SHADER_NAME}"
    VERBATIM)

  list(APPEND COMPILED_SHADERS "${OUTPUT_PATH}")
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${COMPILED_SHADERS})
