#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D hdrColor;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

vec3 acesToneMap(vec3 color)
{
    // ACES Filmic approximation
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    return color;
}

void main()
{
    vec3 hdrColor = texture(hdrColor, in_uv).rgb;

    vec3 toned = acesToneMap(hdrColor);
    toned = pow(toned, vec3(1.0 / 2.2)); // gamma / sRGB

    out_color = vec4(toned, 1.0);
}
