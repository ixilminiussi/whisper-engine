#include "wsp_device.h"

// wsp
#include "wsp/wsp_devkit.h"
#include "wsp_static_utils.h"
#include "wsp_window.h"

// std
#include <cassert>
#include <set>
#include <stdexcept>
#include <unordered_set>

// lib
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

Device::Device()
    : _validationLayers{"VK_LAYER_KHRONOS_validation"},
      _deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName}
{
}

Device::~Device()
{
    if (!_freed)
    {
        spdlog::critical("Device: forgot to Free before deletion");
    }
}

void Device::Initialize()
{
    CreateInstance();
    _freed = false;
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

#ifndef NDEBUG
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(_vkInstance),
                                                                           "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(static_cast<VkInstance>(_vkInstance), debugMessenger, nullptr);
    }
#endif

    _vkInstance.destroySurfaceKHR(_surface);
    _vkInstance.destroy();

    _freed = true;
}

void Device::CreateInstance()
{
#ifndef NDEBUG
    if (!CheckValidationLayerSupport())
    {
        throw std::runtime_error("Device: validation layers requested, but not available!");
    }
#endif

    vk::ApplicationInfo appInfo = {};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "Whisper App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Whisper Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo = {};
    createInfo.sType = vk::StructureType::eInstanceCreateInfo;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char *> extensions = GetRequiredExtensions();

    for (const char *ext : extensions)
    {
        spdlog::info("Requested extension: {}", ext);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
    createInfo.ppEnabledLayerNames = _validationLayers.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;
    debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                  vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                  vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugCreateInfo.pfnUserCallback = DebugCallback;

    debugCreateInfo.pNext = nullptr;

    createInfo.pNext = &debugCreateInfo;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif

    if (vk::createInstance(&createInfo, nullptr, &_vkInstance) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Device: failed to create _vkInstance!");
    }

#ifndef NDEBUG
    if (CreateDebugUtilsMessengerEXT(_vkInstance, &debugCreateInfo, nullptr, &debugMessenger) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Device: failed to set up debug messenger.");
    }
#endif

    ExtensionsCompatibilityTest();
}

#ifndef NDEBUG
bool Device::CheckValidationLayerSupport()
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
#endif

void Device::ExtensionsCompatibilityTest()
{
    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    spdlog::info("available extensions:");
    std::unordered_set<std::string> available;
    for (const vk::ExtensionProperties &extension : extensions)
    {
        spdlog::info("\t{0}", extension.extensionName.data());
        available.insert(extension.extensionName);
    }

    spdlog::info("required extensions:");
    std::vector<const char *> requiredExtensions = GetRequiredExtensions();
    for (const char *&required : requiredExtensions)
    {
        spdlog::info("\t{0}", required);
        if (available.find(required) == available.end())
        {
            throw std::runtime_error("Device: missing required glfw extension");
        }
    }
}

std::vector<const char *> Device::GetRequiredExtensions()
{
    uint32_t extensionCount = 0;
    const char **extensions;
    extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char *> requiredExtensions(extensions, extensions + extensionCount);

#ifndef NDEBUG
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return requiredExtensions;
}

void Device::BindWindow(Window *window)
{
    check(_vkInstance);

    window->CreateWindowSurface(_vkInstance, &_surface);

    PickPhysicalDevice(_surface);
    CreateLogicalDevice(_physicalDevice, _surface);
    CreateCommandPool(_physicalDevice, _surface);
}

void Device::PickPhysicalDevice(const vk::SurfaceKHR &surface)
{
    check(_vkInstance);

    std::vector<vk::PhysicalDevice> devices = _vkInstance.enumeratePhysicalDevices();
    spdlog::info("Device count: {0}", devices.size());
    if (devices.size() == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const auto &device : devices)
    {
        QueueFamilyIndices indices = FindQueueFamilies(device, surface);

        std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

        std::set<std::string> requiredExtensions(_deviceExtensions.begin(), _deviceExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        bool extensionsSupported = requiredExtensions.empty();

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            swapChainAdequate =
                !device.getSurfaceFormatsKHR(surface).empty() && !device.getSurfacePresentModesKHR(surface).empty();
        }

        vk::PhysicalDeviceFeatures supportedFeatures;
        supportedFeatures = device.getFeatures();

        if (indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy)
        {
            _physicalDevice = device;
            break;
        }
    }

    if (_physicalDevice == nullptr)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    vk::PhysicalDeviceProperties properties = _physicalDevice.getProperties();
    spdlog::info("Device: physical device - {0}", (char *)properties.deviceName.data());
}

QueueFamilyIndices Device::FindQueueFamilies(vk::PhysicalDevice device, const vk::SurfaceKHR &surface) const
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
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

void Device::CreateLogicalDevice(const vk::PhysicalDevice &physicalDevice, const vk::SurfaceKHR &surface)
{
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

// might not really be necessary anymore because device specific validation layers
// have been deprecated
#ifndef NDEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
    createInfo.ppEnabledLayerNames = _validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    if (physicalDevice.createDevice(&createInfo, nullptr, &_device) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    _graphicsQueue = _device.getQueue(indices.graphicsFamily, 0);
    _presentQueue = _device.getQueue(indices.presentFamily, 0);
}

void Device::CreateCommandPool(const vk::PhysicalDevice &physicalDevice, const vk::SurfaceKHR &surface)
{
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    if (_device.createCommandPool(&poolInfo, nullptr, &_commandPool) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

// class member functions
/**
Device::Device(Window &window) : _window{window}
{
    DebugUtil::initialize(_device, _instance);
}

Device::~Device()
{
    _device.destroyCommandPool(_commandPool);
    _device.destroy();

    if (_enableValidationLayers)
    {
        destroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }

    _instance.destroySurfaceKHR(_surface);
    _instance.destroy();
}


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

SwapChainSupportDetails Device::querySwapChainSupport(vk::PhysicalDevice device)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(_surface);

    details.formats = device.getSurfaceFormatsKHR(_surface);

    details.presentModes = device.getSurfacePresentModesKHR(_surface);

    return details;
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
    throw std::runtime_error("failed to find supported format!");
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

    throw std::runtime_error("failed to find suitable memory type!");
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
        throw std::runtime_error("failed to create vertex buffer!");
    }

    vk::MemoryRequirements memRequirements = _device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory !");
    }

    _device.bindBufferMemory(buffer, bufferMemory, 0);
}

vk::CommandBuffer Device::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    if (_device.allocateCommandBuffers(&allocInfo, &commandBuffer) != vk::Result::eSuccess)
    {
        spdlog::warn("Device: failed to allocate command buffer !");
    }

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
    {
        spdlog::warn("Device: failed to begin command buffer !");
    }
    return commandBuffer;
}

void Device::endSingleTimeCommands(vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (_graphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE) != vk::Result::eSuccess)
    {
        spdlog::warn("Device: failed to submit graphics queue !");
    }
    _graphicsQueue.waitIdle();

    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
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
        throw std::runtime_error("failed to create image!");
    }

    vk::MemoryRequirements memRequirements = _device.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &imageMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to allocate image memory!");
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

} // namespace wsp
