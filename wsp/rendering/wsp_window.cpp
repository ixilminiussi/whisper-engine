#include "wsp_window.h"
#include "wsp_devkit.h"

// std
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace wsp
{

Window::Window(int width, int height, std::string name)
    : _width{width}, _height{height}, _name{name}, _resized{false}, _freed{false}
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _glfwWindow = glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_glfwWindow, FramebufferResizeCallback);
    glfwShowWindow(_glfwWindow);
}

Window::~Window()
{
    if (!_freed)
    {
        spdlog::critical("Window: forgot to Free before deletion");
    }
}

Window *Window::OpenWindow(int width, int height, std::string name)
{
    return new Window(width, height, name);
}

void Window::Free()
{
    if (_freed)
    {
        spdlog::error("Window: already freed window");
        return;
    }

    glfwDestroyWindow(_glfwWindow);

    _freed = true;
}

void Window::FramebufferResizeCallback(GLFWwindow *glfwWindow, int width, int height)
{
    Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));
    window->_resized = true;
    window->_width = width;
    window->_height = height;
}

void Window::CreateWindowSurface(vk::Instance instance, vk::SurfaceKHR *surface)
{
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(instance), _glfwWindow, nullptr,
                                              reinterpret_cast<VkSurfaceKHR *>(surface));

    if (result != VK_SUCCESS)
    {
        spdlog::critical("Window: failed to create window surface! Error: {}",
                         vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Window: failed to create window surface!");
    }
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_glfwWindow);
}

vk::Extent2D Window::GetExtent() const
{
    return {static_cast<uint32_t>(_width), static_cast<uint32_t>(_height)};
}

bool Window::WasResized() const
{
    return _resized;
}

void Window::ResetResizedFlag()
{
    _resized = false;
}

GLFWwindow *Window::GetGLFWHandle() const
{
    return _glfwWindow;
}

} // namespace wsp
