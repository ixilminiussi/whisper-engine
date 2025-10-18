#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <filesystem>
#include <map>
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

    friend class Editor;

  protected:
    std::vector<std::unique_ptr<class Mesh>> _meshes;
    std::map<std::filesystem::path, std::vector<size_t>> _importList;

    std::filesystem::path _fileRoot;
};

} // namespace wsp

#endif
