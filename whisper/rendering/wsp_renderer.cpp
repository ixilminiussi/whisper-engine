#include "wsp_renderer.h"

#include "fl_graph.h"
#include "fl_handles.h"
#include "wsp_device.h"
#include "wsp_window.h"
#include <spdlog/spdlog.h>

namespace wsp
{

Renderer::Renderer(const Device *device, const Window *window)
    : _freed{false}, _currentImageIndex{0}, _currentFrameIndex{0}
{
    _window = window;

    _graph = new fl::Graph(device);

    fl::ResourceCreateInfo colorResourceInfo{};
    colorResourceInfo.role = fl::ResourceRole::eColor;
    colorResourceInfo.format = vk::Format::eR8G8B8A8Unorm;
    colorResourceInfo.extent = vk::Extent2D{1024, 1024};
    colorResourceInfo.isTarget = true;

    fl::ResourceCreateInfo depthResourceInfo{};
    depthResourceInfo.role = fl::ResourceRole::eDepth;
    depthResourceInfo.format = vk::Format::eR32Sfloat;
    depthResourceInfo.extent = vk::Extent2D{1024, 1024};
    depthResourceInfo.isTarget = true;

    const fl::Resource color = _graph->NewResource(colorResourceInfo);
    const fl::Resource depth = _graph->NewResource(depthResourceInfo);

    fl::PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {color, depth};
    passCreateInfo.reads = {};

    _graph->NewPass(passCreateInfo);

    _graph->Compile(device);

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
    device->FreeCommandBuffers(&_commandBuffers);
    _commandBuffers.clear();

    _freed = true;
    spdlog::info("Renderer: succesfully freed");
}

vk::CommandBuffer Renderer::BeginRender(const Device *device)
{
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

    swapchain->BeginRenderPass(commandBuffer, _currentImageIndex);

    return commandBuffer;
}

void Renderer::Render(vk::CommandBuffer commandBuffer)
{
    _graph->Run(commandBuffer);
}

void Renderer::EndRender(const Device *device)
{
    Swapchain *swapchain = _window->GetSwapchain();
    auto commandBuffer = _commandBuffers[_currentFrameIndex];

    commandBuffer.endRenderPass();
    commandBuffer.end();

    swapchain->SubmitCommandBuffer(device, commandBuffer, _currentImageIndex, _currentFrameIndex);

    _currentFrameIndex = (_currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;

    device->WaitIdle();
}

void Renderer::CreateCommandBuffers(const Device *device)
{
    _commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    device->AllocateCommandBuffers(&_commandBuffers);
}

} // namespace wsp
