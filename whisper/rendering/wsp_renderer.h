#ifndef WSP_RENDERER
#define WSP_RENDERER

#include "wsp_handles.h"
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

    void NewWindow();
    void Free();
    void Run();

    static TracyVkCtx GetTracyCtx();

  private:
    void FreeWindow(const class Device *, size_t ID);

    void CreateInstance();
    std::vector<char const *> GetRequiredExtensions();
    void ExtensionsCompatibilityTest();

#ifndef NDEBUG
    bool CheckValidationLayerSupport();
#endif

    std::vector<char const *> const _validationLayers;
    std::vector<char const *> const _deviceExtensions;

    std::vector<class Graph *> _graphs;
    std::vector<class Window *> _windows;
    std::vector<class Editor *> _editors;
    std::vector<std::vector<Resource>> _resources;

    static class Device *_device;
    static vk::Instance _vkInstance;

    static TracyVkCtx tracyCtx;

    bool _freed;
};

} // namespace wsp

#endif
