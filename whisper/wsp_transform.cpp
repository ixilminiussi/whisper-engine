#include <wsp_transform.hpp>

#include <wsp_devkit.hpp>

#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

using namespace wsp;

Transform::Transform() : _position{0.f}, _scale{1.f}, _rotation{}
{
    Refresh();
}

Transform::Transform(glm::mat4 const &mat4)
{
    FromMatrix(mat4);
}

Transform::Transform(glm::vec3 const &position, glm::vec3 const &scale, glm::quat const &rotation)
    : _position{position}, _scale{scale}, _rotation{rotation}
{
    Refresh();
}

Transform::Transform(glm::vec3 const &position, glm::vec3 const &scale, glm::vec3 const &euler)
    : _position{position}, _scale{scale}
{
    SetRotation(euler);
}

glm::mat4 Transform::GetMatrix() const
{
    return _matrix;
}

glm::mat3 Transform::GetNormalMatrix() const
{
    return _normalMatrix;
}

void Transform::FromMatrix(glm::mat4 const &mat4)
{
    _position = mat4[3];

    _scale.x = glm::length(glm::vec3(mat4[0]));
    _scale.y = glm::length(glm::vec3(mat4[1]));
    _scale.z = glm::length(glm::vec3(mat4[2]));

    glm::mat3 rotationMatrix = glm::mat3(mat4);
    rotationMatrix[0] /= _scale.x;
    rotationMatrix[1] /= _scale.y;
    rotationMatrix[2] /= _scale.z;

    _rotation = glm::quat_cast(rotationMatrix);
}

glm::vec3 Transform::Forward() const
{
    return glm::normalize(_rotation * glm::vec3(0.0f, 0.0f, -1.0f)); // Default forward in OpenGL is -Z
}

glm::vec3 Transform::Up() const
{
    return glm::normalize(_rotation * glm::vec3(0.0f, -1.0f, 0.0f)); // Default up is +Y
}

glm::vec3 Transform::Right() const
{
    return glm::normalize(_rotation * glm::vec3(1.0f, 0.0f, 0.0f)); // Default right is +X
}

Transform operator+(Transform const &a, Transform const &b)
{
    return Transform{a.GetMatrix() * b.GetMatrix()};
}

void Transform::SetPosition(glm::vec3 const &position)
{
    _position = position;
    Refresh();
}

void Transform::SetScale(glm::vec3 const &scale)
{
    _scale = scale;
    Refresh();
}

void Transform::SetRotation(glm::quat const &rotation)
{
    _rotation = glm::normalize(rotation);
    Refresh();
}

void Transform::SetRotation(glm::vec3 const &euler)
{
    _rotation = glm::quat{euler};
    Refresh();
}

glm::vec3 const &Transform::GetPosition() const
{
    return _position;
}

glm::vec3 const &Transform::GetScale() const
{
    return _scale;
}

glm::quat const &Transform::GetRotation() const
{
    return _rotation;
}

void Transform::Refresh()
{
    {
        glm::mat3 const rotationMatrix = glm::mat3_cast(_rotation);
        glm::mat3 const scaleMatrix = glm::mat3(glm::scale(glm::mat4(1.0f), 1.0f / _scale));
        _normalMatrix = glm::transpose(glm::inverse(rotationMatrix * scaleMatrix));
    }

    {
        glm::mat4 const rotationMatrix = glm::mat4_cast(_rotation);
        glm::mat4 const translationMatrix = glm::translate(glm::mat4(1.0f), _position);
        _matrix = translationMatrix * rotationMatrix;
    }
}
