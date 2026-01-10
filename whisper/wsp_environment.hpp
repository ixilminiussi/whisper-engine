#ifndef WSP_ENVIRONMENT
#define WSP_ENVIRONMENT

#include <wsp_camera.hpp>
#include <wsp_constants.hpp>
#include <wsp_devkit.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_typedefs.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <filesystem>

#include <.generated/wsp_environment.generated.hpp>

namespace wsp
{

WCLASS()
class Environment
{
  public:
    Environment(std::filesystem::path const &skyboxPath, std::filesystem::path const &irradiancePath,
                glm::vec2 const &sunDirection, glm::vec3 const &color, float sunIntensity);
    ~Environment() = default;

    void Load();

    void PopulateUbo(ubo::Ubo *) const;
    void SetShadowMapRadius(float);

    WCLASS_BODY$Environment();

  protected:
    WREFRESH()
    void Refresh();

    glm::vec2 _sunDirection;
    glm::vec3 _sunSource;

    WPROPERTY(eSlider, 1.f, 100.f)
    float _shadowMapRadius;
    Camera _shadowMapCamera;

    WPROPERTY(eColor)
    glm::vec3 _sunColor;
    WPROPERTY(eSlider, 0.f, 10.f)
    float _sunIntensity;

    TextureID _skyboxTexture;
    TextureID _irradianceTexture;

    WPROPERTY(eSlider, 0.f, 360.f, 1.f)
    float _rotation;

    std::filesystem::path _skyboxPath;
    std::filesystem::path _irradiancePath;
};

} // namespace wsp

WGENERATED_META_DATA()

#endif
