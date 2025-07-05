#ifndef WSP_CAMERA
#define WSP_CAMERA

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace wsp
{

class Camera
{
  public:
    Camera();
    ~Camera();

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
    void SetPerspectiveProjection(float fov, float aspect, float near, float far);

    void LookTowards(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
    void LookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
    void LookXYZ(glm::vec3 position, glm::vec3 rotation);

    static void OnResizeCallback(void *camera, class Device const *, size_t width, size_t height);

    void SetAspectRatio(float);
    void SetFOV(float);
    void SetNearPlane(float);
    void SetFarPlane(float);

  private:
    enum Mode
    {
        ePerspective,
        eOrthographic
    };

    void UpdateProjection();

    glm::mat4 _projectionMatrix;
    glm::mat4 _viewMatrix;

    Mode _mode;

    float _aspectRatio, _fov; // for perspective

    float _left, _right, _top, _bottom, _near, _far; // for orthographic
};

} // namespace wsp

#endif
