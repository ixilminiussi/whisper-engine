#include <wsp_swapchain.hpp>

#include <wsp_constants.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_engine.hpp>
#include <wsp_renderer.hpp>
#include <wsp_window.hpp>

#include <imgui_impl_vulkan.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <string>

using namespace wsp;

Swapchain::Swapchain(Window const *window, vk::Extent2D extent2D, vk::SwapchainKHR oldSwapchain)
    : _imageAvailableSemaphores{}, _renderFinishedSemaphores{}, _inFlightFences{}, _imagesInFlight{}, _images{},
      _currentImageIndex{0}, _currentFrameIndex{0}
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    vk::SurfaceKHR const surface = window->GetSurface();
    vk::SurfaceFormatKHR const surfaceFormat = ChooseSwapSurfaceFormat(device->GetSurfaceFormatsKHR(surface));
    vk::PresentModeKHR const presentMode = ChooseSwapPresentMode(device->GetSurfacePresentModesKHR(surface));
    vk::SurfaceCapabilitiesKHR const capabilities = device->GetSurfaceCapabilitiesKHR(surface);
    vk::Extent2D const extent = ChooseSwapExtent(window->GetExtent(), capabilities);

    _minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && _minImageCount > capabilities.maxImageCount)
    {
        _minImageCount = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.surface = surface;

    createInfo.minImageCount = _minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

    QueueFamilyIndices const indices = device->FindQueueFamilies(surface);
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

    device->CreateSwapchainKHR(createInfo, &_swapchain, "swapchain");

    _images = device->GetSwapchainImagesKHR(_swapchain, _minImageCount);
    _imageFormat = surfaceFormat.format;

    _extent = extent;

    CreateImageViews(_images.size());
    CreateRenderPass();
    CreateFramebuffers(_images.size());
    CreateSyncObjects(_images.size());
    CreateCommandBuffers();
}

Swapchain::~Swapchain()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        device->DestroySemaphore(&_renderFinishedSemaphores[i]);
        device->DestroySemaphore(&_imageAvailableSemaphores[i]);
        device->DestroyFence(&_inFlightFences[i]);
    }
    _renderFinishedSemaphores.clear();
    _imageAvailableSemaphores.clear();
    _inFlightFences.clear();

    for (vk::Framebuffer &framebuffer : _framebuffers)
    {
        device->DestroyFramebuffer(&framebuffer);
    }
    _framebuffers.clear();

    device->DestroyRenderPass(&_renderPass);

    for (uint32_t i = 0; i < _imageViews.size(); i++)
    {
        device->DestroyImageView(&_imageViews[i]);
    }
    _imageViews.clear();

    if (_swapchain)
    {
        device->DestroySwapchainKHR(&_swapchain);
        _swapchain = nullptr;
    }

    spdlog::debug("Swapchain: freed");
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

void Swapchain::SubmitCommandBuffer(vk::CommandBuffer commandBuffers)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_currentImageIndex < _imagesInFlight.size());

    if (_imagesInFlight[_currentImageIndex] != nullptr)
    {
        device->WaitForFences({_imagesInFlight[_currentImageIndex]});
    }
    _imagesInFlight[_currentImageIndex] = _inFlightFences[_currentFrameIndex];

    vk::SubmitInfo submitInfo = {};

    vk::Semaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrameIndex]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers;

    vk::Semaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrameIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    device->ResetFences({_inFlightFences[_currentFrameIndex]});
    device->SubmitToGraphicsQueue({&submitInfo}, _inFlightFences[_currentFrameIndex]);

    vk::PresentInfoKHR presentInfo = {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = {_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &_currentImageIndex;

    _currentFrameIndex = (_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    device->PresentKHR(&presentInfo);
}

[[nodiscard]] vk::CommandBuffer Swapchain::NextCommandBuffer()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    AcquireNextImage();
    vk::CommandBuffer commandBuffer = _commandBuffers[_currentFrameIndex];

    vk::CommandBufferBeginInfo const beginInfo{};

    if (vk::Result const result = commandBuffer.begin(&beginInfo); result != vk::Result::eSuccess)
    {
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Renderer: failed to begin recording command buffer!");
    }

    extern TracyVkCtx TRACY_CTX;
    TracyVkCollect(TRACY_CTX, commandBuffer);

    return commandBuffer;
}

void Swapchain::AcquireNextImage()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->WaitForFences({_inFlightFences[_currentFrameIndex]});

    device->AcquireNextImageKHR(_swapchain, _imageAvailableSemaphores[_currentFrameIndex], VK_NULL_HANDLE,
                                &_currentImageIndex);
}

vk::SwapchainKHR Swapchain::GetHandle() const
{
    return _swapchain;
}

uint32_t Swapchain::GetCurrentFrameIndex() const
{
    return _currentFrameIndex;
}

void Swapchain::BeginRenderPass(vk::CommandBuffer commandBuffer, bool isCleared) const
{
    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffers[_currentImageIndex];

    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = _extent;

    renderPassInfo.clearValueCount = 0;
    renderPassInfo.pClearValues = nullptr;

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

    if (isCleared)
    {
        extern TracyVkCtx TRACY_CTX;
        TracyVkZone(TRACY_CTX, commandBuffer, "clear");
        vk::ClearAttachment clearAttachment = {};
        clearAttachment.aspectMask = vk::ImageAspectFlagBits::eColor;
        clearAttachment.clearValue.color = vk::ClearColorValue(std::array<float, 4>{0., 0., 0., 1.f});
        clearAttachment.colorAttachment = 0;

        vk::ClearRect clearRect = {};
        clearRect.rect.offset = vk::Offset2D{0, 0};
        clearRect.rect.extent = _extent;
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        commandBuffer.clearAttachments(clearAttachment, clearRect);
    }

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

void Swapchain::BlitImage(vk::CommandBuffer commandBuffer, vk::Image image, vk::Extent2D resolution) const
{
    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "blit");

    vk::Image const swapchainImage = _images.at(_currentImageIndex);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eNone,
                                                         vk::AccessFlagBits::eTransferWrite,
                                                         vk::ImageLayout::eUndefined,
                                                         vk::ImageLayout::eTransferDstOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         swapchainImage,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eColorAttachmentWrite,
                                                         vk::AccessFlagBits::eTransferWrite,
                                                         vk::ImageLayout::eUndefined,
                                                         vk::ImageLayout::eTransferSrcOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         image,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

    vk::ImageBlit blitRegion{};
    blitRegion.srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
    blitRegion.srcOffsets[1] = vk::Offset3D{(int)resolution.width, (int)resolution.height, 1};
    blitRegion.dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
    blitRegion.dstOffsets[1] = vk::Offset3D{(int)_extent.width, (int)_extent.height, 1};

    commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, swapchainImage,
                            vk::ImageLayout::eTransferDstOptimal, 1, &blitRegion, vk::Filter::eLinear);
}

void Swapchain::SkipBlit(vk::CommandBuffer commandBuffer) const
{
    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "skip blit");

    vk::Image const swapchainImage = _images.at(_currentImageIndex);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eNone,
                                                         vk::AccessFlagBits::eColorAttachmentWrite,
                                                         vk::ImageLayout::eUndefined,
                                                         vk::ImageLayout::eTransferDstOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         swapchainImage,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
}

vk::SurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats)
{
    for (vk::SurfaceFormatKHR const &availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Swapchain::ChooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes)
{
    for (vk::PresentModeKHR const &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    spdlog::info("Swapchain: present mode: V-sync");
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::ChooseSwapExtent(vk::Extent2D windowExtent, vk::SurfaceCapabilitiesKHR const &capabilities)
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

void Swapchain::CreateImageViews(uint32_t count)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);
    check(_images.size() <= count);

    _imageViews.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = _images[i];
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = _imageFormat;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        device->CreateImageView(viewInfo, &_imageViews[i], "swapchain image view " + std::to_string(i));
    }
}

void Swapchain::CreateRenderPass()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.format = _imageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    // colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    // colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.initialLayout = vk::ImageLayout::eTransferDstOptimal;

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
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    device->CreateRenderPass(renderPassInfo, &_renderPass, "swapchain render pass");
}

void Swapchain::CreateFramebuffers(uint32_t count)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_imageViews.size() <= count);

    _framebuffers.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        std::array<vk::ImageView, 1> attachments = {_imageViews[i]};

        vk::Extent2D const swapChainExtent = _extent;
        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        device->CreateFramebuffer(framebufferInfo, &_framebuffers[i], "swapchain framebuffer");
    }
}

void Swapchain::CreateSyncObjects(uint32_t count)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_images.size() <= count);

    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    _imagesInFlight.resize(count, VK_NULL_HANDLE);

    vk::SemaphoreCreateInfo const semaphoreInfo = {};

    vk::FenceCreateInfo fenceInfo = {};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        device->CreateSemaphore(semaphoreInfo, &_imageAvailableSemaphores[i],
                                "swapchain semaphore " + std::to_string(i));
        device->CreateSemaphore(semaphoreInfo, &_renderFinishedSemaphores[i],
                                "swapchain semaphore " + std::to_string(i));
        device->CreateFence(fenceInfo, &_inFlightFences[i], "swapchain fence " + std::to_string(i));
    }
}

void Swapchain::CreateCommandBuffers()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    device->AllocateCommandBuffers(&_commandBuffers);
}
