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
#include <spdlog/spdlog.h>

#include <filesystem>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui_impl_vulkan.h>

using namespace wsp;

AssetsManager::AssetsManager() : _fileRoot{WSP_ASSETS}
{
#ifndef NDEBUG
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _defaultSampler =
        Sampler::Builder{}.Name("preview sampler").AddressMode(vk::SamplerAddressMode::eClampToEdge).Build(device);
#endif
}

AssetsManager::~AssetsManager()
{
    spdlog::info("AssetsManager: began termination");

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    delete _defaultSampler;

#ifndef NDEBUG
    for (auto &[image, pair] : _previewTextures)
    {
        ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)pair.first);
        device->DestroyImageView(&pair.second);
    }
#endif

    for (auto const &[mesh, path] : _meshes)
    {
        delete mesh;
    }
    _meshes.clear();

    for (Texture *texture : _textures)
    {
        delete texture;
    }
    _textures.clear();

    for (Sampler *sampler : _samplers)
    {
        delete sampler;
    }
    _samplers.clear();

    for (auto &[image, path] : _images)
    {
        delete image;
    }
    _images.clear();

    spdlog::info("AssetsManager: terminated");
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

    // images are "created" but the actual vk::Image depends on the image's usage by materials
    std::vector<Image *> images{};
    for (int i = 0; i < data->images_count; i++)
    {
        std::filesystem::path const imagePath = (_fileRoot / data->images[i].uri).lexically_normal();
        if (_images.contains(imagePath))
        {
            images.push_back(_images.from(imagePath).at(0));
        }
        else
        {
            Image *image = Image::BuildGlTF(device, data->images + i, _fileRoot);
            _images[image] = imagePath;
            images.push_back(image);
        }
    }

    std::vector<Sampler *> samplers{};
    for (int i = 0; i < data->samplers_count; i++)
    {
        Sampler *sampler = Sampler::Builder{}.GlTF(data->samplers + i).Build(device);
        _samplers.push_back(sampler);
        samplers.push_back(sampler);
    }

    if (samplers.size() == 0)
    {
        samplers.push_back(_defaultSampler); // make sure there is always ONE sampler available
    }

    // similarly we don't finish building the textures yet, the materials themselves build the textures to pick the
    // appropriate image format
    std::vector<std::pair<Texture::Builder, Texture *>> textureBuilders{};
    for (int i = 0; i < data->textures_count; i++)
    {
        Texture::Builder textureBuilder =
            Texture::Builder{}.GlTF(data->textures + i, data->images, images, data->samplers, samplers);
        ;

        textureBuilders.push_back({textureBuilder, nullptr});
    }

    std::vector<Material *> materials{};
    for (int i = 0; i < data->materials_count; i++)
    {
        Material *material = Material::BuildGlTF(device, data->materials + i, data->textures, &textureBuilders);

        materials.push_back(material);
    }

    for (auto &[builder, texture] : textureBuilders)
    {
        if (texture)
        {
            RenderManager::Get()->GetStaticTextures()->Push({texture});
            _textures.push_back(texture);
        }
    }

    for (Material *material : materials)
    {
        _materials.push_back(material);
        material->SetID(_materials.size() - 1);

        if (ensure(_materials.size() < MAX_MATERIALS))
        {
            material->GetInfo(&_materialInfos[_materials.size() - 1]);
        }
        else
        {
            spdlog::critical(
                "AssetsManager: reached material limit, either increase it in constants.hpp or import less materials");
        }
    }

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

    cgltf_free(data);

    return meshes;
}

std::vector<class Mesh *> AssetsManager::ImportPNG(std::filesystem::path const &filepath)
{
}

#ifndef NDEBUG
ImTextureID AssetsManager::GetTextureID(Image *image)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(image);

    if (_previewTextures.find(image) == _previewTextures.end())
    {
        check(image->AskForVariant(vk::Format::eR8G8B8Srgb));
        vk::ImageViewCreateInfo imageViewInfo{};
        imageViewInfo.viewType = vk::ImageViewType::e2D;
        imageViewInfo.image = image->GetImage(vk::Format::eR8G8B8Srgb);
        imageViewInfo.format = vk::Format::eR8G8B8Srgb;
        imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        vk::ImageView imageView;
        device->CreateImageView(imageViewInfo, &imageView, "preview image");

        _previewTextures[image] = {(ImTextureID)ImGui_ImplVulkan_AddTexture(
                                       static_cast<VkSampler>(_defaultSampler->GetSampler()),
                                       static_cast<VkImageView>(imageView),
                                       static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)),
                                   imageView};
    }

    return _previewTextures.at(image).first;
}
#endif

std::array<ubo::Material, MAX_MATERIALS> const &AssetsManager::GetMaterialInfos() const
{
    return _materialInfos;
}
