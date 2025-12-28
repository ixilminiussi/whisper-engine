#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec4 v_tangent;
layout(location = 1) in vec3 v_position;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec3 v_color;
layout(location = 4) in vec2 v_uv;

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
    float materialID = push.normalMatrix[3][3];
    o.uv = vec3(v_uv, materialID);

    // Normal mapping parameters
    o.w_tangent = mat3(push.normalMatrix) * v_tangent.xyz;
    o.w_normal = mat3(push.normalMatrix) * v_normal;
    o.w_bitangent = -cross(o.w_normal, o.w_tangent) * v_tangent.w;

    o.m_tangent = v_tangent.xyz;
    o.m_bitangent = -cross(v_normal, o.m_tangent) * v_tangent.w;
    vec3 w_position = (push.modelMatrix * vec4(v_position, 1.0)).xyz;

    vec4 sc_position = ubo.light.sun.viewProjection * vec4(w_position, 1.0);
    o.sc_position = sc_position.xyz / sc_position.w;

    gl_Position = ubo.camera.viewProjection * vec4(w_position, 1.0);

    o.w_ray = normalize(ubo.camera.w_position - w_position);
}
