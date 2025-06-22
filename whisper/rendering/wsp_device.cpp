#include "wsp_device.h"

// wsp
#include "wsp_devkit.h"

// std
#include <cassert>
#include <set>
#include <spdlog/fmt/bundled/base.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

// lib
#include <GLFW/glfw3.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

Device::Device()
{
}

Device::~Device()
{
    if (!_freed)
    {
        spdlog::critical("Device: forgot to Free before deletion");
    }
}

void Device::Initialize(const std::vector<const char *> requiredExtensions, vk::Instance instance,
                        vk::SurfaceKHR surface)
{
    _freed = false;

    PickPhysicalDevice(requiredExtensions, instance, surface);
    CreateLogicalDevice(requiredExtensions, _physicalDevice, surface);
    CreateCommandPool(_physicalDevice, surface);
}

void Device::Free()
{
    if (_freed)
    {
        spdlog::error("Device: already freed device");
        return;
    }

    _device.destroyCommandPool(_commandPool);
    _device.destroy();

    _freed = true;

    spdlog::info("Device: freed device");
}

#ifndef NDEBUG
void Device::PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const
{
    check(_physicalDevice);
    check(_device);

    initInfo->PhysicalDevice = _physicalDevice;
    initInfo->Device = _device;
    initInfo->Queue = _graphicsQueue;
}
#endif

void Device::PickPhysicalDevice(const std::vector<const char *> &requiredExtensions, vk::Instance instance,
                                vk::SurfaceKHR surface)
{
    check(instance);

    const std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    spdlog::info("Device count: {0}", devices.size());
    if (devices.size() == 0)
    {
        throw std::runtime_error("Device: failed to find GPUs with Vulkan support");
    }

    for (const auto &device : devices)
    {
        const QueueFamilyIndices indices = FindQueueFamilies(device, surface);

        const std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

        std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensionsSet.erase(extension.extensionName);
        }

        const bool areExtensionsSupported = requiredExtensionsSet.empty();

        bool isSwapChainAdequate = false;
        if (areExtensionsSupported)
        {
            isSwapChainAdequate =
                !device.getSurfaceFormatsKHR(surface).empty() && !device.getSurfacePresentModesKHR(surface).empty();
        }

        vk::PhysicalDeviceFeatures supportedFeatures;
        supportedFeatures = device.getFeatures();

        if (indices.isComplete() && areExtensionsSupported && isSwapChainAdequate &&
            supportedFeatures.samplerAnisotropy)
        {
            _physicalDevice = device;
            break;
        }
    }

    if (!_physicalDevice)
    {
        throw std::runtime_error("Device: failed to find a suitable GPU");
    }

    vk::PhysicalDeviceProperties properties = _physicalDevice.getProperties();
    spdlog::info("Device: physical device - {0}", (char *)properties.deviceName.data());
}

QueueFamilyIndices Device::FindQueueFamilies(vk::SurfaceKHR surface) const
{
    QueueFamilyIndices indices;

    const std::vector<vk::QueueFamilyProperties> queueFamilies = _physicalDevice.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        const vk::Bool32 presentSupport = _physicalDevice.getSurfaceSupportKHR(i, surface);
        if (queueFamily.queueCount > 0 && presentSupport)
        {
            indices.presentFamily = i;
        }
        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

QueueFamilyIndices Device::FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices;

    const std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        const vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
        if (queueFamily.queueCount > 0 && presentSupport)
        {
            indices.presentFamily = i;
        }
        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

vk::SurfaceCapabilitiesKHR Device::GetSurfaceCapabilitiesKHR(vk::SurfaceKHR surface) const
{
    check(_physicalDevice);
    return _physicalDevice.getSurfaceCapabilitiesKHR(surface);
}

std::vector<vk::SurfaceFormatKHR> Device::GetSurfaceFormatsKHR(vk::SurfaceKHR surface) const
{
    check(_physicalDevice);
    return _physicalDevice.getSurfaceFormatsKHR(surface);
}

std::vector<vk::PresentModeKHR> Device::GetSurfacePresentModesKHR(vk::SurfaceKHR surface) const
{
    check(_physicalDevice);
    return _physicalDevice.getSurfacePresentModesKHR(surface);
}

void Device::CreateLogicalDevice(const std::vector<const char *> &requiredExtensions, vk::PhysicalDevice physicalDevice,
                                 vk::SurfaceKHR surface)
{
    const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    const float queuePriority = 1.0f;
    for (const uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    vk::DeviceCreateInfo createInfo = {};
    createInfo.sType = vk::StructureType::eDeviceCreateInfo;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (const vk::Result result = physicalDevice.createDevice(&createInfo, nullptr, &_device);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("failed to create logical device");
    }

    _graphicsQueue = _device.getQueue(indices.graphicsFamily, 0);
    _presentQueue = _device.getQueue(indices.presentFamily, 0);
}

void Device::CreateCommandPool(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
    check(_device);

    const QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    if (const vk::Result result = _device.createCommandPool(&poolInfo, nullptr, &_commandPool);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("failed to create command pool");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::ResetFences(const std::vector<vk::Fence> &fences) const
{
    check(_device);

    if (const vk::Result result = _device.resetFences(fences.size(), fences.data()); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to reset fence");
    }
}

void Device::WaitForFences(const std::vector<vk::Fence> &fences, bool waitAll, uint64_t timer) const
{
    check(_device);

    if (const vk::Result result = _device.waitForFences(fences.size(), fences.data(), (vk::Bool32)waitAll, timer);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to wait for fence");
    }
}

void Device::SubmitToGraphicsQueue(const std::vector<vk::SubmitInfo *> &submits, vk::Fence fence) const
{
    check(_device);

    if (const vk::Result result = _graphicsQueue.submit(submits.size(), *submits.data(), fence);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to wait for fence");
    }
}

void Device::PresentKHR(const vk::PresentInfoKHR *presentInfo) const
{
    check(_device);

    if (const vk::Result result = _presentQueue.presentKHR(presentInfo); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to present KHR");
    }
}

void Device::CreateSemaphore(const vk::SemaphoreCreateInfo &createInfo, vk::Semaphore *semaphore) const
{
    check(_device);

    if (const vk::Result result = _device.createSemaphore(&createInfo, nullptr, semaphore);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create semaphore");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::CreateFence(const vk::FenceCreateInfo &createInfo, vk::Fence *fence) const
{
    check(_device);

    if (const vk::Result result = _device.createFence(&createInfo, nullptr, fence); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create fence");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::CreateFramebuffer(const vk::FramebufferCreateInfo &createInfo, vk::Framebuffer *framebuffer) const
{
    check(_device);

    if (const vk::Result result = _device.createFramebuffer(&createInfo, nullptr, framebuffer);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create framebuffer");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::CreateRenderPass(const vk::RenderPassCreateInfo &createInfo, vk::RenderPass *renderPass) const
{
    check(_device);

    if (const vk::Result result = _device.createRenderPass(&createInfo, nullptr, renderPass);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create renderPass");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::CreateImageAndBindMemory(const vk::ImageCreateInfo &createInfo, vk::Image *image,
                                      vk::DeviceMemory *imageMemory) const
{
    check(_device);

    if (const vk::Result result = _device.createImage(&createInfo, nullptr, image); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create imageView");
    }

    const vk::MemoryRequirements memRequirements = _device.getImageMemoryRequirements(*image);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(memRequirements.memoryTypeBits, {vk::MemoryPropertyFlagBits::eDeviceLocal});

    if (_device.allocateMemory(&allocInfo, nullptr, imageMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Device: failed to allocate image memory!");
    }

    _device.bindImageMemory(*image, *imageMemory, 0);

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::CreateImageView(const vk::ImageViewCreateInfo &createInfo, vk::ImageView *imageView) const
{
    check(_device);

    if (const vk::Result result = _device.createImageView(&createInfo, nullptr, imageView);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create imageView");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

vk::Sampler Device::CreateSampler(const vk::SamplerCreateInfo &createInfo) const
{
    check(_device);

    vk::Sampler sampler = _device.createSampler(createInfo);

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");

    return sampler;
}

void Device::CreateSwapchainKHR(const vk::SwapchainCreateInfoKHR &createInfo, vk::SwapchainKHR *swapchain) const
{
    check(_device);

    if (const vk::Result result = _device.createSwapchainKHR(&createInfo, nullptr, swapchain);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create swapchain");
    }

    // DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");
}

void Device::CreateDescriptorPool(const vk::DescriptorPoolCreateInfo &createInfo,
                                  vk::DescriptorPool *descriptorPool) const
{
    check(_device);

    if (const vk::Result result = _device.createDescriptorPool(&createInfo, nullptr, descriptorPool);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create descriptor pool");
    }
}

// DebugUtil::nameObject(_renderPass, vk::ObjectType::eRenderPass, "Swapchain RenderPass");

void Device::DestroySemaphore(vk::Semaphore semaphore) const
{
    check(_device);
    _device.destroySemaphore(semaphore, nullptr);
}
void Device::DestroyFence(vk::Fence fence) const
{
    check(_device);
    _device.destroyFence(fence, nullptr);
}
void Device::DestroyFramebuffer(vk::Framebuffer framebuffer) const
{
    check(_device);
    _device.destroyFramebuffer(framebuffer, nullptr);
}
void Device::DestroyRenderPass(vk::RenderPass renderPass) const
{
    check(_device);
    _device.destroyRenderPass(renderPass, nullptr);
}
void Device::DestroyImageView(vk::ImageView imageView) const
{
    check(_device);
    _device.destroyImageView(imageView, nullptr);
}
void Device::DestroySwapchainKHR(vk::SwapchainKHR swapchainKHR) const
{
    check(_device);
    _device.destroySwapchainKHR(swapchainKHR, nullptr);
}
void Device::DestroyDescriptorPool(vk::DescriptorPool descriptorPool) const
{
    check(_device);
    _device.destroyDescriptorPool(descriptorPool, nullptr);
}

uint32_t Device::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Device: failed to find suitable memory type");
}

void Device::AllocateCommandBuffers(std::vector<vk::CommandBuffer> *commandBuffers) const
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers->size());

    if (const vk::Result result = _device.allocateCommandBuffers(&allocInfo, commandBuffers->data());
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to allocate command buffers");
    }
}

void Device::FreeCommandBuffers(std::vector<vk::CommandBuffer> *commandBuffers) const
{
    _device.freeCommandBuffers(_commandPool, static_cast<uint32_t>(commandBuffers->size()), commandBuffers->data());
}

void Device::AcquireNextImageKHR(vk::SwapchainKHR swapchain, vk::Semaphore semaphore, vk::Fence fence,
                                 uint32_t *imageIndex, uint64_t timeout) const
{
    if (const vk::Result result = _device.acquireNextImageKHR(swapchain, timeout, semaphore, fence, imageIndex);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to acquire next imageKHR");
    }
}

std::vector<vk::Image> Device::GetSwapchainImagesKHR(vk::SwapchainKHR swapchain, uint32_t minImageCount) const
{
    if (const vk::Result result = _device.getSwapchainImagesKHR(swapchain, &minImageCount, nullptr);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        spdlog::warn("Device: failed to get swapchain imagesKHR");
    }

    std::vector<vk::Image> images{};
    images.resize(minImageCount);
    if (const vk::Result result = _device.getSwapchainImagesKHR(swapchain, &minImageCount, images.data());
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        spdlog::warn("swapchain: failed to get swapchainimageskhr");
    }

    return images;
}

void Device::WaitIdle() const
{
    _device.waitIdle();
}

} // namespace wsp

// class member functions
/**
bool Device::checkValidationLayerSupport()
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char *layerName : _validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

vk::SampleCountFlagBits Device::getSampleCount()
{
    // Get properties of our new device to know some values
    vk::PhysicalDeviceProperties deviceProperties = _physicalDevice.getProperties();

    vk::SampleCountFlagBits msaaSamples;

    vk::SampleCountFlags counts =
        deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64)
        msaaSamples = vk::SampleCountFlagBits::e64;
    else if (counts & vk::SampleCountFlagBits::e32)
        msaaSamples = vk::SampleCountFlagBits::e32;
    else if (counts & vk::SampleCountFlagBits::e16)
        msaaSamples = vk::SampleCountFlagBits::e16;
    else if (counts & vk::SampleCountFlagBits::e8)
        msaaSamples = vk::SampleCountFlagBits::e8;
    else if (counts & vk::SampleCountFlagBits::e4)
        msaaSamples = vk::SampleCountFlagBits::e4;
    else if (counts & vk::SampleCountFlagBits::e2)
        msaaSamples = vk::SampleCountFlagBits::e2;
    else
        msaaSamples = vk::SampleCountFlagBits::e1;

    return msaaSamples;
}

vk::Format Device::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                       vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        vk::FormatProperties props = _physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format");
}

uint32_t Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

void Device::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                          vk::Buffer &buffer, vk::DeviceMemory &bufferMemory)
{
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (_device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create vertex buffer");
    }

    vk::MemoryRequirements memRequirements = _device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory ");
    }

    _device.bindBufferMemory(buffer, bufferMemory, 0);
}

void Device::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void Device::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, uint32_t depth,
                               uint32_t layerCount)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, depth};

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void Device::createImageWithInfo(const vk::ImageCreateInfo &imageInfo, vk::MemoryPropertyFlags properties,
                                 vk::Image &image, vk::DeviceMemory &imageMemory) const
{
    if (_device.createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create image");
    }

    vk::MemoryRequirements memRequirements = _device.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &imageMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to allocate image memory");
    }

    _device.bindImageMemory(image, imageMemory, 0);
}

void Device::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                   uint32_t mipLevels)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eNone;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    }

    commandBuffer.pipelineBarrier(srcStage, dstStage, {}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    endSingleTimeCommands(commandBuffer);
}
**/
