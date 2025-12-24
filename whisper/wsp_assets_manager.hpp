#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <wsp_constants.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_image.hpp>
#include <wsp_material.hpp>
#include <wsp_sampler.hpp>
#include <wsp_typedefs.hpp>
#include <wsp_types/dictionary.hpp>
#include <wsp_types/slot_map.hpp>

#include <filesystem>
#include <map>
#include <vector>

#ifndef NDEBUG
#include <imgui.h>
#include <vulkan/vulkan.hpp>
#endif

namespace wsp
{

struct ImageKeyChain
{
    bool operator()(Image::CreateInfo const &a, Image::CreateInfo const &b) const
    {
        return std::tie(a.filepath) < std::tie(b.filepath);
    }
};

WCLASS()
class AssetsManager
{
  public:
    static AssetsManager *Get();
    ~AssetsManager();

    TextureID LoadTexture(Texture::CreateInfo const &);
    MaterialID LoadMaterial(Material::CreateInfo const &);

    void UnloadAll();

    std::vector<class Mesh *> ImportGlTF(std::filesystem::path const &relativePath);

    std::array<ubo::Material, MAX_MATERIALS> const &GetMaterialInfos() const;

    Image *RequestImage(Image::CreateInfo const &);
    Sampler *RequestSampler(Sampler::CreateInfo const &samplerInfo = {});
    Texture const *GetTexture(TextureID const &) const;
    Material const *GetMaterial(MaterialID const &) const;

    class StaticTextures *GetStaticTextures() const;
    class StaticTextures *GetStaticCubemaps() const;

    friend class Editor;

  protected:
    static AssetsManager *_instance;
    AssetsManager();

    class StaticTextures *_staticTextures;
    class StaticTextures *_staticCubemaps;

    dod::slot_map32<Texture> _textures;
    dod::slot_map32<Material> _materials;
    // std::array<ubo::Material, MAX_MATERIALS> _materialInfos;

    dictionary<class Mesh *, std::filesystem::path, 1024> _meshes;

    std::map<Image::CreateInfo, Image *> _images;
    std::map<Sampler::CreateInfo, Sampler *> _samplers;

#ifndef NDEBUG
    ImTextureID GetTextureID(class Image *image);
    std::map<class Image *, std::pair<ImTextureID, vk::ImageView>> _previewTextures;
#endif

    std::filesystem::path _fileRoot;
};

} // namespace wsp

#endif
