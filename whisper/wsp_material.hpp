#ifndef WSP_MATERIAL
#define WSP_MATERIAL

#include <wsp_global_ubo.hpp>
#include <wsp_texture.hpp>

#include <string>
#include <vector>

class cgltf_material;
class cgltf_texture;

namespace wsp
{

class Material
{
  public:
    static Material *BuildGlTF(class Device const *, cgltf_material const *, cgltf_texture const *pTexture,
                               std::vector<std::pair<Texture::Builder, Texture *>> *);

    Material(class Texture *albedo, class Texture *normal, class Texture *metallicRoughness, class Texture *occlusion,
             glm::vec3 albedoColor, glm::vec3 fresnelColor, float roughness, float metallic, float anisotropy,
             std::string const &name = "");

    void GetInfo(ubo::Material *) const;

    int GetID() const;
    void SetID(int ID);

  protected:
    std::string _name;

    int _ID;

    class Texture *_albedoTexture;
    class Texture *_normalTexture;
    class Texture *_metallicRoughnessTexture;
    class Texture *_occlusionTexture;

    glm::vec3 _albedoColor;
    glm::vec3 _fresnelColor;

    float _roughness;
    float _metallic;
    float _anisotropy;
};

} // namespace wsp

#endif
