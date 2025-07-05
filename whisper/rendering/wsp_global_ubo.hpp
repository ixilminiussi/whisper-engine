#ifndef WSP_GLOBAL_UBO
#define WSP_GLOBAL_UBO

#include <glm/matrix.hpp>

namespace wsp
{

struct CameraInfo
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 position;
};

struct GlobalUbo
{
    CameraInfo camera;
};

} // namespace wsp

#endif
