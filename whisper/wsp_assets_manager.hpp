#ifndef WSP_ASSETS_MANAGER
#define WSP_ASSETS_MANAGER

#include <memory>
#include <vector>

namespace wsp
{

class AssetsManager
{
  public:
    AssetsManager();
    ~AssetsManager();

    void ImportMeshes(class Device *, std::string const &filepath, size_t *count);
    // temp
    static class Mesh *ImportMeshTmp(class Device *, std::string const &filepath);

  protected:
    std::vector<std::unique_ptr<class Mesh>> _meshes;
};

} // namespace wsp

#endif
