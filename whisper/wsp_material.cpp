#include <wsp_material.hpp>

#include <wsp_devkit.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_texture.hpp>

#include <stdexcept>

#include <cgltf.h>

using namespace wsp;

Material *Material::BuildGlTF(Device const *device, cgltf_material const *material, cgltf_texture const *pTexture,
                              std::vector<std::pair<Texture::Builder, Texture *>> *textureBuilders)
{
    check(material);

    // Helper lambda to get or build texture with format validation
    auto getOrBuildTexture = [&](cgltf_texture const *gltfTexture, vk::Format format) -> Texture * {
        if (!pTexture || textureBuilders->empty() || !gltfTexture)
        {
            return nullptr;
        }

        long const index = gltfTexture - pTexture;
        if (!inbetween<long>(index, 0, textureBuilders->size()))
        {
            throw std::invalid_argument(fmt::format("Material: gltf material has invalid texture index: {}", index));
        }

        auto &[builder, texture] = textureBuilders->at(index);

        if (texture)
        {
            check(texture->GetFormat() == format);
        }
        else
        {
            texture = builder.Format(format).Build(device);
        }

        return texture;
    };

    Texture *albedoTexture = nullptr;
    Texture *normalTexture = nullptr;
    Texture *metallicRoughnessTexture = nullptr;
    Texture *occlusionTexture = nullptr;

    glm::vec3 albedoColor{1.0f};
    glm::vec3 fresnelColor{1.0f};
    float roughness = 1.0f;
    float metallic = 0.0f;
    float anisotropy = 0.0f;

    std::string const name = material->name ? material->name : "";

    if (material->has_pbr_metallic_roughness)
    {
        metallic = material->pbr_metallic_roughness.metallic_factor;
        roughness = material->pbr_metallic_roughness.roughness_factor;
        albedoColor = *(glm::vec3 *)(material->pbr_metallic_roughness.base_color_factor);

        metallicRoughnessTexture = getOrBuildTexture(
            material->pbr_metallic_roughness.metallic_roughness_texture.texture, vk::Format::eR8G8B8Unorm);

        albedoTexture =
            getOrBuildTexture(material->pbr_metallic_roughness.base_color_texture.texture, vk::Format::eR8G8B8Srgb);
    }

    if (material->has_anisotropy)
    {
        anisotropy = material->anisotropy.anisotropy_strength;
    }

    normalTexture = getOrBuildTexture(material->normal_texture.texture, vk::Format::eR8G8B8Unorm);

    occlusionTexture = getOrBuildTexture(material->occlusion_texture.texture, vk::Format::eR8G8B8Unorm);

    return new Material(albedoTexture, normalTexture, metallicRoughnessTexture, occlusionTexture, albedoColor,
                        fresnelColor, roughness, metallic, anisotropy, name);
}

Material::Material(Texture *albedo, Texture *normal, Texture *metallicRoughness, Texture *occlusion,
                   glm::vec3 albedoColor, glm::vec3 fresnelColor, float roughness, float metallic, float anisotropy,
                   std::string const &name)
    : _ID{INVALID_ID}, _albedoTexture{albedo}, _normalTexture{normal}, _metallicRoughnessTexture{metallicRoughness},
      _occlusionTexture{occlusion}, _albedoColor{albedoColor}, _fresnelColor{fresnelColor}, _roughness{roughness},
      _metallic{metallic}, _anisotropy{anisotropy}, _name{name}
{
}

void Material::GetInfo(ubo::Material *info) const
{
    check(info);
    info->albedoTex = _albedoTexture ? _albedoTexture->GetID() : INVALID_ID;
    info->normalMap = _normalTexture ? _normalTexture->GetID() : INVALID_ID;
    info->metallicRoughnessMap = _metallicRoughnessTexture ? _metallicRoughnessTexture->GetID() : INVALID_ID;
    info->occlusionMap = _occlusionTexture ? _occlusionTexture->GetID() : INVALID_ID;
    info->albedoColor = _albedoColor;
    info->fresnelColor = _fresnelColor;
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
