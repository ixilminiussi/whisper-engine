#version 450

const vec2 OFFSETS[6] =
    vec2[](vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0));

layout(location = 0) out vec2 outOffset;
layout(location = 1) out vec2 outUV;

void main()
{
    outOffset = OFFSETS[gl_VertexIndex];
    outUV = (outOffset + 1.0) / 2.0;
    gl_Position = vec4(outOffset, 0.0, 1.0);
}
