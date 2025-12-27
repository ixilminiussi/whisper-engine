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

WSTRUCT()
struct Sun
{
    WPROPERTY()
    glm::vec3 direction{1.f, 0.f, 0.f};
    WPROPERTY()
    glm::vec3 color{1.f};
    WPROPERTY()
    float intensity{10.f};
};

WCLASS()
class Environment
{
  public:
    Environment();
    ~Environment() = default;

    void PopulateUbo(ubo::Ubo *) const;

    WCLASS_BODY$Environment();

  protected:
    WPROPERTY()
    Sun sun;

    TextureID _skyboxTexture;
    TextureID _irradianceTexture;
};

} // namespace wsp

WGENERATED_META_DATA()

#endif
