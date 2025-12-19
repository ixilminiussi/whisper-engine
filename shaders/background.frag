#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 in_ndc;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D textures[];
layout(set = 2, binding = 0) uniform samplerCube cubemaps[];

#include "ubo.glsl"

void main()
{
    vec3 w_position = (inverse(ubo.camera.projection) * vec4(normalize(in_ndc), 1.0)).xyz;
    vec3 V = normalize(ubo.camera.position.xyz - w_position); // view vec

    int textureID = int(ubo.light.sun.direction.w);
    if (textureID == INVALID_ID)
    {
        discard;
    }

    out_color = vec4(texture(cubemaps[textureID], V).rgb, 1.0);

    return;
}
