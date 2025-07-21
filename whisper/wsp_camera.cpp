#include <wsp_camera.hpp>

#include <wsp_devkit.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace wsp;

Camera::Camera() : _mode{ePerspective}, _aspectRatio{1.0}, _fov{80.f}
{
}

void Camera::UpdateProjection()
{
    switch (_mode)
    {
    case eOrthographic:
        _projectionMatrix = glm::ortho(_left, _right, _bottom, _top, _near, _far);
        break;
    case ePerspective:
        check(glm::abs(_aspectRatio - std::numeric_limits<float>::epsilon()) > 0.0f);

        _projectionMatrix = glm::perspective(glm::radians(_fov), _aspectRatio, _nearPlane, _farPlane);
        _projectionMatrix[1][1] *= -1.0f;
        break;
    }
}

glm::mat4 const &Camera::GetProjection() const
{
    return _projectionMatrix;
}

glm::mat4 const &Camera::GetView() const
{
    return _viewMatrix;
}

float Camera::GetFOV() const
{
    return _fov;
}

float Camera::GetNearPlane() const
{
    return _near;
}

float Camera::GetFarPlane() const
{
    return _far;
}

glm::vec3 Camera::GetPosition() const
{
    return glm::vec3(glm::inverse(_viewMatrix)[3]);
}

glm::vec3 Camera::GetUp() const
{
    return glm::vec3(glm::inverse(_viewMatrix)[1]);
}

glm::vec3 Camera::GetForward() const
{
    return glm::vec3(glm::inverse(_viewMatrix)[2]);
}

glm::vec3 Camera::GetRight() const
{
    return glm::vec3(glm::inverse(_viewMatrix)[0]);
}

float Camera::GetPitch() const
{
    return glm::asin(GetForward().y);
}

float Camera::GetYaw() const
{
    glm::vec3 forward = GetForward();
    glm::vec3 forwardXZ = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
    return glm::atan(forwardXZ.x, forwardXZ.z);
}

void Camera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far)
{
    _left = left;
    _right = right;
    _top = top;
    _bottom = bottom;
    _near = near;
    _far = far;
    _mode = eOrthographic;

    UpdateProjection();
}

void Camera::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    _aspectRatio = aspectRatio;
    _nearPlane = nearPlane;
    _farPlane = farPlane;
    _fov = fov;
    _mode = ePerspective;

    UpdateProjection();
}

void Camera::LookTowards(glm::vec3 position, glm::vec3 direction, glm::vec3 up)
{
    LookAt(position, position + direction, up);
}

void Camera::LookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up)
{
    _viewMatrix = glm::lookAt(position, target, up);
}

void Camera::LookXYZ(glm::vec3 position, glm::vec3 rotation)
{
    float const c3 = glm::cos(rotation.z);
    float const s3 = glm::sin(rotation.z);
    float const c2 = glm::cos(rotation.x);
    float const s2 = glm::sin(rotation.x);
    float const c1 = glm::cos(rotation.y);
    float const s1 = glm::sin(rotation.y);
    glm::vec3 const u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    glm::vec3 const v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    glm::vec3 const w{(c2 * s1), (-s2), (c1 * c2)};
    _viewMatrix = glm::mat4{1.f};
    _viewMatrix[0][0] = u.x;
    _viewMatrix[1][0] = u.y;
    _viewMatrix[2][0] = u.z;
    _viewMatrix[0][1] = v.x;
    _viewMatrix[1][1] = v.y;
    _viewMatrix[2][1] = v.z;
    _viewMatrix[0][2] = w.x;
    _viewMatrix[1][2] = w.y;
    _viewMatrix[2][2] = w.z;
    _viewMatrix[3][0] = -glm::dot(u, position);
    _viewMatrix[3][1] = -glm::dot(v, position);
    _viewMatrix[3][2] = -glm::dot(w, position);
}

void Camera::OnResizeCallback(void *camera, class Device const *, size_t width, size_t height)
{
    check(camera);
    reinterpret_cast<Camera *>(camera)->SetAspectRatio((float)width / (float)height);
}

void Camera::SetAspectRatio(float aspectRatio)
{
    if (aspectRatio != _aspectRatio)
    {
        _aspectRatio = aspectRatio;
    }

    UpdateProjection();
}

void Camera::SetNearPlane(float nearPlane)
{
    _nearPlane = nearPlane;

    UpdateProjection();
}

void Camera::SetFarPlane(float farPlane)
{
    _farPlane = farPlane;

    UpdateProjection();
}

void Camera::SetFOV(float FOV)
{
    _fov = FOV;

    UpdateProjection();
}
