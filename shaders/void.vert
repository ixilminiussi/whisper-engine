#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec4 v_tangent;
layout(location = 1) in vec3 v_position;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec3 v_color;
layout(location = 4) in vec2 v_uv;

#include "ubo.glsl"

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

void main()
{
    vec3 w_position = (push.modelMatrix * vec4(v_position, 1.0)).xyz;

    gl_Position = ubo.light.sun.viewProjection * vec4(w_position, 1.0);
}
