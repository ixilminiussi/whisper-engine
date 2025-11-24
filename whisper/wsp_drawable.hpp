#ifndef WSP_DRAWABLE
#define WSP_DRAWABLE

#include <vulkan/vulkan.hpp>

namespace wsp
{

class Drawable
{
  public:
    Drawable() = default;
    ~Drawable() = default;

    virtual void Draw(vk::CommandBuffer) const = 0;
    virtual void Bind(vk::CommandBuffer) const = 0;

    void BindAndDraw(vk::CommandBuffer commandBuffer) const
    {
        Bind(commandBuffer);
        Draw(commandBuffer);
    }

    virtual float GetRadius() const = 0;
};

} // namespace wsp

#endif
