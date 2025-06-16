#ifndef WSP_WINDOW_H
#define WSP_WINDOW_H

// std
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
    static Window *OpenWindow(int width, int height, std::string name);
    ~Window();

    void CreateWindowSurface(vk::Instance instance, vk::SurfaceKHR *surface);
    void Free();

    bool ShouldClose() const;

    vk::Extent2D GetExtent() const;

    bool WasResized() const;

    void ResetResizedFlag();

    GLFWwindow *GetGLFWHandle() const;

  private:
    Window(int width, int height, std::string name);

    static void FramebufferResizeCallback(GLFWwindow *glfwWindow, int width, int height);

    int _width;
    int _height;
    bool _resized;
    std::string _name;
    GLFWwindow *_glfwWindow;

    bool _freed;
};

} // namespace wsp

#endif
