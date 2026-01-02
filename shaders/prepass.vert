#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec4 i_tangent;
layout(location = 1) in vec3 i_position;
layout(location = 2) in vec3 i_normal;
layout(location = 3) in vec3 i_color;
layout(location = 4) in vec2 i_uv;

#include "prepass_lib.glsl"

layout(location = 0) out v_info o;

#include "ubo.glsl"

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

void main()
{
    vec3 w_position = (push.modelMatrix * vec4(i_position, 1.0)).xyz;
    o.w_normal = mat3(push.normalMatrix) * i_normal;

    o.materialID = push.normalMatrix[3][3];
    o.uv = i_uv;

    vec3 w_tangent = mat3(push.normalMatrix) * i_tangent.xyz;
    vec3 w_bitangent = -cross(o.w_normal, w_tangent) * i_tangent.w;
    o.w_tangentMatrix = mat3(normalize(w_tangent), normalize(w_bitangent), normalize(o.w_normal));

    o.v_position = (ubo.camera.view * vec4(w_position, 1.0)).xyz;

    gl_Position = ubo.camera.viewProjection * vec4(w_position, 1.0);
}
