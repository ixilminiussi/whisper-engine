#ifndef WSP_EDITOR_CAMERA
#define WSP_EDITOR_CAMERA

#include "wsp_camera.hpp"

#include <glm/vec3.hpp>

namespace wsp
{

class EditorCamera
{
  public:
    EditorCamera();
    ~EditorCamera() = default;

    void SetOrbitPoint(glm::vec3 const &newPoint);

    void ZoomIn(float);
    void ZoomOut(float);

    void RotateYaw(float);
    void RotatePitch(float);

    void OrbitYaw(float);
    void OrbitPitch(float);

    void SetAspectRatio(float);

    Camera const *GetCamera() const;

    static void OnResizeCallback(void *camera, class Device const *, size_t width, size_t height);

  protected:
    Camera _camera;

    glm::vec3 _orbitPoint;
    glm::vec3 _position;
    float _orbitDistance;
};

} // namespace wsp

#endif
