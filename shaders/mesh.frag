#version 450

layout(location = 0) in vec3 in_ws_normal;

layout(location = 0) out vec4 out_color;

#include "ubo.glsl"

void main()
{
    out_color = vec4((in_ws_normal + 1.0) / 2.0, 1.0);
}
