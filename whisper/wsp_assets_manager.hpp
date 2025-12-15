#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <wsp_constants.hpp>
#include <wsp_custom_types.hpp>
#include <wsp_global_ubo.hpp>

#include <filesystem>
#include <map>
#include <vector>

#ifndef NDEBUG
#include <imgui.h>
#include <vulkan/vulkan.hpp>
#endif

namespace wsp
{

class AssetsManager
{
  public:
    AssetsManager();
    ~AssetsManager();

    std::vector<class Mesh *> ImportGlTF(std::filesystem::path const &filepath);
    std::vector<class Mesh *> ImportPNG(std::filesystem::path const &filepath);

    std::array<ubo::Material, MAX_MATERIALS> const &GetMaterialInfos() const;

    friend class Editor;

  protected:
    dictionary<class Mesh *, std::filesystem::path> _meshes;
    std::vector<class Texture *> _textures;
    dictionary<class Image *, std::filesystem::path> _images;
    std::vector<class Sampler *> _samplers;
    std::vector<class Material *> _materials;
    std::array<ubo::Material, MAX_MATERIALS> _materialInfos;

    class Sampler *_defaultSampler;

#ifndef NDEBUG
    ImTextureID GetTextureID(class Image *image);
    std::map<class Image *, std::pair<ImTextureID, vk::ImageView>> _previewTextures;
#endif

    std::filesystem::path _fileRoot;
};

} // namespace wsp

#endif
