#include "wsp_renderer.h"

#include "fl_graph.h"
#include "fl_handles.h"
#include "wsp_device.h"
#include "wsp_devkit.h"
#include "wsp_window.h"
#include <spdlog/spdlog.h>

namespace wsp
{

Renderer::Renderer(const Device *device, Window *window) : _freed{false}, _currentImageIndex{0}, _currentFrameIndex{0}
{
    check(device);
    check(window);

    _window = window;

    _graph = new fl::Graph(device, _window->GetExtent().width, _window->GetExtent().height);
    window->BindResizeCallback(_graph, fl::Graph::WindowResizeCallback);

    fl::ResourceCreateInfo colorResourceInfo{};
    colorResourceInfo.role = fl::ResourceRole::eColor;
    colorResourceInfo.format = vk::Format::eR8G8B8A8Unorm;

    fl::ResourceCreateInfo depthResourceInfo{};
    depthResourceInfo.role = fl::ResourceRole::eDepth;
    depthResourceInfo.format = vk::Format::eD32Sfloat;

    const fl::Resource color = _graph->NewResource(colorResourceInfo);
    const fl::Resource depth = _graph->NewResource(depthResourceInfo);

    fl::PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {color, depth};
    passCreateInfo.reads = {};
    passCreateInfo.vertFile = "triangle.vert.spv";
    passCreateInfo.fragFile = "triangle.frag.spv";
    passCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(3, 1, 0, 0); };

    _graph->NewPass(passCreateInfo);

    _graph->Compile(device, color);

    CreateCommandBuffers(device);
}

Renderer::~Renderer()
{
    if (!_freed)
    {
        spdlog::error("Renderer: forgot to Free before deletion");
    }
}

void Renderer::Free(const Device *device)
{
    check(device);

    device->FreeCommandBuffers(&_commandBuffers);
    _commandBuffers.clear();

    _graph->Free(device);
    delete _graph;

    _freed = true;
    spdlog::info("Renderer: succesfully freed");
}

vk::CommandBuffer Renderer::RenderGraph(const Device *device)
{
    check(device);

    const Swapchain *swapchain = _window->GetSwapchain();
    swapchain->AcquireNextImage(device, _currentFrameIndex, &_currentImageIndex);

    const vk::CommandBuffer commandBuffer = _commandBuffers[_currentFrameIndex];

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

    if (const vk::Result result = commandBuffer.begin(&beginInfo); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Renderer: failed to begin recording command buffer!");
    }

    _graph->Run(commandBuffer);

    return commandBuffer;
}

void Renderer::SwapchainOpen(const Device *device, vk::CommandBuffer commandBuffer)
{
    check(device);

    Swapchain *swapchain = _window->GetSwapchain();
    swapchain->BlitImage(commandBuffer, _graph->GetTargetImage(), _window->GetExtent(), _currentImageIndex);
    swapchain->BeginRenderPass(commandBuffer, _currentImageIndex);
}

void Renderer::SwapchainFlush(const Device *device, vk::CommandBuffer commandBuffer)
{
    commandBuffer.endRenderPass();
    commandBuffer.end();

    Swapchain *swapchain = _window->GetSwapchain();
    swapchain->SubmitCommandBuffer(device, commandBuffer, _currentImageIndex, _currentFrameIndex);

    _currentFrameIndex = (_currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;

    device->WaitIdle();
}

void Renderer::CreateCommandBuffers(const Device *device)
{
    check(device);

    _commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    device->AllocateCommandBuffers(&_commandBuffers);
}

} // namespace wsp
