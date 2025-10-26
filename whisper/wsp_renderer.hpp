#ifndef WSP_RENDERER
#define WSP_RENDERER

#include <wsp_handles.hpp>

#include <vulkan/vulkan.hpp>

#include <tracy/TracyVulkan.hpp>

#include <vector>

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
    void Free();
    void Render(vk::CommandBuffer, size_t frameIndex) const;

    class Graph *GetGraph() const;

  private:
    void Free(class Device const *);

    std::unique_ptr<class Graph> _graph;

    bool _freed;
};

} // namespace wsp

#endif
