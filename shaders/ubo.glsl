#define INVALID_ID -1

struct Camera
{
    mat4 viewProjection;
    mat4 view;
    mat4 projection;
    mat4 inverseView;
    mat4 inverseProjection;
    vec3 w_position;
};

struct Sun
{
    vec3 direction;
    vec4 color;
    mat4 viewProjection;
};

struct Light
{
    Sun sun;
    int skyboxTex;
    int irradianceTex;
    float rotation;
};

struct Material
{
    int albedoTex;
    int normalTex;
    int metallicRoughnessTex;
    int occlusionTex;

    vec3 albedoColor;
    vec3 fresnelColor;
    vec4 specularColor; // ior on a

    float roughness;
    float metallic;
    float anisotropy;

    int specularTex;
};

layout(set = 0, binding = 0) uniform Ubo
{
    Camera camera;
    Light light;
    Material materials[500];
}
ubo;
