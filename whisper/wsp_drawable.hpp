#ifndef WSP_DRAWABLE
#define WSP_DRAWABLE

#include <wsp_transform.hpp>

#include <vulkan/vulkan.hpp>

namespace wsp
{

class Drawable
{
  public:
    Drawable() = default;
    ~Drawable() = default;

    virtual void Draw(vk::CommandBuffer, vk::PipelineLayout, class Transform const &) const = 0;
    virtual void Bind(vk::CommandBuffer) const = 0;

    void BindAndDraw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout,
                     Transform const &transform = Transform{}) const
    {
        Bind(commandBuffer);
        Draw(commandBuffer, pipelineLayout, transform);
    }
};

} // namespace wsp

#endif
