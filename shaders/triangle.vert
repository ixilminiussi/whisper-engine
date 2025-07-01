#version 450

const vec2 OFFSETS[3] = vec2[](vec2(0.5, 0.5), vec2(0.0, -0.5), vec2(-0.5, 0.5));
const vec3 COLORS[3] = vec3[](vec3(1.0, 0., 0.), vec3(0., 1.0, 0.), vec3(0., 0., 1.0));

layout(location = 0) out vec2 outOffset;
layout(location = 1) out vec3 outColor;

void main()
{
    outOffset = OFFSETS[gl_VertexIndex];
    outColor = COLORS[gl_VertexIndex];
    gl_Position = vec4(outOffset, 0.0, 1.0);
}
