#ifndef NDEBUG
#ifndef WSP_EDITOR
#define WSP_EDITOR

#include <vulkan/vulkan.hpp>

namespace wsp
{

class Editor
{
  public:
    static bool IsActive();

  private:
    static bool _active;

  public:
    Editor(const class Window *, const class Device *, vk::Instance);
    ~Editor();

    void Free(const class Device *);
    static void Render(vk::CommandBuffer commandBuffer);

  protected:
    void InitImGui(const class Window *, const class Device *, vk::Instance);
    vk::DescriptorPool _imguiDescriptorPool;

    static void ApplyImGuiTheme();

    bool _freed;
};

} // namespace wsp

#endif
#endif
