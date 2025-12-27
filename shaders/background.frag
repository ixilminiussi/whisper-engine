#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 in_ray;

layout(set = 1, binding = 0) uniform samplerCube cubemaps[];

#include "ubo.glsl"

layout(location = 0) out vec4 out_color;

vec3 getSky(in vec3 ray)
{
    int skyboxTexID = ubo.light.skyboxTex;
    return skyboxTexID != INVALID_ID ? texture(cubemaps[skyboxTexID], ray).rgb : ubo.light.sun.color.rgb;
}

void main()
{
    out_color = vec4(getSky(normalize(in_ray)), 1.0);
}
