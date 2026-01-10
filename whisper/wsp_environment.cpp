#include <wsp_environment.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_drawable.hpp>
#include <wsp_image.hpp>
#include <wsp_static_textures.hpp>

#include <glm/gtc/quaternion.hpp>

#include <spdlog/spdlog.h>

using namespace wsp;

Environment::Environment(std::filesystem::path const &skyboxPath, std::filesystem::path const &irradiancePath,
                         glm::vec2 const &sunDirection, glm::vec3 const &color, float sunIntensity)
    : _skyboxPath{skyboxPath}, _irradiancePath{irradiancePath}, _sunDirection{sunDirection}, _sunColor{color},
      _sunIntensity{sunIntensity}, _shadowMapRadius{30.f}, _skyboxTexture{0}, _irradianceTexture{0}
{
    Refresh();
}

void Environment::Load()
{
    if (_skyboxTexture != 0 && _irradianceTexture != 0)
    {
        return;
    }
    AssetsManager *assetsManager = AssetsManager::Get();
    check(assetsManager);

    Image::CreateInfo skyboxImageInfo{};
    skyboxImageInfo.filepath = _skyboxPath;
    skyboxImageInfo.format = vk::Format::eR32G32B32A32Sfloat;
    skyboxImageInfo.cubemap = true;
    skyboxImageInfo.mipLevels = 8;
    Image const *skyboxImage = assetsManager->RequestImage(skyboxImageInfo);

    Texture::CreateInfo skyboxTextureInfo{};
    skyboxTextureInfo.pImage = skyboxImage;
    skyboxTextureInfo.pSampler = assetsManager->RequestSampler();
    skyboxTextureInfo.name = _skyboxPath.filename();

    _skyboxTexture = assetsManager->LoadTexture(skyboxTextureInfo);

    Image::CreateInfo irradianceImageInfo{};
    irradianceImageInfo.filepath = _irradiancePath;
    irradianceImageInfo.format = vk::Format::eR32G32B32A32Sfloat;
    irradianceImageInfo.cubemap = true;
    Image const *irradianceImage = assetsManager->RequestImage(irradianceImageInfo);

    Texture::CreateInfo irradianceTextureInfo{};
    irradianceTextureInfo.pImage = irradianceImage;
    irradianceTextureInfo.pSampler = assetsManager->RequestSampler();
    irradianceTextureInfo.name = _skyboxPath.filename();

    _irradianceTexture = assetsManager->LoadTexture(irradianceTextureInfo);

    assetsManager->GetStaticCubemaps()->Push({_skyboxTexture, _irradianceTexture});
}

void Environment::PopulateUbo(ubo::Ubo *ubo) const
{
    check(ubo);

    StaticTextures *staticCubemaps = AssetsManager::Get()->GetStaticCubemaps();
    check(staticCubemaps);

    ubo->light.sun.direction = glm::normalize(_sunSource);
    ubo->light.sun.color = _sunColor;
    ubo->light.sun.intensity = _sunIntensity;
    ubo->light.sun.viewProjection = _shadowMapCamera.GetProjection() * _shadowMapCamera.GetView();
    ubo->light.skybox = staticCubemaps->GetID(_skyboxTexture);
    ubo->light.irradiance = staticCubemaps->GetID(_irradianceTexture);
    ubo->light.rotation = glm::radians(_rotation);
}

void Environment::SetShadowMapRadius(float shadowMapRadius)
{
    _shadowMapRadius = shadowMapRadius;
    Refresh();
}

void Environment::Refresh()
{
    glm::vec3 source =
        (glm::vec3)(glm::quat(glm::vec3{0.f, _sunDirection.x, _sunDirection.y}) * glm::vec4{1.f, 0.f, 0.f, 1.f});

    glm::vec3 const up = glm::vec3{0.f, 1.f, 0.f};
    float const radiansRotation = glm::radians(_rotation);
    float const cosX = cos(radiansRotation);
    float const sinX = sin(radiansRotation);

    source =
        glm::normalize(cosX * source + (glm::cross(up, source) * sinX) + (up * glm::dot(up, source) * (1.f - cosX)));

    _sunSource = source;
    _shadowMapCamera.SetOrthographicProjection(-_shadowMapRadius, _shadowMapRadius, _shadowMapRadius, -_shadowMapRadius,
                                               -_shadowMapRadius, _shadowMapRadius);
    _shadowMapCamera.LookTowards(glm::vec3{0.f}, _sunSource);
}
