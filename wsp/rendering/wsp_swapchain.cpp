#include "wsp_swapchain.h"
#include "imgui_impl_vulkan.h"
#include "wsp_device.h"
#include "wsp_devkit.h"
#include "wsp_window.h"

// lib
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

Swapchain::Swapchain(const class Window *window, const class Device *device, vk::Extent2D extent2D,
                     vk::SwapchainKHR oldSwapchain)
    : _freed{false}, _imageAvailableSemaphores{}, _renderFinishedSemaphores{}, _inFlightFences{}, _imagesInFlight{},
      _images{}
{
    const vk::SurfaceKHR *surface = window->GetSurface();
    const vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(device->GetSurfaceFormatsKHR(*surface));
    const vk::PresentModeKHR presentMode = ChooseSwapPresentMode(device->GetSurfacePresentModesKHR(*surface));
    const vk::SurfaceCapabilitiesKHR capabilities = device->GetSurfaceCapabilitiesKHR(*surface);
    const vk::Extent2D extent = ChooseSwapExtent(window->GetExtent(), capabilities);

    _minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && _minImageCount > capabilities.maxImageCount)
    {
        _minImageCount = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    createInfo.surface = *surface;

    createInfo.minImageCount = _minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    const QueueFamilyIndices indices = device->FindQueueFamilies(*surface);
    int queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = (uint32_t *)queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = oldSwapchain;

    device->CreateSwapchainKHR(createInfo, &_swapchain);

    _images = device->GetSwapchainImagesKHR(_swapchain, _minImageCount);
    _imageFormat = surfaceFormat.format;

    _extent = extent;

    CreateImageViews(device, _images.size());
    CreateRenderPass(device);
    CreateFramebuffers(device, _images.size());
    CreateSyncObjects(device, _images.size());
}

Swapchain::~Swapchain()
{
    if (!_freed)
    {
        spdlog::error("Swapchain: forgot to Free before deletion");
    }
}

void Swapchain::Free(const Device *device)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        device->DestroySemaphore(_renderFinishedSemaphores[i]);
        device->DestroySemaphore(_imageAvailableSemaphores[i]);
        device->DestroyFence(_inFlightFences[i]);
    }
    _renderFinishedSemaphores.clear();
    _imageAvailableSemaphores.clear();
    _inFlightFences.clear();

    for (const vk::Framebuffer &framebuffer : _framebuffers)
    {
        device->DestroyFramebuffer(framebuffer);
    }
    _framebuffers.clear();

    device->DestroyRenderPass(_renderPass);

    for (size_t i = 0; i < _imageViews.size(); i++)
    {
        device->DestroyImageView(_imageViews[i]);
    }
    _imageViews.clear();

    if (_swapchain)
    {
        device->DestroySwapchainKHR(_swapchain);
        _swapchain = nullptr;
    }

    _freed = true;
    spdlog::info("Swapchain: freed swapchain");
}

#ifndef NDEBUG
void Swapchain::PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const
{
    initInfo->MinImageCount = _minImageCount;
    initInfo->ImageCount = _images.size();
    initInfo->MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo->RenderPass = _renderPass;
}
#endif

void Swapchain::SubmitCommandBuffer(const Device *device, vk::CommandBuffer commandBuffers, uint32_t imageIndex,
                                    uint32_t frameIndex)
{
    check(imageIndex < _imagesInFlight.size());

    if (_imagesInFlight[imageIndex] != nullptr)
    {
        device->WaitForFences({_imagesInFlight[imageIndex]});
    }
    _imagesInFlight[imageIndex] = _inFlightFences[frameIndex];

    vk::SubmitInfo submitInfo = {};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    vk::Semaphore waitSemaphores[] = {_imageAvailableSemaphores[frameIndex]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers;

    vk::Semaphore signalSemaphores[] = {_renderFinishedSemaphores[frameIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    device->ResetFences({_inFlightFences[frameIndex]});
    device->SubmitToGraphicsQueue({&submitInfo}, _inFlightFences[frameIndex]);

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = {_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    try
    {
        device->PresentKHR(&presentInfo);
    }
    catch (...)
    {
    }
}

void Swapchain::AcquireNextImage(const Device *device, uint32_t frameIndex, uint32_t *imageIndex) const
{
    device->WaitForFences({_inFlightFences[frameIndex]});

    device->AcquireNextImageKHR(_swapchain, _imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, imageIndex);
}

vk::SwapchainKHR Swapchain::GetHandle() const
{
    return _swapchain;
}

void Swapchain::BeginRenderPass(vk::CommandBuffer commandBuffer, uint32_t frameIndex) const
{
    check(frameIndex < _framebuffers.size());

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffers[frameIndex];

    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = _extent;

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_extent.width);
    viewport.height = static_cast<float>(_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = _extent;
    commandBuffer.setViewport(0, 1, &viewport);
    commandBuffer.setScissor(0, 1, &scissor);
}

vk::SurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
    for (const vk::SurfaceFormatKHR &availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Swapchain::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    for (const vk::PresentModeKHR &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            spdlog::info("Swapchain: Present mode: Mailbox");
            return availablePresentMode;
        }
    }

    // for (const vk::PresentModeKHR &availablePresentMode : availablePresentModes) {
    //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //     spdlog::info("Present mode: Immediate");
    //     return availablePresentMode;
    //   }
    // }

    spdlog::info("Swapchain: present mode: V-sync");
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::ChooseSwapExtent(vk::Extent2D windowExtent, const vk::SurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        vk::Extent2D actualExtent = windowExtent;
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Swapchain::CreateImageViews(const Device *device, size_t count)
{
    check(_images.size() <= count);

    _imageViews.resize(count);
    for (size_t i = 0; i < count; i++)
    {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
        viewInfo.image = _images[i];
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = _imageFormat;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        device->CreateImageView(viewInfo, &_imageViews[i]);
    }
}

void Swapchain::CreateRenderPass(const Device *device)
{
    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.format = _imageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    vk::SubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    dependency.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;
    dependency.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

    std::array<vk::AttachmentDescription, 1> attachments = {colorAttachment};
    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    device->CreateRenderPass(renderPassInfo, &_renderPass);
}

void Swapchain::CreateFramebuffers(const Device *device, size_t count)
{
    check(_imageViews.size() <= count);

    _framebuffers.resize(count);
    for (size_t i = 0; i < count; i++)
    {
        std::array<vk::ImageView, 1> attachments = {_imageViews[i]};

        const vk::Extent2D swapChainExtent = _extent;
        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        device->CreateFramebuffer(framebufferInfo, &_framebuffers[i]);
    }
}

void Swapchain::CreateSyncObjects(const Device *device, size_t count)
{
    check(_images.size() <= count);

    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    _imagesInFlight.resize(count, VK_NULL_HANDLE);

    vk::SemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    vk::FenceCreateInfo fenceInfo = {};
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        device->CreateSemaphore(semaphoreInfo, &_imageAvailableSemaphores[i]);
        device->CreateSemaphore(semaphoreInfo, &_renderFinishedSemaphores[i]);
        device->CreateFence(fenceInfo, &_inFlightFences[i]);
    }
}

} // namespace wsp
/**

vk::Format Swapchain::findDepthFormat()
{
    return _device.findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

} // namespace cmx
**/
