#ifndef WSP_VIEWPORT_CAMERA
#define WSP_VIEWPORT_CAMERA

#include <.generated/wsp_viewport_camera.generated.hpp>

#include <wsp_camera.hpp>
#include <wsp_devkit.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace wsp
{

FROST()
class ViewportCamera
{
  public:
    enum PossessionMode
    {
        eReleased = 0,
        eOrbit = 1,
        eMove = 2,
    };

    ViewportCamera(glm::vec3 const &center, float distance, glm::vec2 const &rotation); // orbit builder
    ~ViewportCamera() = default;

    void SetOrbitTarget(glm::vec3 const &target);
    glm::vec3 const &GetOrbitTarget() const;

    void Zoom(float);

    glm::vec3 const &GetPosition() const;
    glm::mat4 GetViewProjection() const;

    void Possess(PossessionMode);
    void Unpossess();

    void OnMouseScroll(double dt, glm::vec2);
    void OnMouseMovement(double dt, glm::vec2);
    void OnKeyboardMovement(double dt, glm::vec2);

    void SetAspectRatio(float);

    Camera const *GetCamera() const;

    static void OnResizeCallback(void *camera, class Device const *, size_t width, size_t height);

    void Update(double dt);

    REFRESH()
    void Refresh();

  FROST_BODY$ViewportCamera

      protected : void Rotate(glm::vec2);
    void Orbit(glm::vec2);

    void RefreshView();

    PROPERTY()
    Camera _camera;

    glm::vec3 _orbitPoint;
    glm::vec3 _orbitTarget;
    float _orbitLerp;

    PROPERTY(Edit::eSlider, 0.01f, 10.f, 0.01f)
    float _orbitDistance;

    PROPERTY(Edit::eDrag)
    glm::vec3 _position; // Gets priority over orbit;

    PROPERTY(Edit::eDrag, 0.f, 0.f, 1.f)
    glm::vec2 _rotation; // Get priority over orbit; position

    PROPERTY(Edit::eSlider, 1.f, 100.f, 0.01f)
    float _movementSpeed;
    PROPERTY(Edit::eSlider, 0.1f, 2.f, 0.01f)
    glm::vec2 _mouseSensitivity;

    PossessionMode _possessionMode;
};

} // namespace wsp

using namespace wsp;
FROST_DATA$ViewportCamera;

#endif
