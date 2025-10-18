#ifndef WSP_RENDER_MANAGER
#define WSP_RENDER_MANAGER

#include <cstddef>
#include <map>
#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

using WindowID = size_t;

namespace wsp
{

class RenderManager
{
  public:
    static RenderManager *Get();

  protected:
    static RenderManager *_instance;

  public:
    ~RenderManager();

    WindowID NewWindowRenderer();
    bool ShouldClose(WindowID);
    void CloseWindow(WindowID);

    void Free();

    void BindResizeCallback(WindowID, void *pointer, void (*function)(void *, size_t, size_t));
    class Graph *GetGraph(WindowID) const;

    vk::CommandBuffer BeginRender(WindowID, bool blit = false);
    void EndRender(vk::CommandBuffer, WindowID);

    void InitImGui(WindowID);

  protected:
    RenderManager();

    void CreateInstance();

    static std::vector<char const *> GetRequiredExtensions();
    static void ExtensionsCompatibilityTest();

    bool CheckValidationLayerSupport();

    std::vector<char const *> const _validationLayers;
    std::vector<char const *> const _deviceExtensions;

    using WindowRenderer = struct
    {
        std::unique_ptr<class Window> window;
        std::unique_ptr<class Renderer> renderer;
    };

    std::map<WindowID, WindowRenderer> _windowRenderers;
    std::map<WindowID, vk::DescriptorPool> _imguiDescriptorPools;

    static vk::Instance _vkInstance;

    bool _freed;
};

} // namespace wsp

#endif
