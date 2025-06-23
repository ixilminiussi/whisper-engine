#include "wsp_window.h"
#include "wsp_devkit.h"
#include "wsp_swapchain.h"

// std
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vulkan/vulkan_handles.hpp>

namespace wsp
{

Window::Window(vk::Instance instance, size_t width, size_t height, std::string name)
    : _width{width}, _height{height}, _name{name}, _freed{false}, _glfwWindow{}, _swapchain{}, _device{}
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _glfwWindow = glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_glfwWindow, FramebufferResizeCallback);

    CreateSurface(instance);
}

Window::~Window()
{
    if (!_freed)
    {
        spdlog::critical("Window: forgot to Free before deletion");
    }
}

void Window::Free(const vk::Instance &instance)
{
    if (_freed)
    {
        spdlog::error("Window: already freed window");
        return;
    }

    if (_swapchain)
    {
        check(_device);
        _swapchain->Free(_device);
    }

    instance.destroySurfaceKHR(_surface);
    glfwDestroyWindow(_glfwWindow);

    _freed = true;
    spdlog::info("Window: succesfully freed window \"{0}\"", _name.c_str());
}

void Window::FramebufferResizeCallback(GLFWwindow *glfwWindow, int width, int height)
{
    Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));
    window->_width = width;
    window->_height = height;
}

void Window::CreateSurface(vk::Instance instance)
{
    const VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(instance), _glfwWindow, nullptr,
                                                    reinterpret_cast<VkSurfaceKHR *>(&_surface));

    if (result != VK_SUCCESS)
    {
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Window: failed to create window surface!");
    }
}

void Window::SetDevice(const Device *device)
{
    _device = device;
}

void Window::BuildSwapchain()
{
    if (!_device)
    {
        throw std::runtime_error("Window: forgot to set window's device before building swapchain");
    }

    if (_swapchain != nullptr)
    {
        Swapchain *oldSwapchain = _swapchain;
        _swapchain = new Swapchain(this, _device, {(uint32_t)_width, (uint32_t)_height}, oldSwapchain->GetHandle());
        oldSwapchain->Free(_device);
    }
    _swapchain = new Swapchain(this, _device, {(uint32_t)_width, (uint32_t)_height});
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_glfwWindow);
}

vk::Extent2D Window::GetExtent() const
{
    return {static_cast<uint32_t>(_width), static_cast<uint32_t>(_height)};
}

GLFWwindow *Window::GetGLFWHandle() const
{
    return _glfwWindow;
}

Swapchain *Window::GetSwapchain() const
{
    return _swapchain;
}

const vk::SurfaceKHR *Window::GetSurface() const
{
    return &_surface;
}

} // namespace wsp
