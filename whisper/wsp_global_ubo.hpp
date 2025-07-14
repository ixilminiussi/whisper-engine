#ifndef WSP_GLOBAL_UBO
#define WSP_GLOBAL_UBO

#include <glm/matrix.hpp>

namespace wsp
{

struct CameraInfo
{
    glm::mat4 viewProjection;
};

struct GlobalUbo
{
    CameraInfo camera;
};

} // namespace wsp

#endif
