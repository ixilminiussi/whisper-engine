#ifndef WSP_RENDERER
#define WSP_RENDERER

#include "wsp_handles.hpp"
#include <vulkan/vulkan.hpp>
//
#include <tracy/TracyVulkan.hpp>

// std
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

    void Initialize();
    void Free();
    void Render() const;
    void Update(double dt);

    static TracyVkCtx GetTracyCtx();

    bool ShouldClose();

  private:
    void Free(class Device const *);

    void CreateInstance();
    std::vector<char const *> GetRequiredExtensions();
    void ExtensionsCompatibilityTest();

#ifndef NDEBUG
    bool CheckValidationLayerSupport();
#endif

    std::vector<char const *> const _validationLayers;
    std::vector<char const *> const _deviceExtensions;

    class Graph *_graph;
    class Window *_window;
    class Editor *_editor;
    std::vector<Resource> _resources;

    static class Device *_device;
    static vk::Instance _vkInstance;

    static TracyVkCtx tracyCtx;

    bool _freed;
};

} // namespace wsp

#endif
