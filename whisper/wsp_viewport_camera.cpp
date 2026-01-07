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
      _orbitLerp{0.1f}, _possessionMode{eReleased}, _mouseSensitivity{2.f, 1.5f}, _movementSpeed{10.f},
      _viewDistance{1000.f}
{
    SetOrbitDistance(_orbitDistance);

    _camera.SetFOV(40.f);
    _camera.SetNearPlane(1.0);
}

void ViewportCamera::Refresh()
{
    _camera.SetFarPlane(_viewDistance);
    _camera.SetNearPlane(_viewDistance / 2000.f);

    RefreshView();
    _camera.Refresh();
}

void ViewportCamera::SetOrbitDistance(float distance)
{
    _orbitDistance = distance;
    _orbitDistance = std::max(_orbitDistance, 0.05f);

    _camera.SetLeft(-_orbitDistance);
    _camera.SetRight(_orbitDistance);
    _camera.SetTop(-_orbitDistance);
    _camera.SetBottom(_orbitDistance);
    _camera.SetNear(-_orbitDistance);
    _camera.SetFar(_orbitDistance);
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
    _orbitDistance *= 1.0f - value;
    _orbitDistance = std::max(_orbitDistance, 0.05f);

    SetOrbitDistance(_orbitDistance);
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

    glm::quat const qYaw = glm::angleAxis(glm::radians(_rotation.x), up);

    glm::vec3 forward = qYaw * glm::vec3{1.f, 0.f, 0.f};
    glm::vec3 const right = glm::normalize(glm::cross(up, forward));

    glm::quat const qPitch = glm::angleAxis(glm::radians(_rotation.y), right);

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

void ViewportCamera::OnResizeCallback(void *editorCamera, uint32_t width, uint32_t height)
{
    check(editorCamera);
    reinterpret_cast<ViewportCamera *>(editorCamera)->_camera.SetAspectRatio((float)width / (float)height);
}

void ViewportCamera::RefreshView()
{
    // yaw and pitch, determine forward
    // orbit position is fixed
    // orbit distance is fixed
    // position is determined by previous three arguments

    glm::vec3 const up = glm::vec3{0.f, -1.f, 0.f};

    glm::quat const qYaw = glm::angleAxis(glm::radians(_rotation.x), up);

    glm::vec3 forward = qYaw * glm::vec3{1.f, 0.f, 0.f};
    glm::vec3 const right = glm::normalize(glm::cross(up, forward));

    glm::quat const qPitch = glm::angleAxis(glm::radians(_rotation.y), right);

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
        value.y = glm::clamp(value.y, -1.f, 1.f);
        _movementSpeed *= 1.0f + (0.1f * value.y);
        _movementSpeed = glm::clamp(_movementSpeed, .1f, 10000.f);
    }
    else if (_possessionMode == eOrbit)
    {
        value.y = glm::clamp(value.y, -1.f, 1.f);
        Zoom(value.y * 0.05f);
    }
}

void ViewportCamera::OnMouseMovement(double dt, glm::vec2 value)
{
    dt = std::min(dt, 0.008);

    value.x *= -1;
    if (_possessionMode == eOrbit)
    {
        Orbit(value * _mouseSensitivity * (float)dt);
    }
    else if (_possessionMode == eMove)
    {
        Rotate(value * _mouseSensitivity * (float)dt);
    }
}

void ViewportCamera::OnKeyboardMovement(double dt, glm::vec2 value)
{
    if (_possessionMode == eMove)
    {
        RefreshView();
        value = glm::normalize(value);
        value *= _movementSpeed * (float)dt;

        glm::vec3 const forward = _camera.GetForward();
        glm::vec3 const right = _camera.GetRight();

        _orbitPoint += forward * 0.01f * -value.y;
        _orbitPoint += right * 0.01f * value.x;

        _orbitTarget = _orbitPoint;
    }
}

void ViewportCamera::Lift(double dt, int value)
{
    if (_possessionMode == eMove)
    {
        glm::vec3 constexpr up = glm::vec3{0.f, 1.f, 0.f};

        _orbitPoint += up * 0.01f * _movementSpeed * (float)dt;

        _orbitTarget = _orbitPoint;
    }
}

void ViewportCamera::Sink(double dt, int value)
{
    if (_possessionMode == eMove)
    {
        glm::vec3 constexpr down = glm::vec3{0.f, -1.f, 0.f};

        _orbitPoint += down * 0.01f * _movementSpeed * (float)dt;

        _orbitTarget = _orbitPoint;
    }
}
