#ifndef WSP_EDITOR
#define WSP_EDITOR

#include <vulkan/vulkan.hpp>

#include <functional>
#include <map>
#include <memory>

#define REGULAR_FONT 0u
#define THUMBNAILS_FONT 1u

struct ImFont;

namespace wsp
{

class Editor
{
  public:
    Editor(class Window const *, class Device const *, vk::Instance);
    ~Editor();

    Editor(Editor const &) = delete;
    Editor &operator=(Editor const &) = delete;

    void Free(class Device const *);
    void Render(vk::CommandBuffer, class Graph *, class Window *, class Device const *);
    void Update(double dt);

    bool IsActive() const;

    void PopulateUbo(class GlobalUbo *);

  protected:
    void InitImGui(class Window const *, class Device const *, vk::Instance);
    vk::DescriptorPool _imguiDescriptorPool;

    static void InitDockspace(unsigned int dockspaceID);

    static void RenderDockspace();
    void RenderViewport(class Device const *, class Graph *);

    static void ApplyImGuiTheme();

    bool _active;
    bool _freed;

    std::unique_ptr<class ViewportCamera> _viewportCamera;
    std::unique_ptr<class AssetsManager> _assetsManager;

    std::vector<std::function<void()>> _deferredQueue;
};

} // namespace wsp

#endif
