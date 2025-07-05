#ifndef WSP_WINDOW
#define WSP_WINDOW

// std
#include <map>
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
    Window(vk::Instance, size_t width, size_t height, std::string name);
    ~Window();

    Window(Window const &) = delete;
    Window &operator=(Window const &) = delete;

    void Free(vk::Instance const &);

    bool ShouldClose() const;

    vk::Extent2D GetExtent() const;

    GLFWwindow *GetGLFWHandle() const;

    class Swapchain *GetSwapchain() const;

    vk::SurfaceKHR GetSurface() const;

    void Initialize(const class Device *);

    vk::CommandBuffer NextCommandBuffer(size_t *frameIndex);
    void SwapchainOpen(vk::CommandBuffer, vk::Image blittedImage = VK_NULL_HANDLE) const;
    void SwapchainFlush(vk::CommandBuffer);

    void BindResizeCallback(void *, void (*)(void *, Device const *, size_t, size_t));
    void UnbindResizeCallback(void *);

  private:
    void BuildSwapchain();
    void CreateSurface(vk::Instance);

    static void FramebufferResizeCallback(GLFWwindow *, int width, int height);
    void Resize(size_t width, size_t height);

    size_t _width;
    size_t _height;
    std::string _name;
    GLFWwindow *_glfwWindow;

    class Swapchain *_swapchain;
    const class Device *_device;

    vk::SurfaceKHR _surface;

    std::map<void *, void (*)(void *, Device const *, size_t, size_t)> _resizeCallbacks;

    bool _freed;
};

} // namespace wsp

#endif
