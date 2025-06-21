#ifndef WSP_WINDOW
#define WSP_WINDOW

// std
#include "wsp_swapchain.h"
#include <string>

// lib
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace wsp
{

class Window
{
  public:
    Window(vk::Instance, size_t width, size_t height, std::string name);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    void Free(const vk::Instance &);

    bool ShouldClose() const;

    vk::Extent2D GetExtent() const;

    GLFWwindow *GetGLFWHandle() const;

    Swapchain *GetSwapchain() const;

    const vk::SurfaceKHR *GetSurface() const;

    void SetDevice(const class Device *);
    void BuildSwapchain();

  private:
    void CreateSurface(vk::Instance);

    static void FramebufferResizeCallback(GLFWwindow *glfwWindow, int width, int height);

    size_t _width;
    size_t _height;
    std::string _name;
    GLFWwindow *_glfwWindow;

    class Swapchain *_swapchain;
    const class Device *_device;

    vk::SurfaceKHR _surface;

    bool _freed;
};

} // namespace wsp

#endif
