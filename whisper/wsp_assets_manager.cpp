#include "wsp_engine.hpp"
#include <wsp_device.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_editor.hpp>
#include <wsp_mesh.hpp>

#include <IconsMaterialSymbols.h>

#include <cgltf.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <stdexcept>

using namespace wsp;

AssetsManager::AssetsManager() : _fileRoot{WSP_ASSETS}
{
}

AssetsManager::~AssetsManager()
{
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
    Device const *device = SafeDeviceAccessor::Get();

    for (std::unique_ptr<Mesh> &mesh : _meshes)
    {
        mesh->Free(device);
        mesh.release();
    }

    _meshes.clear();
}

void AssetsManager::ImportTextures(std::filesystem::path const &filepath)
{
}

std::vector<Mesh *> AssetsManager::ImportMeshes(std::filesystem::path const &relativePath)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    std::filesystem::path const filepath = (_fileRoot / relativePath).lexically_normal();

    // if we already imported file return the meshes without reimporting
    if (_importList.find(filepath) != _importList.end())
    {
        std::vector<class Mesh *> _drawables;
        for (size_t const index : _importList.at(filepath))
        {
            _drawables.push_back(_meshes.at(index).get());
        }
        return _drawables;
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
            std::filesystem::path const baseDir = std::filesystem::path(filepath).parent_path();
            std::filesystem::path const fullPath = baseDir / image.uri;
            ImportTextures(fullPath);
        }
    }

    spdlog::info("AssetsManager: importing new assets from '{}'", filepath.string());

    std::vector<class Mesh *> _drawables;
    for (int i = 0; i < data->meshes_count; i++)
    {
        _meshes.emplace_back(new Mesh(device, &data->meshes[i]));
        _importList[filepath].push_back(_meshes.size() - 1);

        _drawables.push_back(_meshes.at(_meshes.size() - 1).get());
    }

    cgltf_free(data);

    return _drawables;
}
