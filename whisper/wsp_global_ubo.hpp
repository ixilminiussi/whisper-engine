#ifndef WSP_GLOBAL_UBO
#define WSP_GLOBAL_UBO

#include <wsp_constants.hpp>

#include <glm/matrix.hpp>
#include <wsp_constants.hpp>

namespace wsp
{

namespace ubo
{

struct Camera
{
    glm::mat4 viewProjection;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 inverseView;
    glm::mat4 inverseProjection;
    glm::vec3 position;
    float _pad0;
};

struct Sun
{
    glm::vec3 direction{1.f, 0.f, 0.f};
    float _pad0;
    glm::vec3 color{1.f};
    float intensity{10.f}; // will be color.w
    glm::mat4 viewProjection{};
};

struct Light
{
    Sun sun;
    int skybox = INVALID_ID;
    int irradiance = INVALID_ID;
    float _pad0, _pad1;
};

struct Material
{
    int albedoTex = INVALID_ID;
    int normalMap = INVALID_ID;
    int metallicRoughnessMap = INVALID_ID;
    int occlusionMap = INVALID_ID;

    glm::vec3 albedoColor{1.f};
    float _pad0;
    glm::vec3 fresnelColor{1.f};
    float _pad1;

    float roughness{0.f};
    float metallic{0.f};
    float anisotropy{0.f};
    float _pad2;
};

struct Ubo
{
    Camera camera;
    Light light;
    Material materials[MAX_MATERIALS];
};

} // namespace ubo

} // namespace wsp

#endif
