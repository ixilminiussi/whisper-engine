#include <wsp_window.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_engine.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_renderer.hpp>
#include <wsp_swapchain.hpp>

#include <vulkan/vulkan_handles.hpp>

#include <spdlog/spdlog.h>

#include <stdexcept>

using namespace wsp;

Window::Window(vk::Instance instance, uint32_t width, uint32_t height, std::string name)
    : _width{width}, _height{height}, _name{name}, _freed{false}, _glfwWindow{}, _swapchain{}
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

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

void Window::Initialize()
{
    BuildSwapchain();
}

void Window::Free(vk::Instance const &instance)
{
    if (_freed)
    {
        spdlog::error("Window: already freed window");
        return;
    }

    delete _swapchain;

    instance.destroySurfaceKHR(_surface);
    glfwDestroyWindow(_glfwWindow);

    _freed = true;
    spdlog::info("Window: freed window \"{}\"", _name.c_str());
}

void Window::FramebufferResizeCallback(GLFWwindow *glfwWindow, int width, int height)
{
    Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));
    window->Resize(width, height);
}

void Window::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    _width = width;
    _height = height;

    check(_swapchain);

    BuildSwapchain();

    for (auto [pointer, function] : _resizeCallbacks)
    {
        function(pointer, width, height);
    }
}

void Window::CreateSurface(vk::Instance instance)
{
    VkResult const result = glfwCreateWindowSurface(static_cast<VkInstance>(instance), _glfwWindow, nullptr,
                                                    reinterpret_cast<VkSurfaceKHR *>(&_surface));

    if (result != VK_SUCCESS)
    {
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Window: failed to create window surface!");
    }
}

void Window::BuildSwapchain()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->WaitIdle();

    if (_swapchain)
    {
        Swapchain const *oldSwapchain = _swapchain;
        _swapchain = new Swapchain(this, vk::Extent2D{(uint32_t)_width, (uint32_t)_height}, oldSwapchain->GetHandle());

        delete oldSwapchain;
    }
    else
    {
        _swapchain = new Swapchain(this, vk::Extent2D{(uint32_t)_width, (uint32_t)_height});
        glfwSetWindowUserPointer(_glfwWindow, this);
        glfwSetFramebufferSizeCallback(_glfwWindow, FramebufferResizeCallback);
    }
}

vk::CommandBuffer Window::NextCommandBuffer(uint32_t *frameIndex)
{
    check(frameIndex);

    *frameIndex = _swapchain->GetCurrentFrameIndex();
    return _swapchain->NextCommandBuffer();
}

void Window::SwapchainOpen(vk::CommandBuffer commandBuffer, vk::Image blittedImage) const
{
    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "swapchain");

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
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ZoneScopedN("submit and wait idle");
    commandBuffer.endRenderPass();
    commandBuffer.end();

    _swapchain->SubmitCommandBuffer(commandBuffer);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_glfwWindow);
}

void Window::BindResizeCallback(void *pointer, void (*function)(void *, uint32_t, uint32_t))
{
    _resizeCallbacks[pointer] = function;
    function(pointer, _width, _height);
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
    return _swapchain;
}

vk::SurfaceKHR Window::GetSurface() const
{
    return _surface;
}
