#version 450

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec2 v_uv;
layout(location = 4) in int v_mat_ID;

layout(location = 0) out vec3 out_normal;

struct Camera
{
    mat4 view_proj;
};

layout(set = 0, binding = 0) uniform GlobalUbo
{
    Camera camera;
}
ubo;

void main()
{
    gl_Position = ubo.camera.view_proj * vec4(v_position, 1.0);
    out_normal = v_normal;
}
