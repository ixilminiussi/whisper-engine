#version 450
#extension GL_GOOGLE_include_directive : require

vec2 points[6] = {{1., 1.}, {1., -1.}, {-1., 1.}, {-1., -1.}, {-1., 1.}, {1., -1.}};

#include "ubo.glsl"

layout(location = 0) out vec3 out_ray;

void main()
{
    vec4 clip = vec4(points[gl_VertexIndex], 1.0, 1.0);
    vec4 view = ubo.camera.inverseProjection * clip;
    vec3 viewDir = view.xyz / view.w;
    out_ray = (ubo.camera.inverseView * vec4(viewDir, 1.0)).xyz;
    gl_Position = vec4(clip.xy, 0.999, 1.);
}
