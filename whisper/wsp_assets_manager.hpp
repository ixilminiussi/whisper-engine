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

    void ImportTextures(std::string const &filepath);
    void ImportMeshes(std::string const &filepath);

    void Free();

    class Drawable const *GetDrawable();

  protected:
    std::vector<std::unique_ptr<class Mesh>> _meshes;
    class Drawable *_drawable;
};

} // namespace wsp

#endif
