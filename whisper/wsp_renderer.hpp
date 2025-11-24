#ifndef WSP_RENDERER
#define WSP_RENDERER

#include <wsp_handles.hpp>

#include <vulkan/vulkan.hpp>

#include <tracy/TracyVulkan.hpp>

namespace wsp
{

class Renderer
{
  public:
    Renderer();
    ~Renderer();

    Renderer(Renderer const &) = delete;
    Renderer &operator=(Renderer const &) = delete;

    void Initialize(size_t width, size_t height);
    void Render(vk::CommandBuffer, size_t frameIndex) const;

    class Graph *GetGraph() const;

  private:
    std::unique_ptr<class Graph> _graph;
};

} // namespace wsp

#endif
