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
    glm::vec3 position;
    float _pad0;
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
    // Light light;
    Material materials[MAX_MATERIALS];
};

} // namespace ubo

} // namespace wsp

#endif
