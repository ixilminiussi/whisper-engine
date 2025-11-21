#version 450

layout(location = 0) in vec3 in_ws_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D textures[];

#include "ubo.glsl"

void main()
{
    out_color = vec4(texture(textures[0], in_uv).rgb, 1.0);
}
