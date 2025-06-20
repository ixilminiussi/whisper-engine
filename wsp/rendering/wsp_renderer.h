#ifndef WSP_RENDERER
#define WSP_RENDERER

// std
#include <vector>

// lib
#include <vulkan/vulkan.hpp>

namespace wsp
{

class Renderer
{
  public:
    Renderer(const class Device *, const class Window *);
    ~Renderer();

    void Free(const class Device *);

    [[nodiscard]] vk::CommandBuffer BeginRender(const class Device *);
    void EndRender(const class Device *);

  private:
    void CreateCommandBuffers(const class Device *);
    std::vector<vk::CommandBuffer> _commandBuffers;

    const class Window *_window;

    uint32_t _currentImageIndex;
    uint32_t _currentFrameIndex;

    bool _freed;
};

} // namespace wsp

#endif
