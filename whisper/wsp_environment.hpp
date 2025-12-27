#ifndef WSP_ENVIRONMENT
#define WSP_ENVIRONMENT

#include <wsp_constants.hpp>
#include <wsp_devkit.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_typedefs.hpp>

#include <glm/vec3.hpp>

#include <.generated/wsp_environment.generated.hpp>

namespace wsp
{

WCLASS()
class Environment
{
  public:
    Environment();
    ~Environment() = default;

    void PopulateUbo(ubo::Ubo *) const;

    WCLASS_BODY$Environment();

  protected:
    WREFRESH()
    void Refresh();

    WPROPERTY(eDrag)
    glm::vec2 _sunDirection{3.35, 0.87f};
    glm::vec3 _sunSource{1.f, 0.f, 0.f};
    WPROPERTY(eColor)
    glm::vec3 _sunColor{1.f};
    WPROPERTY(eDrag, 0.f, 10.f)
    float _sunIntensity{10.f};

    TextureID _skyboxTexture;
    TextureID _irradianceTexture;
};

} // namespace wsp

WGENERATED_META_DATA()

#endif
