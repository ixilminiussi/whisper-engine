#version 450

const vec2 OFFSETS[3] = vec2[](vec2(0.5, 0.5), vec2(0.0, -0.5), vec2(-0.5, 0.5));

layout(location = 0) out vec2 outOffset;

void main()
{
    outOffset = OFFSETS[gl_VertexIndex];
    gl_Position = vec4(outOffset, 0.0, 1.0);
}
