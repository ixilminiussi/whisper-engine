#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec4 i_tangent;
layout(location = 1) in vec3 i_position;
layout(location = 2) in vec3 i_normal;
layout(location = 3) in vec3 i_color;
layout(location = 4) in vec2 i_uv;

#include "pbr_lib.glsl"

layout(location = 0) out v_info o;

#include "ubo.glsl"

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

vec2 points[6] = {{1., 1.}, {-1., 1.}, {1., -1.}, {-1., -1.}, {1., -1.}, {-1., 1.}};

void main()
{
    o.materialID = push.normalMatrix[3][3];
    o.uv = i_uv;

    // Normal mapping parameters
    vec3 w_normal = mat3(push.normalMatrix) * i_normal;

    o.v_normal = normalize(ubo.camera.view * vec4(w_normal, 0.)).xyz;

    o.m_tangent = i_tangent.xyz;
    o.m_bitangent = -cross(i_normal, o.m_tangent) * i_tangent.w;
    vec3 w_position = (push.modelMatrix * vec4(i_position, 1.)).xyz;
    o.v_position = (ubo.camera.view * vec4(w_position, 1.)).xyz;

    vec4 sc_position = ubo.light.sun.viewProjection * vec4(w_position, 1.);
    o.sc_position = sc_position.xyz / sc_position.w;

    o.c_position = ubo.camera.viewProjection * vec4(w_position, 1.);
    gl_Position = o.c_position;

    o.w_ray = normalize(ubo.camera.w_position - w_position);
}
