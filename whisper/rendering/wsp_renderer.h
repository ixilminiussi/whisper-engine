#ifndef WSP_RENDERER
#define WSP_RENDERER

// wps
#include "wsp_device.h"

// std
#include <vector>

// lib
#include <vulkan/vulkan.hpp>
//
#include <tracy/TracyVulkan.hpp>

namespace fl
{
class Graph;
}

namespace wsp
{

class Renderer
{
  public:
    Renderer(const class Device *, class Window *);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void Free(const class Device *);

    [[nodiscard]] vk::CommandBuffer RenderGraph(const class Device *);
    void SwapchainOpen(const class Device *, vk::CommandBuffer);
    void SwapchainFlush(const class Device *, vk::CommandBuffer);

  private:
    void CreateCommandBuffers(const class Device *);
    std::vector<vk::CommandBuffer> _commandBuffers;

    const class Window *_window;
    class fl::Graph *_graph;

    uint32_t _currentImageIndex;
    uint32_t _currentFrameIndex;

    bool _freed;
};

} // namespace wsp

#endif
