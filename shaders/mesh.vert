#version 450

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec3 v_color;
layout(location = 4) in vec2 v_uv;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;

#include "ubo.glsl"

void main()
{
    gl_Position = ubo.camera.view_proj * vec4(v_position, 1.0);
    out_normal = v_normal;
    out_uv = v_uv;
}
