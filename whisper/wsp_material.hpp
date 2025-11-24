#ifndef WSP_MATERIAL
#define WSP_MATERIAL

#include <memory>
#include <string>
#include <vector>

class cgltf_material;
class cgltf_texture;

namespace wsp
{

class Material
{
  public:
    Material(cgltf_material const *material, cgltf_texture const *textures,
             std::vector<std::shared_ptr<class Texture>>);

  protected:
    int _normalTextureID;

    std::string _name;
};

} // namespace wsp

#endif
