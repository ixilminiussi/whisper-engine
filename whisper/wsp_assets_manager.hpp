#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <wsp_custom_types.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace wsp
{

class AssetsManager
{
  public:
    AssetsManager(class StaticTextureAllocator *staticTextureAllocator = nullptr);
    ~AssetsManager();

    std::vector<std::shared_ptr<class Texture>> ImportTextures(std::filesystem::path const &filepath);
    std::vector<std::shared_ptr<class Mesh>> ImportMeshes(std::filesystem::path const &filepath);

    void Free();

    friend class Editor;

  protected:
    dictionary<std::shared_ptr<class Mesh>, std::filesystem::path> _meshes;
    dictionary<std::shared_ptr<class Texture>, std::filesystem::path> _textures;

    std::filesystem::path _fileRoot;

    class StaticTextureAllocator *_staticTextureAllocator;

    bool _freed;
};

} // namespace wsp

#endif
