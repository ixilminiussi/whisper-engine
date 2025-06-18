#include "wsp_renderer.h"

#include "wsp_window.h"
#include <spdlog/spdlog.h>

namespace wsp
{

Renderer::Renderer(const Device *device, const Window *window) : _freed{false}
{
    _window = window;
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
    _freed = true;
    spdlog::info("Renderer: succesfully freed");
}

} // namespace wsp

/**
void Renderer::free()
{
    _swapChain->free();
    delete _swapChain.release();

    freeCommandBuffers();

    _freed = true;
}


void Renderer::freeCommandBuffers()
{
    _device.device().freeCommandBuffers(_device.getCommandPool(), static_cast<uint32_t>(_commandBuffers.size()),
                                        _commandBuffers.data());
    _commandBuffers.clear();
}

vk::CommandBuffer Renderer::beginFrame()
{
    assert(!_isFrameStarted && "Can't call beginFrame while already in progress");

    vk::Result result = _swapChain->acquireNextImage(&_currentImageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return nullptr;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    _isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

    if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    return commandBuffer;
}

void Renderer::endFrame()
{
    assert(_isFrameStarted && "Can't call endFrame while frame is not in progress");

    auto commandBuffer = getCurrentCommandBuffer();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    auto result = _swapChain->submitCommandBuffers(&commandBuffer, &_currentImageIndex);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _window.wasWindowResized())
    {
        _window.resetWindowResizedFlag();
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _isFrameStarted = false;
    _currentFrameIndex = (_currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginSwapChainRenderPass(vk::CommandBuffer commandBuffer)
{
    assert(_isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't begin render pass on command buffer from a different frame");

    static vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = _swapChain->getRenderPass();
    renderPassInfo.framebuffer = _swapChain->getFrameBuffer(_currentImageIndex);

    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = _swapChain->getSwapChainExtent();

    static std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

    static vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapChain->width());
    viewport.height = static_cast<float>(_swapChain->height());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    static vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = _swapChain->getSwapChainExtent();
    commandBuffer.setViewport(0, 1, &viewport);
    commandBuffer.setScissor(0, 1, &scissor);
}

void Renderer::endSwapChainRenderPass(vk::CommandBuffer commandBuffer)
{
    assert(_isFrameStarted && "Renderer: Can't call endSwapChainRenderPass if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Renderer: Can't end render pass on command buffer from a different frame");

    commandBuffer.endRenderPass();
}

} // namespace cmx
*/
