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

    Editor(Editor const &) = delete;
    Editor &operator=(Editor const &) = delete;

    void Free(const class Device *);
    void Render(vk::CommandBuffer, class Camera *, class Graph *, const class Device *);
    void Update(float dt);

    bool IsActive() const;

  protected:
    void InitImGui(const class Window *, const class Device *, vk::Instance);
    vk::DescriptorPool _imguiDescriptorPool;

    static void ApplyImGuiTheme();

    bool _active;
    bool _freed;

    std::vector<std::function<void()>> _deferredQueue;
};

} // namespace wsp

#endif
