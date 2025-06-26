#version 450

layout(location = 0) in vec2 inOffset;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;

void main()
{
    outColor = vec4(1.0 - texture(colorSampler, inUV).rgb, 1.0);
}
