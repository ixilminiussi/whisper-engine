#include <wsp_environment.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_image.hpp>
#include <wsp_static_textures.hpp>

#include <glm/gtc/quaternion.hpp>

using namespace wsp;

Environment::Environment()
{
    AssetsManager *assetsManager = AssetsManager::Get();
    check(assetsManager);

    Image::CreateInfo skyboxImageInfo{};
    skyboxImageInfo.filepath =
        (std::filesystem::path(WSP_ENGINE_ASSETS) / std::filesystem::path("skybox.exr")).lexically_normal();
    skyboxImageInfo.format = vk::Format::eR32G32B32A32Sfloat;
    skyboxImageInfo.cubemap = true;
    Image const *skyboxImage = assetsManager->RequestImage(skyboxImageInfo);

    Texture::CreateInfo skyboxTextureInfo{};
    skyboxTextureInfo.pImage = skyboxImage;
    skyboxTextureInfo.pSampler = assetsManager->RequestSampler();
    skyboxTextureInfo.name = "skybox";

    _skyboxTexture = assetsManager->LoadTexture(skyboxTextureInfo);

    Image::CreateInfo irradianceImageInfo{};
    irradianceImageInfo.filepath =
        (std::filesystem::path(WSP_ENGINE_ASSETS) / std::filesystem::path("irradiance.exr")).lexically_normal();
    irradianceImageInfo.format = vk::Format::eR32G32B32A32Sfloat;
    irradianceImageInfo.cubemap = true;
    Image const *irradianceImage = assetsManager->RequestImage(irradianceImageInfo);

    Texture::CreateInfo irradianceTextureInfo{};
    irradianceTextureInfo.pImage = irradianceImage;
    irradianceTextureInfo.pSampler = assetsManager->RequestSampler();
    irradianceTextureInfo.name = "irradiance";

    _irradianceTexture = assetsManager->LoadTexture(irradianceTextureInfo);

    assetsManager->GetStaticCubemaps()->Push({_skyboxTexture, _irradianceTexture});

    Refresh();
}

void Environment::PopulateUbo(ubo::Ubo *ubo) const
{
    check(ubo);

    StaticTextures *staticCubemaps = AssetsManager::Get()->GetStaticCubemaps();
    check(staticCubemaps);

    ubo->light.sun.direction = glm::normalize(_sunSource);
    ubo->light.sun.color = _sunColor;
    ubo->light.sun.intensity = _sunIntensity;
    ubo->light.skybox = staticCubemaps->GetID(_skyboxTexture);
    ubo->light.irradiance = staticCubemaps->GetID(_irradianceTexture);
}

void Environment::Refresh()
{
    glm::vec4 source = glm::quat(glm::vec3{0.f, _sunDirection.x, _sunDirection.y}) * glm::vec4{1.f, 0.f, 0.f, 1.f};
    _sunSource = source;
}
