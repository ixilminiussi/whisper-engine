#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <filesystem>
#include <memory>
#include <vector>

namespace wsp
{

class AssetsManager
{
  public:
    AssetsManager();
    ~AssetsManager();

    void ImportTextures(std::filesystem::path const &filepath);
    std::vector<class Mesh *> ImportMeshes(std::filesystem::path const &filepath);

    void Free();

    friend class ContentBrowser;

  protected:
    std::vector<std::unique_ptr<class Mesh>> _meshes;

    std::filesystem::path _fileRoot;
};

class ContentBrowser
{
  public:
    ContentBrowser(AssetsManager *);
    ~ContentBrowser();

    void RenderEditor();

  protected:
    AssetsManager *_assetsManager;

    std::filesystem::path _currentDirectory;
};

} // namespace wsp

#endif
