#version 450

layout(location = 0) in vec2 inOffset;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo
{
    float a;
}
ubo;

void main()
{
    outColor = vec4(ubo.a, 1.0, 1.0, 1.0);
}
