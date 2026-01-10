#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 in_ray;

layout(set = 1, binding = 0) uniform samplerCube sCubemaps[];

#include "ubo.glsl"

layout(location = 0) out vec4 out_color;

vec3 getSky(in vec3 ray)
{
    int skyboxTexID = ubo.light.skyboxTex;
    const vec3 up = vec3(0., -1., 0.);
    const float cosX = cos(ubo.light.rotation);
    const float sinX = sin(ubo.light.rotation);
    const vec3 rotatedRay = normalize(cosX * ray + (cross(up, ray) * sinX) + (up * dot(up, ray) * (1. - cosX)));
    return skyboxTexID != INVALID_ID ? texture(sCubemaps[skyboxTexID], rotatedRay).rgb : ubo.light.sun.color.rgb;
}

void main()
{
    out_color = vec4(getSky(normalize(in_ray)), 1.);

    // for debugging
    // if (dot(normalize(in_ray), -normalize(ubo.light.sun.direction)) > .99999)
    // {
    //     out_color = vec4(1., 0., 0., 1.);
    // }
}
