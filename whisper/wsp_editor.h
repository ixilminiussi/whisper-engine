#ifndef WSP_EDITOR
#define WSP_EDITOR

// lib
#include <vulkan/vulkan.hpp>

// std
#include <functional>
#include <map>

namespace wsp
{

class Editor
{
  public:
    Editor(const class Window *, const class Device *, vk::Instance);
    ~Editor();

    Editor(const Editor &) = delete;
    Editor &operator=(const Editor &) = delete;

    void Free(const class Device *);
    void Render(vk::CommandBuffer, class Graph *, const class Device *);
    void Update(float dt);

    void BindToggle(void *, void (*)(void *, bool));
    void UnbindToggle(void *);
    bool isActive() const;

  protected:
    void InitImGui(const class Window *, const class Device *, vk::Instance);
    vk::DescriptorPool _imguiDescriptorPool;

    static void ApplyImGuiTheme();

    std::map<void *, void (*)(void *, bool)> _toggleDispatchers;

    bool _active;
    bool _freed;

    std::vector<std::function<void()>> _deferredQueue;
};

} // namespace wsp

#endif
