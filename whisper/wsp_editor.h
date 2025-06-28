#ifndef NDEBUG
#ifndef WSP_EDITOR
#define WSP_EDITOR

#include <vulkan/vulkan.hpp>

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
    void Render(vk::CommandBuffer, const class Renderer *);
    void Update(float dt);

    void BindToggle(void *who, void (*)(void *, bool));

  protected:
    void InitImGui(const class Window *, const class Device *, vk::Instance);
    vk::DescriptorPool _imguiDescriptorPool;

    static void ApplyImGuiTheme();

    std::vector<std::pair<void *, void (*)(void *, bool)>> _toggleDispatchers;

    bool _active;
    bool _freed;
};

} // namespace wsp

#endif
#endif
