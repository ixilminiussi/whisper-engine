#version 450
#extension GL_GOOGLE_include_directive : require

vec2 points[6] = {{-1., -1.}, {-1., 1.}, {1., -1.}, {-1., 1.}, {1., 1.}, {1., -1.}};

#include "ubo.glsl"

layout(location = 0) out vec3 out_ndc;

void main()
{
    out_ndc = vec3(points[gl_VertexIndex], .99999);
    gl_Position = vec4(out_ndc, 1.);
}
