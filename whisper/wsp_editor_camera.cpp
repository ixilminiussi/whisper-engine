#include <wsp_devkit.hpp>
#include <wsp_editor_camera.hpp>

#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

#include <spdlog/spdlog.h>

using namespace wsp;

EditorCamera::EditorCamera() : _camera{}, _orbitDistance{1.f}, _orbitPoint{0.f}
{
    _camera.SetPerspectiveProjection(40.f, 1.f, 0.01f, 1000.f);

    _position = glm::vec3{1.f, -1.f, 1.f};
    _position = glm::normalize(_position - _orbitPoint) * _orbitDistance;
    _position += _orbitPoint;

    _camera.LookAt(_position, _orbitPoint);
}

void EditorCamera::SetOrbitPoint(glm::vec3 const &newPoint)
{
    glm::vec3 offset = _orbitPoint - _position;
    _orbitPoint = newPoint;

    _position = _orbitPoint - offset;

    _camera.LookAt(_position, _orbitPoint);
}

void EditorCamera::ZoomIn(float value)
{
    _orbitDistance *= 1.0f - (0.05f * value);
    _orbitDistance = glm::min(_orbitDistance, 0.05f);

    glm::vec3 offset = _orbitPoint - _position;

    _position = _orbitPoint - glm::normalize(offset) * _orbitDistance;
}

void EditorCamera::ZoomOut(float value)
{
    _orbitDistance *= 1.0f + (0.05f * value);

    glm::vec3 offset = _orbitPoint - _position;

    _position = _orbitPoint - glm::normalize(offset) * _orbitDistance;
}

void EditorCamera::RotateYaw(float value)
{
    glm::vec3 up = _camera.GetUp();

    glm::quat rotator = glm::angleAxis(value, up);

    glm::vec3 forward = _orbitPoint - _position;
    forward = rotator * forward;

    _orbitPoint = _position + forward;

    _camera.LookAt(_position, _orbitPoint);
}

void EditorCamera::RotatePitch(float value)
{
    glm::vec3 right = _camera.GetRight();

    glm::quat rotator = glm::angleAxis(value, right);

    glm::vec3 forward = _orbitPoint - _position;
    forward = rotator * forward;

    _orbitPoint = _position + forward;

    _camera.LookAt(_position, _orbitPoint);
}

void EditorCamera::OrbitYaw(float value)
{
    glm::vec3 up = {0.f, -1.f, 0.f};

    glm::quat rotator = glm::angleAxis(value, up);

    glm::vec3 offset = _orbitPoint - _position;
    offset = rotator * offset;

    _position = _orbitPoint - offset;

    _camera.LookAt(_position, _orbitPoint);
}

void EditorCamera::OrbitPitch(float value)
{
    glm::vec3 right = _camera.GetRight();

    glm::quat rotator = glm::angleAxis(value, right);

    glm::vec3 offset = _orbitPoint - _position;
    offset = rotator * offset;

    _position = _orbitPoint - offset;

    _camera.LookAt(_position, _orbitPoint);
}

void EditorCamera::SetAspectRatio(float aspectRatio)
{
    _camera.SetAspectRatio(aspectRatio);
}

Camera const *EditorCamera::GetCamera() const
{
    return &_camera;
}

void EditorCamera::OnResizeCallback(void *editorCamera, class Device const *, size_t width, size_t height)
{
    check(editorCamera);
    reinterpret_cast<EditorCamera *>(editorCamera)->_camera.SetAspectRatio((float)width / (float)height);
}
