#ifndef WSP_MATERIAL
#define WSP_MATERIAL

#include <wsp_devkit.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_texture.hpp>
#include <wsp_typedefs.hpp>

#include <string>
#include <vector>

#include <.generated/wsp_material.generated.hpp>

class cgltf_material;
class cgltf_texture;

namespace wsp
{

WCLASS()
class Material
{
  public:
    struct CreateInfo
    {
        TextureID albedo{};
        TextureID normal{};
        TextureID metallicRoughness{};
        TextureID occlusion{};
        TextureID specular{};

        glm::vec3 albedoColor{};
        glm::vec3 fresnelColor{};
        glm::vec3 specularColor{.04};

        float roughness{0.};
        float metallic{0.};
        float anisotropy{0.};

        std::string name{""};
    };

    static void PropagateFormatFromGlTF(cgltf_material const *, cgltf_texture const *pTexture,
                                        std::vector<Texture::CreateInfo> *);
    static CreateInfo GetCreateInfoFromGlTF(cgltf_material const *, cgltf_texture const *pTexture,
                                            std::vector<TextureID> const &);

    Material(CreateInfo const &);

    void GetInfo(ubo::Material *) const;

    int GetID() const;
    void SetID(int ID);

    WCLASS_BODY$Material();

    std::string const &GetName() const;

  protected:
    std::string const _name;

    int _ID;

    WPROPERTY()
    TextureID _albedoTexture;
    WPROPERTY()
    TextureID _normalTexture;
    WPROPERTY()
    TextureID _metallicRoughnessTexture;
    WPROPERTY()
    TextureID _occlusionTexture;
    WPROPERTY()
    TextureID _specularTexture;

    WPROPERTY(eColor)
    glm::vec3 _albedoColor;
    WPROPERTY(eColor)
    glm::vec3 _fresnelColor;
    WPROPERTY(eColor)
    glm::vec3 _specularColor;

    WPROPERTY(eSlider, 0.f, 1.f)
    float _roughness;
    WPROPERTY(eSlider, 0.f, 1.f)
    float _metallic;
    WPROPERTY(eSlider, 0.f, 1.f)
    float _anisotropy;
};

} // namespace wsp

WGENERATED_META_DATA()

#endif
