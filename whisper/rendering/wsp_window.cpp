#include "wsp_window.h"
#include "wsp_device.h"
#include "wsp_devkit.h"
#include "wsp_engine.h"
#include "wsp_swapchain.h"

// std
#include <bits/types/wint_t.h>
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
    window->Resize(width, height);
}

void Window::Resize(size_t width, size_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    _width = width;
    _height = height;

    check(_device);
    check(_swapchain);

    BuildSwapchain();

    for (auto [pointer, function] : _resizeCallbacks)
    {
        function(pointer, _device, width, height);
    }
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

void Window::Initialize(const Device *device)
{
    check(device);

    _device = device;
    BuildSwapchain();
}

void Window::BuildSwapchain()
{
    if (!_device)
    {
        throw std::runtime_error("Window: forgot to set window's device before building swapchain");
    }

    _device->WaitIdle();

    if (_swapchain != nullptr)
    {
        Swapchain *oldSwapchain = _swapchain;
        _swapchain = new Swapchain(this, _device, {(uint32_t)_width, (uint32_t)_height}, oldSwapchain->GetHandle());
        oldSwapchain->Free(_device, true);
        delete oldSwapchain;
    }
    else
    {
        _swapchain = new Swapchain(this, _device, {(uint32_t)_width, (uint32_t)_height});
        glfwSetWindowUserPointer(_glfwWindow, this);
        glfwSetFramebufferSizeCallback(_glfwWindow, FramebufferResizeCallback);
    }
}

vk::CommandBuffer Window::NextCommandBuffer(size_t *frameIndex)
{
    check(_device);
    check(frameIndex);

    *frameIndex = _swapchain->GetCurrentFrameIndeex();
    return _swapchain->NextCommandBuffer(_device);
}

void Window::SwapchainOpen(vk::CommandBuffer commandBuffer, vk::Image blittedImage) const
{
    TracyVkZone(engine::TracyCtx(), commandBuffer, "swapchain");

    if (blittedImage != VK_NULL_HANDLE)
    {
        _swapchain->BlitImage(commandBuffer, blittedImage, GetExtent());
        _swapchain->BeginRenderPass(commandBuffer, false);
    }
    else
    {
        _swapchain->SkipBlit(commandBuffer);
        _swapchain->BeginRenderPass(commandBuffer, true);
    }
}

void Window::SwapchainFlush(vk::CommandBuffer commandBuffer)
{
    check(_device);

    ZoneScopedN("submit and wait idle");
    commandBuffer.endRenderPass();
    commandBuffer.end();

    _swapchain->SubmitCommandBuffer(_device, commandBuffer);

    _device->WaitIdle();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_glfwWindow);
}

void Window::BindResizeCallback(void *pointer, void (*function)(void *, const wsp::Device *, size_t, size_t))
{
    _resizeCallbacks[pointer] = function;
    function(pointer, _device, _width, _height);
}

void Window::UnbindResizeCallback(void *pointer)
{
    _resizeCallbacks.erase(pointer);
}

vk::Extent2D Window::GetExtent() const
{
    return {static_cast<uint32_t>(_width), static_cast<uint32_t>(_height)};
}

GLFWwindow *Window::GetGLFWHandle() const
{
    check(_glfwWindow);
    return _glfwWindow;
}

Swapchain *Window::GetSwapchain() const
{
    check(_swapchain);
    return _swapchain;
}

vk::SurfaceKHR Window::GetSurface() const
{
    return _surface;
}

} // namespace wsp
