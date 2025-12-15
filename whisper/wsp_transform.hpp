#ifndef WSP_TRANSFORM
#define WSP_TRANSFORM

#include <.generated/wsp_transform.generated.hpp>

#include <wsp_devkit.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace wsp
{

WCLASS()
class Transform
{
  public:
    Transform();
    Transform(glm::mat4 const &);
    Transform(glm::vec3 const &position, glm::vec3 const &scale, glm::quat const &rotation);
    Transform(glm::vec3 const &position, glm::vec3 const &scale, glm::vec3 const &euler);
    ~Transform() = default;

    glm::mat4 GetMatrix() const;
    glm::mat3 GetNormalMatrix() const;
    void FromMatrix(glm::mat4 const &);
    glm::vec3 Forward() const;
    glm::vec3 Right() const;
    glm::vec3 Up() const;

    void SetPosition(glm::vec3 const &);
    void SetScale(glm::vec3 const &);
    void SetRotation(glm::quat const &quaternion);
    void SetRotation(glm::vec3 const &euler);

    glm::vec3 const &GetPosition() const;
    glm::vec3 const &GetScale() const;
    glm::quat const &GetRotation() const;

    REFRESH()
    void Refresh();

    WCLASS_BODY$Transform();

  protected:
    WPROPERTY(eDrag);
    glm::vec3 _position;
    WPROPERTY(eDrag);
    glm::vec3 _scale;
    glm::quat _rotation;

    // get updated on changes
    glm::mat4 _matrix;
    glm::mat3 _normalMatrix;
};

static Transform operator+(Transform const &, Transform const &);

} // namespace wsp

#endif
