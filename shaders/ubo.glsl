struct Camera
{
    mat4 view_proj;
};

layout(set = 0, binding = 0) uniform GlobalUbo
{
    Camera camera;
}
ubo;
