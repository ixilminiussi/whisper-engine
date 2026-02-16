#ifndef WSP_RENDER_MANAGER
#define WSP_RENDER_MANAGER

#include <wsp_typedefs.hpp>

#include <map>
#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

struct GLFWwindow;

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

    bool Validate(WindowID) const;

    void Free();

    void BindResizeCallback(WindowID, void *pointer, void (*function)(void *, uint32_t, uint32_t));
    class Graph *GetGraph(WindowID) const;
    GLFWwindow *GetGLFWHandle(WindowID) const;

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
