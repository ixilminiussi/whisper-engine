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
#include <wsp_scene.hpp>
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
    _staticTextures = new StaticTextures{MAX_DYNAMIC_TEXTURES, false, "static 2d textures"};
    _staticNoises = new StaticTextures{2, false, "static 2d noises"};
    _staticCubemaps = new StaticTextures{10, true, "static cube textures"};

    _meshes.reserve(1024);
}

void AssetsManager::LoadDefaults()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    Sampler::CreateInfo wrapSamplerInfo{};
    wrapSamplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    wrapSamplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    wrapSamplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    Image::CreateInfo rgbNoiseImageInfo{};
    rgbNoiseImageInfo.filepath =
        (std::filesystem::path(WSP_ENGINE_ASSETS) / std::filesystem::path("rgb-noise.png")).lexically_normal();
    rgbNoiseImageInfo.format = vk::Format::eR8G8B8A8Srgb;
    Image const *rgbNoiseImage = RequestImage(rgbNoiseImageInfo);

    Texture::CreateInfo rgbNoiseTextureInfo{};
    rgbNoiseTextureInfo.pImage = rgbNoiseImage;
    rgbNoiseTextureInfo.pSampler = RequestSampler(wrapSamplerInfo);
    rgbNoiseTextureInfo.name = "rgb noise";

    TextureID const rgbNoiseTexture = LoadTexture(rgbNoiseTextureInfo);

    _staticNoises->Push({rgbNoiseTexture});

    Image::CreateInfo LUTImageInfo{};
    LUTImageInfo.filepath =
        (std::filesystem::path(WSP_ENGINE_ASSETS) / std::filesystem::path("brdfLUT.png")).lexically_normal();
    LUTImageInfo.format = vk::Format::eR8G8B8A8Srgb;
    Image const *LUTImage = RequestImage(LUTImageInfo);

    Texture::CreateInfo LUTTextureInfo{};
    LUTTextureInfo.pImage = LUTImage;
    LUTTextureInfo.pSampler = RequestSampler();
    LUTTextureInfo.name = "lut";

    TextureID const LUTTexture = LoadTexture(LUTTextureInfo);

    _staticNoises->Push({LUTTexture});

    Image::CreateInfo missingImageInfo{};
    missingImageInfo.filepath =
        (std::filesystem::path(WSP_ENGINE_ASSETS) / std::filesystem::path("missing-texture.png")).lexically_normal();
    missingImageInfo.format = vk::Format::eR8G8B8A8Srgb;
    Image const *missingImage = RequestImage(missingImageInfo);

    Texture::CreateInfo missingTextureInfo{};
    missingTextureInfo.pImage = missingImage;
    missingTextureInfo.pSampler = RequestSampler();
    missingTextureInfo.name = "missing";
    TextureID const missingTexture = LoadTexture(missingTextureInfo);

    std::vector<TextureID> missingTextures{};
    missingTextures.resize(_staticTextures->GetSize(), missingTexture);
    _staticTextures->Push(missingTextures);
    _staticTextures->Clear();

    missingImageInfo.filepath =
        (std::filesystem::path(WSP_ENGINE_ASSETS) / std::filesystem::path("missing-texture.png")).lexically_normal();
    missingImageInfo.format = vk::Format::eR8G8B8A8Srgb;
    missingImageInfo.cubemap = true;
    Image const *missingCubemapImage = RequestImage(missingImageInfo);

    missingTextureInfo.pImage = missingCubemapImage;
    missingTextureInfo.pSampler = RequestSampler();
    missingTextureInfo.name = "missing";
    TextureID const missingCubemapTexture = LoadTexture(missingTextureInfo);

    std::vector<TextureID> missingCubemapTextures{};
    missingCubemapTextures.resize(_staticCubemaps->GetSize(), missingCubemapTexture);
    _staticCubemaps->Push(missingCubemapTextures);
    _staticCubemaps->Clear();
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

    for (auto &[image, pair] : _previewTextures)
    {
        ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)pair.first);
        device->DestroyImageView(&pair.second);
    }

    _samplers.clear();
    _images.clear();
    _textures.clear();
    _materials.clear();

    auto it = _meshes.begin();
    while (it != _meshes.end())
    {
        Mesh *mesh = *it;
        it = _meshes.erase(it);
        delete mesh;
    }
    _meshes.clear();

    _imagesMap.clear();
    _samplersMap.clear();

    for (auto const &[path, node] : _scenes)
    {
        delete node;
    }
    _meshes.clear();

    delete _staticTextures;
    delete _staticNoises;
    delete _staticCubemaps;

    spdlog::info("AssetsManager: terminated");
}

Scene *AssetsManager::ImportGlTF(std::filesystem::path const &relativePath)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ZoneScopedN("import gltf");

    std::filesystem::path const filepath = (_fileRoot / relativePath).lexically_normal();

    // if we already imported file return the meshes without reimporting
    if (auto it = _scenes.find(filepath); it != _scenes.end())
    {
        return it->second;
    }

    cgltf_data *data = NULL;
    cgltf_options const options{};

    if (cgltf_result const result = cgltf_parse_file(&options, filepath.c_str(), &data);

        result != cgltf_result_success)
    {
        throw std::invalid_argument(
            fmt::format("AssetsManager: asset '{}' parse error ({})", filepath.filename().string(), ToString(result)));
    }

    if (cgltf_result const result = cgltf_load_buffers(&options, data, filepath.c_str());
        result != cgltf_result_success)
    {
        throw std::invalid_argument(
            fmt::format("AssetsManager: asset '{}' parse error ({})", filepath.filename().string(), ToString(result)));
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
    textures.reserve(data->textures_count);
    for (Texture::CreateInfo const &createInfo : textureCreateInfos)
    {
        textures.push_back(LoadTexture(createInfo));
    }

    _staticTextures->Push(textures);
    // Textures END ====================================

    // Materials BEGIN =================================
    std::vector<MaterialID> materials{};
    materials.reserve(data->materials_count);
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
    std::vector<MeshID> meshes{};
    meshes.reserve(data->meshes_count);
    for (int i = 0; i < data->meshes_count; i++)
    {
        Mesh *mesh = Mesh::BuildGlTF(device, data->meshes + i, data->materials, materials, false);

        _meshes.push_back(mesh);
        meshes.push_back(_meshes.size() - 1);
    }
    // Meshes END ======================================

    // Nodes BEGIN =====================================
    Scene *scene = Scene::BuildGlTF(data->scene, data->meshes, meshes);
    _scenes[filepath] = scene;
    // Nodes END =======================================

    cgltf_free(data);

    return scene;
}

Image const *AssetsManager::FindImage(std::filesystem::path const &filepath)
{
    for (auto &[createInfo, imageKey] : _imagesMap)
    {
        if (createInfo.filepath.compare(filepath) == 0)
        {
            return _images.get(imageKey);
        }
    }

    return nullptr;
}

ImTextureID AssetsManager::RequestTextureID(Image const *image)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(image);

    if (_previewTextures.find(image) == _previewTextures.end())
    {
        vk::ImageViewCreateInfo imageViewInfo{};
        imageViewInfo.viewType = vk::ImageViewType::e2D;
        imageViewInfo.image = image->GetImage();
        imageViewInfo.format = image->GetFormat();
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

Image const *AssetsManager::RequestImage(Image::CreateInfo const &createInfo)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    if (createInfo.format == vk::Format::eUndefined)
    {
        spdlog::warn("AssetsManager: requesting an image of undefined format will always lead to the "
                     "creation of a new image");
    }

    if (auto it = _imagesMap.find(createInfo); it != _imagesMap.end())
    {
        return _images.get(it->second);
    }

    dod::slot_map_key32<Image> const key = _images.emplace(device, createInfo);
    Image::CreateInfo trueInfo = createInfo;
    Image const *image = _images.get(key);
    trueInfo.format = image->GetFormat();
    _imagesMap[trueInfo] = key;

    return image;
}

Sampler const *AssetsManager::RequestSampler(Sampler::CreateInfo const &createInfo)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    if (_samplersMap.find(createInfo) != _samplersMap.end())
    {
        return _samplers.get(_samplersMap.at(createInfo));
    }

    dod::slot_map_key32<Sampler> const key = _samplers.emplace(device, createInfo);
    _samplersMap[createInfo] = key;

    Sampler const *sampler = _samplers.get(key);

    return sampler;
}

Texture const *AssetsManager::GetTexture(TextureID const &id) const
{
    if (id == 0)
    {
        return nullptr;
    }

    dod::slot_map_key32<Texture> const restoredID{id};
    return _textures.get(restoredID);
}

Material const *AssetsManager::GetMaterial(MaterialID const &id) const
{
    if (id == 0)
    {
        return nullptr;
    }

    dod::slot_map_key32<Material> const restoredID{id};
    return _materials.get(restoredID);
}

Mesh const *AssetsManager::GetMesh(MeshID const &id) const
{
    if (id == INVALID_ID)
    {
        return nullptr;
    }

    return _meshes.at(id);
}

StaticTextures *AssetsManager::GetStaticTextures() const
{
    return _staticTextures;
}

StaticTextures *AssetsManager::GetStaticNoises() const
{
    return _staticNoises;
}

StaticTextures *AssetsManager::GetStaticCubemaps() const
{
    return _staticCubemaps;
}
