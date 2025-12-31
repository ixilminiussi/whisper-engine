#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <frost.hpp>
#include <wsp_constants.hpp>
#include <wsp_custom_imgui.hpp>
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

    void LoadDefaults();

    TextureID LoadTexture(Texture::CreateInfo const &);
    MaterialID LoadMaterial(Material::CreateInfo const &);

    void UnloadAll();

    class Node *ImportGlTF(std::filesystem::path const &relativePath);

    std::array<ubo::Material, MAX_MATERIALS> const &GetMaterialInfos() const;

    Image const *RequestImage(Image::CreateInfo const &);
    Sampler const *RequestSampler(Sampler::CreateInfo const &samplerInfo = {});
    Texture const *GetTexture(TextureID const &) const;
    Material const *GetMaterial(MaterialID const &) const;
    class Mesh const *GetMesh(MeshID const &) const;

    class StaticTextures *GetStaticTextures() const;
    class StaticTextures *GetStaticCubemaps() const;

    friend class Editor;
    friend bool frost::RenderNode<TextureID>(char const *label, TextureID *address, frost::Edit edit, float, float,
                                             float, char const *);

  protected:
    static AssetsManager *_instance;
    AssetsManager();

    class StaticTextures *_staticTextures;
    class StaticTextures *_staticCubemaps;

    std::vector<class Mesh *> _meshes;
    dod::slot_map32<Material> _materials;
    dod::slot_map32<Texture> _textures;
    dod::slot_map32<Image> _images;
    dod::slot_map32<Sampler, 128> _samplers;

    std::map<std::filesystem::path, class Node *> _nodes;

    std::map<Image::CreateInfo, dod::slot_map_key32<Image>> _imagesMap;
    std::map<Sampler::CreateInfo, dod::slot_map_key32<Sampler>> _samplersMap;

#ifndef NDEBUG
    Image const *FindImage(std::filesystem::path const &);
    ImTextureID RequestTextureID(class Image const *image);
    std::map<class Image const *, std::pair<ImTextureID, vk::ImageView>> _previewTextures;
#endif

    std::filesystem::path _fileRoot;
};

} // namespace wsp

namespace frost
{

template <>
inline bool RenderNode<TextureID>(char const *label, TextureID *address, Edit edit, float, float, float, char const *)
{
    wsp::AssetsManager *assetsManager = wsp::AssetsManager::Get();
    check(assetsManager);

    Texture const *texture = assetsManager->GetTexture(*address);

    if (texture)
    {
        Image const *image = texture->GetImage();

        if (image)
        {
            ImTextureID const imID = assetsManager->RequestTextureID(image);
            return wsp::ThumbnailButton(imID);
        }
    }
    ImGui::Text("%s", label);

    return wsp::ThumbnailButton(ICON_MS_QUESTION_MARK);
}

} // namespace frost

#endif
