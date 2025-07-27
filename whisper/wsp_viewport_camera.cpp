#include <wsp_viewport_camera.hpp>

#include <imgui.h>

#include <spdlog/spdlog.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <spdlog/fmt/bundled/base.h>

using namespace wsp;

ViewportCamera::ViewportCamera(glm::vec3 const &orbitPoint, float distance, glm::vec2 const &rotation)
    : _orbitDistance{distance}, _rotation{rotation}, _orbitTarget{orbitPoint}, _orbitPoint{orbitPoint},
      _orbitLerp{0.1f}, _possessionMode{eReleased}, _mouseSensitivity{1.f, 0.5f}, _movementSpeed{1.f}
{
    _camera.SetLeft(-_orbitDistance);
    _camera.SetRight(_orbitDistance);
    _camera.SetTop(-_orbitDistance);
    _camera.SetBottom(_orbitDistance);
    _camera.SetNear(-_orbitDistance);
    _camera.SetFar(_orbitDistance);

    _camera.SetPerspectiveProjection(40.f, 1.f, 0.01f, 1000.f);
    RefreshView();
}

void ViewportCamera::Refresh()
{
    RefreshView();
}

void ViewportCamera::SetOrbitTarget(glm::vec3 const &target)
{
    _orbitTarget = target;
}

glm::vec3 const &ViewportCamera::GetOrbitTarget() const
{
    return _orbitTarget;
}

void ViewportCamera::Zoom(float value)
{
    if (_possessionMode != eMove)
    {
        _orbitDistance *= 1.0f - (value);
        _orbitDistance = glm::clamp(_orbitDistance, 0.05f, 400.f);

        _camera.SetLeft(-_orbitDistance);
        _camera.SetRight(_orbitDistance);
        _camera.SetTop(-_orbitDistance);
        _camera.SetBottom(_orbitDistance);
        _camera.SetNear(-_orbitDistance);
        _camera.SetFar(_orbitDistance);
    }
}

glm::vec3 const &ViewportCamera::GetPosition() const
{
    return _position;
}

glm::mat4 ViewportCamera::GetViewProjection() const
{
    return _camera.GetProjection() * _camera.GetView();
}

void ViewportCamera::Rotate(glm::vec2 value)
{
    value *= _mouseSensitivity;
    value.y = glm::clamp(value.y, -(90.f - 0.01f) - _rotation.y, (90.f - 0.01f) - _rotation.y);
    _rotation += value;

    glm::vec3 const up = glm::vec3{0.f, -1.f, 0.f};

    glm::quat qYaw = glm::angleAxis(glm::radians(_rotation.x), up);

    glm::vec3 forward = qYaw * glm::vec3{1.f, 0.f, 0.f};
    glm::vec3 right = glm::normalize(glm::cross(up, forward));

    glm::quat qPitch = glm::angleAxis(glm::radians(_rotation.y), right);

    forward = glm::normalize(qPitch * forward);

    _orbitPoint = _position - forward * _orbitDistance;
    _orbitTarget = _orbitPoint;
}

void ViewportCamera::Orbit(glm::vec2 value)
{
    value *= _mouseSensitivity;
    value.y = glm::clamp(value.y, -(90.f - 0.01f) - _rotation.y, (90.f - 0.01f) - _rotation.y);
    _rotation += value;
    _rotation.x = glm::mod(_rotation.x + 360.f, 360.f);
}

void ViewportCamera::SetAspectRatio(float aspectRatio)
{
    _camera.SetAspectRatio(aspectRatio);
}

Camera const *ViewportCamera::GetCamera() const
{
    return &_camera;
}

void ViewportCamera::OnResizeCallback(void *editorCamera, class Device const *, size_t width, size_t height)
{
    assert(editorCamera);
    reinterpret_cast<ViewportCamera *>(editorCamera)->_camera.SetAspectRatio((float)width / (float)height);
}

void ViewportCamera::RefreshView()
{
    // yaw and pitch, determine forward
    // orbit position is fixed
    // orbit distance is fixed
    // position is determined by previous three arguments

    glm::vec3 const up = glm::vec3{0.f, -1.f, 0.f};

    glm::quat qYaw = glm::angleAxis(glm::radians(_rotation.x), up);

    glm::vec3 forward = qYaw * glm::vec3{1.f, 0.f, 0.f};
    glm::vec3 right = glm::normalize(glm::cross(up, forward));

    glm::quat qPitch = glm::angleAxis(glm::radians(_rotation.y), right);

    forward = glm::normalize(qPitch * forward);

    _position = _orbitPoint + forward * _orbitDistance;

    _camera.LookAt(_position, _orbitPoint);
}

void ViewportCamera::Possess(PossessionMode possessionMode)
{
    _possessionMode = possessionMode;
}

void ViewportCamera::Unpossess()
{
    _possessionMode = eReleased;
}

void ViewportCamera::OnMouseScroll(double dt, glm::vec2 value)
{
    if (_possessionMode == eMove)
    {
        value.x = glm::clamp(value.x, -1.f, 1.f);
        _movementSpeed *= 1.0f + 0.1f * dt * value.x;
        _movementSpeed = glm::clamp(_movementSpeed, .1f, 2.f);
    }
}

void ViewportCamera::OnMouseMovement(double dt, glm::vec2 value)
{
    if (_possessionMode == eOrbit)
    {
        Orbit(glm::radians(-value * (float)dt));
    }
    else
    {
        if (_possessionMode == eMove)
        {
            Rotate(glm::radians(-value * (float)dt));
        }
    }
}

void ViewportCamera::OnKeyboardMovement(double dt, glm::vec2 value)
{
    if (_possessionMode == eMove)
    {
        RefreshView();
        value = glm::normalize(value);
        value *= _movementSpeed * (float)dt;

        glm::vec3 forward = _camera.GetForward();
        glm::vec3 right = _camera.GetRight();

        _orbitPoint += forward * 0.01f * -value.y;
        _orbitPoint += right * 0.01f * -value.x;

        _orbitTarget = _orbitPoint;
        RefreshView();
    }
}

void ViewportCamera::Update(double dt)
{
    // movement inertia (slowly bring viewportCamera to target)
    if (glm::distance(_orbitPoint, _orbitTarget) > glm::epsilon<float>())
    {
        glm::vec3 direction = _orbitTarget - _orbitPoint;
        direction *= _orbitLerp * dt;
        _orbitPoint += direction;
    }

    RefreshView();
}
