#include "wsp_constants.hpp"
#include <wsp_material.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_devkit.hpp>
#include <wsp_static_textures.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_texture.hpp>

#include <stdexcept>

#include <cgltf.h>

using namespace wsp;

void Material::PropagateFormatFromGlTF(cgltf_material const *material, cgltf_texture const *pTexture,
                                       std::vector<Texture::CreateInfo> *createInfos)
{
    check(material);

    // Helper lambda to get or build texture with format validation
    auto populateInfo = [&](cgltf_texture const *gltfTexture, vk::Format format) -> void {
        if (!pTexture || createInfos->empty() || !gltfTexture)
        {
            return;
        }

        long const index = gltfTexture - pTexture;
        if (!inbetween<long>(index, 0, (long)createInfos->size()))
        {
            throw std::invalid_argument(fmt::format("Material: gltf material has invalid texture index: {}", index));
        }

        Texture::CreateInfo &createInfo = createInfos->at(index);

        createInfo.imageInfo.format = format;
        createInfo.deferredImageCreation = true;
    };

    std::string const name = material->name ? material->name : "";

    if (material->has_pbr_metallic_roughness)
    {
        populateInfo(material->pbr_metallic_roughness.metallic_roughness_texture.texture, vk::Format::eR8G8B8A8Unorm);
        populateInfo(material->pbr_metallic_roughness.base_color_texture.texture, vk::Format::eR8G8B8A8Srgb);
    }
    if (material->has_specular)
    {
        populateInfo(material->specular.specular_color_texture.texture, vk::Format::eR8G8B8A8Unorm);
    }

    populateInfo(material->normal_texture.texture, vk::Format::eR8G8B8A8Unorm);
    populateInfo(material->occlusion_texture.texture, vk::Format::eR8G8B8A8Unorm);
}

Material::CreateInfo Material::GetCreateInfoFromGlTF(cgltf_material const *material, cgltf_texture const *pTexture,
                                                     std::vector<TextureID> const &textureIDs)
{
    check(material);

    CreateInfo createInfo{};

    // Helper lambda to get or build texture with format validation
    auto getTexture = [&](cgltf_texture const *gltfTexture) -> TextureID {
        if (!pTexture || textureIDs.empty() || !gltfTexture)
        {
            return {};
        }

        uint32_t const index = gltfTexture - pTexture;
        check(textureIDs.size() > index);

        return textureIDs.at(index);
    };

    createInfo.name = material->name ? material->name : "";

    if (material->has_pbr_metallic_roughness)
    {
        createInfo.metallic = material->pbr_metallic_roughness.metallic_factor;
        createInfo.roughness = material->pbr_metallic_roughness.roughness_factor;
        createInfo.albedoColor = *(glm::vec3 *)(material->pbr_metallic_roughness.base_color_factor);

        createInfo.metallicRoughness = getTexture(material->pbr_metallic_roughness.metallic_roughness_texture.texture);

        createInfo.albedo = getTexture(material->pbr_metallic_roughness.base_color_texture.texture);
    }
    if (material->has_specular)
    {
        createInfo.specular = getTexture(material->specular.specular_color_texture.texture);
        createInfo.specularColor = *(glm::vec3 *)(material->specular.specular_color_factor);
    }
    if (material->has_ior)
    {
        createInfo.ior = material->ior.ior;
    }

    if (material->has_anisotropy)
    {
        createInfo.anisotropy = material->anisotropy.anisotropy_strength;
    }

    createInfo.normal = getTexture(material->normal_texture.texture);
    createInfo.occlusion = getTexture(material->occlusion_texture.texture);

    return createInfo;
}

Material::Material(CreateInfo const &createInfo)
    : _ID{INVALID_ID}, _albedoTexture{createInfo.albedo}, _normalTexture{createInfo.normal},
      _metallicRoughnessTexture{createInfo.metallicRoughness}, _occlusionTexture{createInfo.occlusion},
      _specularTexture{createInfo.specular}, _albedoColor{createInfo.albedoColor},
      _fresnelColor{createInfo.fresnelColor}, _specularColor{createInfo.specularColor},
      _roughness{createInfo.roughness}, _metallic{createInfo.metallic}, _anisotropy{createInfo.anisotropy},
      _ior{createInfo.ior}, _name{createInfo.name}
{
    spdlog::debug("Material: <{}>", _name);
}

void Material::GetInfo(ubo::Material *info) const
{
    auto GetID = [](TextureID id) -> int {
        return static_cast<int>(AssetsManager::Get()->GetStaticTextures()->GetID(id));
    };
    check(info);
    info->albedoTex = GetID(_albedoTexture);
    info->normalMap = GetID(_normalTexture);
    info->metallicRoughnessMap = GetID(_metallicRoughnessTexture);
    info->occlusionMap = GetID(_occlusionTexture);
    info->specularTex = GetID(_specularTexture);
    info->albedoColor = _albedoColor;
    info->fresnelColor = _fresnelColor;
    info->specularColor = _specularColor;
    info->ior = _ior;
    info->roughness = _roughness;
    info->metallic = _metallic;
    info->anisotropy = _anisotropy;
}

int Material::GetID() const
{
    return _ID;
}

void Material::SetID(int ID)
{
    _ID = ID;
}

std::string const &Material::GetName() const
{
    return _name;
}
