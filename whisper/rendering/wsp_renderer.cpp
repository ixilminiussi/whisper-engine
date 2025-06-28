#include "wsp_renderer.h"

#include "wsp_device.h"
#include "wsp_devkit.h"
#include "wsp_engine.h"
#include "wsp_graph.h"
#include "wsp_handles.h"
#include "wsp_window.h"

// lib
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

namespace wsp
{

Renderer::Renderer(const Device *device, Window *window, RendererGoal goal)
    : _freed{false}, _currentImageIndex{0}, _currentFrameIndex{0}, _goal{goal}
{
    check(device);
    check(window);

    _window = window;

    _graph = new Graph(device, _window->GetExtent().width, _window->GetExtent().height);
    window->BindResizeCallback((void *)_graph, Graph::WindowResizeCallback);

    ResourceCreateInfo colorResourceInfo{};
    colorResourceInfo.role = ResourceRole::eColor;
    colorResourceInfo.format = vk::Format::eR8G8B8A8Unorm;
    colorResourceInfo.clear.color = vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f};
    colorResourceInfo.debugName = "color";

    const Resource color = _graph->NewResource(colorResourceInfo);

    PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {color};
    passCreateInfo.reads = {};
    passCreateInfo.vertFile = "triangle.vert.spv";
    passCreateInfo.fragFile = "triangle.frag.spv";
    passCreateInfo.debugName = "white triangle";
    passCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(3, 1, 0, 0); };

    _graph->NewPass(passCreateInfo);

    colorResourceInfo.debugName = "reverse color";
    const Resource reverseColor = _graph->NewResource(colorResourceInfo);

    PassCreateInfo postPassCreateInfo{};
    postPassCreateInfo.writes = {reverseColor};
    postPassCreateInfo.reads = {color};
    postPassCreateInfo.vertFile = "postprocess.vert.spv";
    postPassCreateInfo.fragFile = "postprocess.frag.spv";
    postPassCreateInfo.debugName = "post processing";
    postPassCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(6, 1, 0, 0); };

    _graph->NewPass(postPassCreateInfo);

    _graph->Compile(device, reverseColor, (Graph::GraphGoal)goal);

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

    TracyVkCollect(engine::TracyCtx(), commandBuffer);

    _graph->Run(commandBuffer);

    return commandBuffer;
}

void Renderer::SwapchainOpen(const Device *device, vk::CommandBuffer commandBuffer)
{
    TracyVkZone(wsp::engine::TracyCtx(), commandBuffer, "swapchain");

    check(device);

    Swapchain *swapchain = _window->GetSwapchain();

    if (_goal == RendererGoal::eToTransfer)
    {
        swapchain->BlitImage(commandBuffer, _graph->GetTargetImage(), _window->GetExtent(), _currentImageIndex);
    }
    swapchain->BeginRenderPass(commandBuffer, _currentImageIndex);
}

void Renderer::SwapchainFlush(const Device *device, vk::CommandBuffer commandBuffer)
{
    check(device);

    ZoneScopedN("submit and wait idle");
    commandBuffer.endRenderPass();
    commandBuffer.end();

    Swapchain *swapchain = _window->GetSwapchain();
    swapchain->SubmitCommandBuffer(device, commandBuffer, _currentImageIndex, _currentFrameIndex);

    _currentFrameIndex = (_currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;

    device->WaitIdle();
}

vk::DescriptorSet Renderer::GetRenderedDescriptorSet() const
{
    check(_graph);

    return _graph->GetTargetDescriptorSet();
}

void Renderer::ChangeGoal(const Device *device, RendererGoal goal)
{
    check(_graph);

    _goal = goal;
    _graph->ChangeGoal(device, (Graph::GraphGoal)goal);
}

void Renderer::CreateCommandBuffers(const Device *device)
{
    check(device);

    _commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    device->AllocateCommandBuffers(&_commandBuffers);
}

} // namespace wsp
