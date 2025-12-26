#version 450
#extension GL_GOOGLE_include_directive : require

vec2 points[6] = {{1., 1.}, {-1., 1.}, {1., -1.}, {-1., -1.}, {1., -1.}, {-1., 1.}};

layout(location = 0) out vec2 out_uv;

void main()
{
    out_uv = points[gl_VertexIndex];
    vec4 clip = vec4(out_uv, 1.0, 1.0);
    out_uv = (out_uv + 1.0) / 2.0;
    gl_Position = vec4(clip.xy, 0.999, 1.);
}
