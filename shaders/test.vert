#version 450

vec2 points[6] = {{-1., -1.}, {1., -1.}, {-1., 1.}, {-1., 1.}, {1., -1.}, {1., 1.}};

void main()
{
    gl_Position = vec4(points[gl_VertexIndex % 6], 0., 1.);
}
