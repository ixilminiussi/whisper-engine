#ifndef WSP_WINDOW
#define WSP_WINDOW

// std
#include <map>
#include <memory>
#include <string>

// lib
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>

namespace wsp
{

class Window
{
  public:
    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;
    Window(vk::Instance, uint32_t width, uint32_t height, std::string name);
    ~Window();

    Window(Window const &) = delete;
    Window &operator=(Window const &) = delete;

    void Initialize();
    void Free(vk::Instance const &);

    bool ShouldClose() const;

    vk::Extent2D GetExtent() const;

    GLFWwindow *GetGLFWHandle() const;

    class Swapchain *GetSwapchain() const;

    vk::SurfaceKHR GetSurface() const;

    vk::CommandBuffer NextCommandBuffer(uint32_t *frameIndex);
    void SwapchainOpen(vk::CommandBuffer, vk::Image blittedImage = VK_NULL_HANDLE) const;
    void SwapchainFlush(vk::CommandBuffer);

    void BindResizeCallback(void *, void (*)(void *, uint32_t, uint32_t));
    void UnbindResizeCallback(void *);

  private:
    void BuildSwapchain();
    void CreateSurface(vk::Instance);

    static void FramebufferResizeCallback(GLFWwindow *, int width, int height);
    void Resize(uint32_t width, uint32_t height);

    uint32_t _width;
    uint32_t _height;
    std::string _name;
    GLFWwindow *_glfwWindow;

    class Swapchain *_swapchain;

    vk::SurfaceKHR _surface;

    std::map<void *, void (*)(void *, uint32_t, uint32_t)> _resizeCallbacks;

    bool _freed;
};

} // namespace wsp

#endif
