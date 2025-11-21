#include <wsp_device.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_editor.hpp>
#include <wsp_graph.hpp>
#include <wsp_mesh.hpp>
#include <wsp_texture.hpp>

#include <IconsMaterialSymbols.h>

#include <cgltf.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace wsp;

AssetsManager::AssetsManager(StaticTextureAllocator *staticTextureAllocator)
    : _fileRoot{WSP_ASSETS}, _freed{false}, _staticTextureAllocator{staticTextureAllocator}
{
}

AssetsManager::~AssetsManager()
{
    if (!_freed)
    {
        spdlog::critical("AssetsManager: forgot to Free before deletion");
    }
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

void AssetsManager::Free()
{
    spdlog::info("AssetsManager: began termination");

    if (_freed)
    {
        spdlog::error("AssetsManager: already freed");
        return;
    }

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    for (auto &[mesh, path] : _meshes)
    {
        mesh->Free(device);
    }

    _meshes.clear();

    for (auto &[texture, path] : _textures)
    {
        texture->Free(device);
    }

    _meshes.clear();

    _freed = true;

    spdlog::info("AssetsManager: terminated");
}

std::vector<std::shared_ptr<Texture>> AssetsManager::ImportTextures(std::filesystem::path const &relativePath)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    std::filesystem::path const filepath = (_fileRoot / relativePath).lexically_normal();

    // if we already imported file then return its pointer
    if (std::vector<std::shared_ptr<Texture>> const existing = _textures.from(filepath); existing.size() > 0)
    {
        return existing;
    }

    int w, h, channels;
    stbi_uc *pixels = stbi_load(filepath.c_str(), &w, &h, &channels, STBI_rgb_alpha);

    if (pixels == nullptr)
    {
        throw std::invalid_argument(fmt::format("AssetsManager: asset '{}' couldn't be imported", filepath.string()));
    }

    std::shared_ptr<Texture> const texture = std::make_shared<Texture>(device, pixels, w, h, channels);
    _textures[texture] = filepath;

    spdlog::info("AssetsManager: loaded new texture from '{}'", filepath.string());

    stbi_image_free(pixels);

    return {texture};
}

std::vector<std::shared_ptr<Mesh>> AssetsManager::ImportMeshes(std::filesystem::path const &relativePath)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    std::filesystem::path const filepath = (_fileRoot / relativePath).lexically_normal();

    // if we already imported file return the meshes without reimporting
    if (std::vector<std::shared_ptr<Mesh>> const drawables = _meshes.from(filepath); drawables.size() > 0)
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

    for (int i = 0; i < data->images_count; i++)
    {
        cgltf_image const &image = data->images[i];
        if (image.uri && strncmp(image.uri, "data:", 5) != 0) // we're referencing a path
        {
            std::filesystem::path const textureAbsolutePath = (filepath.parent_path() / image.uri).lexically_normal();
            std::filesystem::path const textureRelativePath = std::filesystem::relative(textureAbsolutePath, _fileRoot);
            std::vector<std::shared_ptr<Texture>> const textures = ImportTextures(textureRelativePath);

            if (_staticTextureAllocator)
            {
                _staticTextureAllocator->BindStaticTexture(0, textures[0].get());
            }
        }
        else if (image.uri && strncmp(image.uri, "data:", 5) == 0) // we're on base64
        {
        }
        else if (image.buffer_view) // embedded in buffer
        {
        }
        else // invalid or extension-defined
        {
        }
    }

    for (int i = 0; i < data->meshes_count; i++)
    {
        _meshes[std::make_unique<Mesh>(device, &data->meshes[i])] = filepath;
    }

    std::vector<std::shared_ptr<Mesh>> drawables = _meshes.from(filepath);

    cgltf_free(data);

    spdlog::info("AssetsManager: loaded new meshes from '{}'", filepath.string());

    return drawables;
}
