#include "imgui.h"
#include <wsp_device.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_constants.hpp>
#include <wsp_editor.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_image.hpp>
#include <wsp_material.hpp>
#include <wsp_mesh.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_sampler.hpp>
#include <wsp_static_textures.hpp>
#include <wsp_texture.hpp>

#include <IconsMaterialSymbols.h>

#include <cgltf.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui_impl_vulkan.h>

using namespace wsp;

AssetsManager *AssetsManager::_instance{nullptr};

AssetsManager *AssetsManager::Get()
{
    if (!_instance)
    {
        _instance = new AssetsManager();
    }

    return _instance;
}

AssetsManager::AssetsManager() : _fileRoot{WSP_ASSETS}
{
#ifndef NDEBUG
    Device const *device = SafeDeviceAccessor::Get();
    check(device);
#endif

    _staticTextures = new StaticTextures(MAX_DYNAMIC_TEXTURES, false, "static 2d textures");
    _staticCubemaps = new StaticTextures(2, true, "static cube textures");
}

AssetsManager::~AssetsManager()
{
    UnloadAll();
}

std::string ToString(cgltf_result result)
{
    switch (result)
    {
    case cgltf_result_success:
        return "success";
    case cgltf_result_data_too_short:
        return "data_too_short";
    case cgltf_result_unknown_format:
        return "unknown_format";
    case cgltf_result_invalid_json:
        return "invalid_json";
    case cgltf_result_invalid_gltf:
        return "invalid_gltf";
    case cgltf_result_invalid_options:
        return "invalid_options";
    case cgltf_result_file_not_found:
        return "file_not_found";
    case cgltf_result_io_error:
        return "io_error";
    case cgltf_result_out_of_memory:
        return "out_of_memory";
    case cgltf_result_legacy_gltf:
        return "legacy_gltf";
    default:
        return "unknown";
    }
};

TextureID AssetsManager::LoadTexture(Texture::CreateInfo const &createInfo)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);
    return _textures.emplace(device, createInfo).raw;
}

MaterialID AssetsManager::LoadMaterial(Material::CreateInfo const &createInfo)
{
    return _materials.emplace(createInfo).raw;
}

void AssetsManager::UnloadAll()
{
    spdlog::info("AssetsManager: began termination");

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

#ifndef NDEBUG
    for (auto &[image, pair] : _previewTextures)
    {
        ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)pair.first);
        device->DestroyImageView(&pair.second);
    }
#endif

    for (auto &[info, image] : _images)
    {
        delete image;
    }
    _images.clear();

    for (auto &[key, sampler] : _samplers)
    {
        delete sampler;
    }
    _samplers.clear();

    _textures.clear();
    _materials.clear();

    for (auto const &[mesh, path] : _meshes)
    {
        delete mesh;
    }
    _meshes.clear();

    delete _staticTextures;
    delete _staticCubemaps;

    spdlog::info("AssetsManager: terminated");
}

std::vector<Mesh *> AssetsManager::ImportGlTF(std::filesystem::path const &relativePath)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ZoneScopedN("import gltf");

    std::filesystem::path const filepath = (_fileRoot / relativePath).lexically_normal();

    // if we already imported file return the meshes without reimporting
    if (std::vector<Mesh *> const drawables = _meshes.from(filepath); drawables.size() > 0)
    {
        return drawables;
    }

    cgltf_data *data = NULL;
    cgltf_options const options{};

    if (cgltf_result const result = cgltf_parse_file(&options, filepath.c_str(), &data);

        result != cgltf_result_success)
    {
        throw std::invalid_argument(
            fmt::format("AssetsManager: asset '{}' parse error ({})", filepath.string(), ToString(result)));
    }

    if (cgltf_result const result = cgltf_load_buffers(&options, data, filepath.c_str());
        result != cgltf_result_success)
    {
        throw std::invalid_argument(
            fmt::format("AssetsManager: asset '{}' parse error ({})", filepath.string(), ToString(result)));
    }

    check(data);

    // Textures BEGIN ==================================
    std::vector<Texture::CreateInfo> textureCreateInfos{};
    for (int i = 0; i < data->textures_count; i++)
    {
        Texture::CreateInfo const createInfo =
            Texture::GetCreateInfoFromGlTF(data->textures + i, filepath.parent_path());
        textureCreateInfos.push_back(createInfo);
    }

    for (int i = 0; i < data->materials_count; i++)
    {
        Material::PropagateFormatFromGlTF(data->materials + i, data->textures, &textureCreateInfos);
    }

    std::vector<TextureID> textures;
    for (Texture::CreateInfo const &createInfo : textureCreateInfos)
    {
        textures.push_back(LoadTexture(createInfo));
    }

    _staticTextures->Push(textures);
    // Textures END ====================================

    // Materials BEGIN =================================
    std::vector<MaterialID> materials{};

    for (int i = 0; i < data->materials_count; i++)
    {
        Material::CreateInfo const createInfo =
            Material::GetCreateInfoFromGlTF(data->materials + i, data->textures, textures);

        materials.push_back(LoadMaterial(createInfo));
    }

    // TODO: do something better here, this is so fucking dumb holy shit ("too bad")
    int i = 0;
    for (Material &material : _materials)
    {
        if (i < MAX_MATERIALS)
        {
            material.SetID(i);

            // material.GetInfo(&_materialInfos[i]);
        }
        else
        {
            spdlog::critical(
                "AssetsManager: reached material limit, either increase it in constants.hpp or import less materials");
            break;
        }
        i++;
    }
    // Materials END ===================================

    // Meshes BEGIN ====================================
    std::vector<Mesh *> meshes{};
    for (int i = 0; i < data->meshes_count; i++)
    {
        if (_meshes.contains(filepath))
        {
            meshes.push_back(_meshes.from(filepath)[0]);
        }
        else
        {
            Mesh *mesh = Mesh::BuildGlTF(device, data->meshes + i, data->materials, materials, true);
            _meshes[mesh] = filepath;
            meshes.push_back(mesh);
        }
    }
    // Meshes END ======================================

    cgltf_free(data);

    return meshes;
}

#ifndef NDEBUG
ImTextureID AssetsManager::GetTextureID(Image *image)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(image);

    if (_previewTextures.find(image) == _previewTextures.end())
    {
        vk::ImageViewCreateInfo imageViewInfo{};
        imageViewInfo.viewType = vk::ImageViewType::e2D;
        imageViewInfo.image = image->GetImage();
        imageViewInfo.format = vk::Format::eR8G8B8Srgb;
        imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        vk::ImageView imageView;
        device->CreateImageView(imageViewInfo, &imageView, "preview image");

        _previewTextures[image] = {(ImTextureID)ImGui_ImplVulkan_AddTexture(
                                       static_cast<VkSampler>(RequestSampler({})->GetSampler()),
                                       static_cast<VkImageView>(imageView),
                                       static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)),
                                   imageView};
    }

    return _previewTextures.at(image).first;
}
#endif

std::array<ubo::Material, MAX_MATERIALS> const &AssetsManager::GetMaterialInfos() const
{
    static std::array<ubo::Material, MAX_MATERIALS> materialInfos{};

    size_t i = 0;
    for (Material const &material : _materials)
    {
        material.GetInfo(&materialInfos[i]);
        i++;
    }
    return materialInfos;
}

Image *AssetsManager::RequestImage(Image::CreateInfo const &createInfo)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    if (_images.find(createInfo) == _images.end())
    {
        _images[createInfo] = new Image{device, createInfo};
    }

    return _images[createInfo];
}

Sampler *AssetsManager::RequestSampler(Sampler::CreateInfo const &createInfo)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    if (_samplers.find(createInfo) == _samplers.end())
    {
        _samplers[createInfo] = new Sampler{device, createInfo};
    }

    return _samplers[createInfo];
}

Texture const *AssetsManager::GetTexture(TextureID const &id) const
{
    dod::slot_map_key32<Texture> const restoredID{id};
    return _textures.get(restoredID);
}

Material const *AssetsManager::GetMaterial(MaterialID const &id) const
{
    dod::slot_map_key32<Material> const restoredID{id};
    return _materials.get(restoredID);
}

StaticTextures *AssetsManager::GetStaticTextures() const
{
    return _staticTextures;
}

StaticTextures *AssetsManager::GetStaticCubemaps() const
{
    return _staticCubemaps;
}
