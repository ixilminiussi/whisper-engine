#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 in_ray;

layout(set = 1, binding = 0) uniform samplerCube cubemaps[];

#include "ubo.glsl"

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 color = texture(cubemaps[0], normalize(in_ray)).rgb;

    out_color = vec4(color, 1.0);
}
