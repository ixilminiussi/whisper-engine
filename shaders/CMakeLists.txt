# shaders/CMakeLists.txt
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(SHADER_FILES
    triangle.vert triangle.frag
    postprocess.vert postprocess.frag
)

set(COMPILED_SHADERS)

foreach(SHADER ${SHADER_FILES})
    set(INPUT_FILE "${SHADER_SOURCE_DIR}/${SHADER}")
    set(OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${SHADER}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND glslc "${INPUT_FILE}" -o "${OUTPUT_FILE}"
        DEPENDS ${INPUT_FILE}
        COMMENT "Compiling shader ${SHADER}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${OUTPUT_FILE})
endforeach()

add_custom_target(CompileShaders ALL
    DEPENDS ${COMPILED_SHADERS}
)

add_dependencies(whisper CompileShaders)

add_definitions(-DSHADER_FILES="./shaders/")


