#define INVALID_ID -1

struct Camera
{
    mat4 viewProjection;
    mat4 view;
    mat4 projection;
    vec3 position;
};

struct Sun
{
    int skybox;
};

struct Light
{
    Sun sun;
};

struct Material
{
    int albedoTex;
    int normalTex;
    int metallicRoughnessTex;
    int occlusionTex;

    vec3 albedoColor;
    vec3 fresnelColor;

    float roughness;
    float metallic;
    float anisotropy;
};

layout(set = 0, binding = 0) uniform Ubo
{
    Camera camera;
    // Light light;
    Material materials[500];
}
ubo;
