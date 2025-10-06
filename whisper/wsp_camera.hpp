#ifndef WSP_CAMERA
#define WSP_CAMERA

#include <.generated/wsp_camera.generated.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <wsp_devkit.hpp>

namespace wsp
{

FROST()
class Camera
{
  public:
    Camera();
    ~Camera() = default;

    glm::mat4 const &GetProjection() const;
    glm::mat4 const &GetView() const;

    float GetFOV() const;
    float GetNearPlane() const;
    float GetFarPlane() const;
    glm::vec3 GetPosition() const;

    glm::vec3 GetUp() const;
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;

    float GetPitch() const;
    float GetYaw() const;

    void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
    void SetPerspectiveProjection(float fov, float aspect, float nearPlane, float farPlane);

    void LookTowards(glm::vec3 const &position, glm::vec3 const &direction,
                     glm::vec3 const &up = glm::vec3{0.f, -1.f, 0.f});
    void LookAt(glm::vec3 const &position, glm::vec3 const &target, glm::vec3 const &up = glm::vec3{0.f, -1.f, 0.f});
    void LookXYZ(glm::vec3 const &position, glm::vec3 const &rotation);

    void SetAspectRatio(float);
    void SetFOV(float);
    void SetNearPlane(float);
    void SetFarPlane(float);
    void SetLeft(float);
    void SetRight(float);
    void SetTop(float);
    void SetBottom(float);
    void SetNear(float);
    void SetFar(float);

    REFRESH()
    void Refresh();

    FROST_BODY$Camera;

  protected:
    enum Mode
    {
        ePerspective,
        eOrthographic
    };

    void UpdateProjection();

    glm::mat4 _projectionMatrix;
    glm::mat4 _viewMatrix;

    Mode _mode;

    float _aspectRatio; // for perspective
    PROPERTY(eSlider, 10.f, 170.f)
    float _fov;
    PROPERTY(eInput)
    float _nearPlane;
    PROPERTY(eInput)
    float _farPlane;

    float _left, _right, _top, _bottom, _near, _far; // for orthographic
};

} // namespace wsp

FROST_DATA$Camera

#endif
