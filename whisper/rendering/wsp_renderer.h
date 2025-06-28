#ifndef WSP_RENDERER
#define WSP_RENDERER

// wps
#include "wsp_device.h"
#include "wsp_handles.h"

// std
#include <vector>

// lib
#include <vulkan/vulkan.hpp>
//
#include <tracy/TracyVulkan.hpp>

namespace wsp
{

class Renderer
{
  public:
    enum RendererGoal
    {
        eToTransfer = 0,
        eToDescriptorSet = 1
    };

    Renderer(const class Device *, class Window *, RendererGoal goal = eToDescriptorSet);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void Free(const class Device *);

    [[nodiscard]] vk::CommandBuffer RenderGraph(const class Device *);
    void SwapchainOpen(const class Device *, vk::CommandBuffer);
    void SwapchainFlush(const class Device *, vk::CommandBuffer);

    vk::DescriptorSet GetRenderedDescriptorSet() const;

    void ChangeGoal(const class Device *, RendererGoal);

  private:
    void CreateCommandBuffers(const class Device *);
    std::vector<vk::CommandBuffer> _commandBuffers;

    const class Window *_window;
    class Graph *_graph;
    RendererGoal _goal;

    uint32_t _currentImageIndex;
    uint32_t _currentFrameIndex;

    bool _freed;
};

} // namespace wsp

#endif
