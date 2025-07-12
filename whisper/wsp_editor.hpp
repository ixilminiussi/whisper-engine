#ifndef WSP_EDITOR
#define WSP_EDITOR

// lib
#include <vulkan/vulkan.hpp>

// std
#include <functional>

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
    void Render(vk::CommandBuffer, class Camera *, class Graph *, class Device const *);
    void Update(float dt);

    bool IsActive() const;

  protected:
    void InitImGui(class Window const *, class Device const *, vk::Instance);
    vk::DescriptorPool _imguiDescriptorPool;

    void InitDockspace(unsigned int dockspaceID);

    void RenderDockspace();
    void RenderViewport(class Device const *, class Camera *, class Graph *);

    static void ApplyImGuiTheme();

    bool _active;
    bool _freed;

    std::vector<std::function<void()>> _deferredQueue;
};

} // namespace wsp

#endif
