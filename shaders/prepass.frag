#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "prepass_lib.glsl"

layout(location = 0) in v_info i;

layout(location = 0) out vec4 out_normal;

layout(set = 1, binding = 0) uniform sampler2D sTextures[];

#include "ubo.glsl"

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

void main()
{
    Material material = ubo.materials[int(i.materialID + 0.01)];

    int normalTexID = material.normalTex;
    vec3 normal = normalTexID != INVALID_ID
                      ? i.w_tangentMatrix * normalize(texture(sTextures[normalTexID], i.uv).rgb * 2. - 1.)
                      : i.w_normal;

    float depth = -i.v_position.z;

    out_normal = vec4(normalize(normal), depth);
}
