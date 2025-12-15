#ifndef WSP_DRAWABLE
#define WSP_DRAWABLE

#include <vulkan/vulkan.hpp>

#include <wsp_transform.hpp>

namespace wsp
{

class Drawable
{
  public:
    Drawable() = default;
    ~Drawable() = default;

    virtual void Draw(vk::CommandBuffer, vk::PipelineLayout) const = 0;
    virtual void Bind(vk::CommandBuffer) const = 0;

    void BindAndDraw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) const
    {
        Bind(commandBuffer);
        Draw(commandBuffer, pipelineLayout);
    }

    virtual float GetRadius() const = 0;

  protected:
    Transform _transform;
};

} // namespace wsp

#endif
