#include <wsp_assets_manager.hpp>
#include <wsp_mesh.hpp>

#include <spdlog/spdlog.h>

#include <cgltf.h>

using namespace wsp;

AssetsManager::AssetsManager()
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

void AssetsManager::ImportMeshes(Device *device, std::string const &filepath, size_t *count)
{
    cgltf_data *data = NULL;
    cgltf_options const options{};

    if (cgltf_result const result = cgltf_parse_file(&options, filepath.c_str(), &data);

        result != cgltf_result_success)
    {
        spdlog::error("AssetsManager: asset {} parse error ({})", filepath, ToString(result));
        return;
    }

    if (cgltf_result const result = cgltf_load_buffers(&options, data, filepath.c_str());
        result != cgltf_result_success)
    {
        spdlog::error("AssetsManager: asset {} parse error ({})", filepath, ToString(result));
        return;
    }

    *count = data->meshes_count;

    spdlog::info("AssetsManager: importing new assets from {}", filepath);

    for (int i = 0; i < data->meshes_count; i++)
    {
        _meshes.emplace_back(new Mesh(device, &data->meshes[i]));
    }

    cgltf_free(data);
}

class Mesh *AssetsManager::ImportMeshTmp(class Device *device, std::string const &filepath)
{
    cgltf_data *data = NULL;
    cgltf_options const options{};

    if (cgltf_result const result = cgltf_parse_file(&options, filepath.c_str(), &data);

        result != cgltf_result_success)
    {
        spdlog::error("AssetsManager: asset {} parse error ({})", filepath, ToString(result));
        return nullptr;
    }

    if (cgltf_result const result = cgltf_load_buffers(&options, data, filepath.c_str());
        result != cgltf_result_success)
    {
        spdlog::error("AssetsManager: asset {} parse error ({})", filepath, ToString(result));
        return nullptr;
    }

    Mesh *mesh = new Mesh{device, &data->meshes[0]};

    cgltf_free(data);

    return mesh;
}
