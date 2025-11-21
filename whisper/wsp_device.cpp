#include <vulkan/vulkan.hpp>
#include <wsp_device.hpp>

#include <wsp_devkit.hpp>

#include <spdlog/fmt/bundled/base.h>
#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h>

#include <imgui_impl_vulkan.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <set>
#include <stdexcept>

using namespace wsp;

Device *Device::_instance{nullptr};

Device *Device::Get()
{
    if (!_instance)
    {
        _instance = new Device();
    }

    return _instance;
}

Device *DeviceAccessor::Get()
{
    return Device::Get();
}

Device const *SafeDeviceAccessor::Get()
{
    return Device::Get();
}

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

void Device::Initialize(std::vector<char const *> const requiredExtensions, vk::Instance instance,
                        vk::SurfaceKHR surface)
{
    _freed = false;

    _debugDispatch = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);

    PickPhysicalDevice(requiredExtensions, instance, surface);
    CreateLogicalDevice(requiredExtensions, _physicalDevice, surface, "main logical device");
    CreateCommandPool(_physicalDevice, surface, "main command pool");
}

void Device::Free()
{
    if (_freed)
    {
        spdlog::error("Device: already freed device");
        return;
    }

    check(_device && "Device: Must initialize device sooner");
    _device.destroyCommandPool(_commandPool);
    _device.destroy();

    _freed = true;

    spdlog::info("Device: freed");
}

#ifndef NDEBUG
void Device::PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const
{
    check(_physicalDevice);
    check(_device && "Device: Must initialize device sooner");

    initInfo->PhysicalDevice = _physicalDevice;
    initInfo->Device = _device;
    initInfo->Queue = _graphicsQueue;
}
#endif

void Device::PickPhysicalDevice(const std::vector<const char *> &requiredExtensions, vk::Instance instance,
                                vk::SurfaceKHR surface)
{
    check(instance);

    std::vector<vk::PhysicalDevice> const devices = instance.enumeratePhysicalDevices();
    spdlog::info("Device count: {0}", devices.size());
    if (devices.size() == 0)
    {
        throw std::runtime_error("Device: failed to find GPUs with Vulkan support");
    }

    for (auto const &device : devices)
    {
        QueueFamilyIndices const indices = FindQueueFamilies(device, surface);

        std::vector<vk::ExtensionProperties> const availableExtensions = device.enumerateDeviceExtensionProperties();

        std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

        for (auto const &extension : availableExtensions)
        {
            requiredExtensionsSet.erase(extension.extensionName);
        }

        bool const areExtensionsSupported = requiredExtensionsSet.empty();

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

    std::vector<vk::QueueFamilyProperties> const queueFamilies = _physicalDevice.getQueueFamilyProperties();

    int i = 0;
    for (auto const &queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        vk::Bool32 const presentSupport = _physicalDevice.getSurfaceSupportKHR(i, surface);
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

    std::vector<vk::QueueFamilyProperties> const queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (auto const &queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        vk::Bool32 const presentSupport = device.getSurfaceSupportKHR(i, surface);
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

vk::CommandBuffer Device::BeginSingleTimeCommand() const
{
    check(_device && "Device: Must initialize device sooner");

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    if (vk::Result const result = _device.allocateCommandBuffers(&allocInfo, &commandBuffer);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to allocate command buffer : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    if (vk::Result const result = commandBuffer.begin(&beginInfo); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to begin command buffer : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    return commandBuffer;
}

void Device::EndSingleTimeCommand(vk::CommandBuffer commandBuffer) const
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vk::Result const result = _graphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to submit graphics queue : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }
    _graphicsQueue.waitIdle();

    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
}

void Device::CreateLogicalDevice(std::vector<char const *> const &requiredExtensions, vk::PhysicalDevice physicalDevice,
                                 vk::SurfaceKHR surface, std::string const &name)
{
    QueueFamilyIndices const indices = FindQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> const uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float const queuePriority = 1.0f;
    for (uint32_t const queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    vk::DeviceCreateInfo createInfo = {};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (vk::Result const result = physicalDevice.createDevice(&createInfo, nullptr, &_device);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("failed to create logical device : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    _graphicsQueue = _device.getQueue(indices.graphicsFamily, 0);
    _presentQueue = _device.getQueue(indices.presentFamily, 0);

    DebugNameObject(_device, vk::ObjectType::eDevice, name);
    DebugNameObject(_graphicsQueue, vk::ObjectType::eQueue, "main graphics queue");
    DebugNameObject(_presentQueue, vk::ObjectType::eQueue, "main present queue");
}

void Device::CreateCommandPool(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, std::string const &name)
{
    check(_device && "Device: Must initialize device sooner");

    QueueFamilyIndices const queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    if (vk::Result const result = _device.createCommandPool(&poolInfo, nullptr, &_commandPool);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create command pool : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(_commandPool, vk::ObjectType::eCommandPool, name);
}

void Device::CreateTracyContext(TracyVkCtx *tracyCtx)
{
    check(_device && "Device: Must initialize device sooner");
    check(_physicalDevice);
    check(_graphicsQueue);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    if (vk::Result const result = _device.allocateCommandBuffers(&allocInfo, &commandBuffer);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to allocate command buffer : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    *tracyCtx = TracyVkContext(_physicalDevice, _device, _graphicsQueue, commandBuffer);

    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
}

void Device::ResetFences(std::vector<vk::Fence> const &fences) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.resetFences(fences.size(), fences.data()); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to reset fence : {}", vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::WaitForFences(std::vector<vk::Fence> const &fences, bool waitAll, uint64_t timer) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.waitForFences(fences.size(), fences.data(), (vk::Bool32)waitAll, timer);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to wait for fence : {}", vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::SubmitToGraphicsQueue(std::vector<vk::SubmitInfo *> const &submits, vk::Fence fence) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _graphicsQueue.submit(submits.size(), *submits.data(), fence);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to wait for fence : {}", vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::PresentKHR(vk::PresentInfoKHR const *presentInfo) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _presentQueue.presentKHR(presentInfo); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to present KHR : {}", vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::CreateSemaphore(vk::SemaphoreCreateInfo const &createInfo, vk::Semaphore *semaphore,
                             std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createSemaphore(&createInfo, nullptr, semaphore);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create semaphore : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*semaphore, vk::ObjectType::eSemaphore, name);
}

void Device::CreateFence(vk::FenceCreateInfo const &createInfo, vk::Fence *fence, std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createFence(&createInfo, nullptr, fence); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create fence : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*fence, vk::ObjectType::eFence, name);
}

void Device::CreateFramebuffer(vk::FramebufferCreateInfo const &createInfo, vk::Framebuffer *framebuffer,
                               std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createFramebuffer(&createInfo, nullptr, framebuffer);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create framebuffer : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*framebuffer, vk::ObjectType::eFramebuffer, name);
}

void Device::CreateRenderPass(vk::RenderPassCreateInfo const &createInfo, vk::RenderPass *renderPass,
                              std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createRenderPass(&createInfo, nullptr, renderPass);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create renderPass : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*renderPass, vk::ObjectType::eRenderPass, name);
}

void Device::CreateImageAndBindMemory(vk::ImageCreateInfo const &createInfo, vk::Image *image,
                                      vk::DeviceMemory *imageMemory, std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createImage(&createInfo, nullptr, image); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create imageView : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    vk::MemoryRequirements const memRequirements = _device.getImageMemoryRequirements(*image);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(memRequirements.memoryTypeBits, {vk::MemoryPropertyFlagBits::eDeviceLocal});

    if (_device.allocateMemory(&allocInfo, nullptr, imageMemory) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Device: failed to allocate image memory");
    }

    _device.bindImageMemory(*image, *imageMemory, 0);

    DebugNameObject(*image, vk::ObjectType::eImage, name);
    DebugNameObject(*imageMemory, vk::ObjectType::eDeviceMemory, name + " device memory");
}

void Device::CreateBufferAndBindMemory(vk::BufferCreateInfo const &createInfo, vk::Buffer *buffer,
                                       vk::DeviceMemory *bufferMemory,
                                       vk::MemoryPropertyFlags const &memoryPropertyFlags,
                                       std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createBuffer(&createInfo, nullptr, buffer); result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create buffer : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    vk::MemoryRequirements const memRequirements = _device.getBufferMemoryRequirements(*buffer);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

    if (vk::Result const result = _device.allocateMemory(&allocInfo, nullptr, bufferMemory);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to allocate buffer memory : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    _device.bindBufferMemory(*buffer, *bufferMemory, 0);

    DebugNameObject(*buffer, vk::ObjectType::eBuffer, name);
    DebugNameObject(*bufferMemory, vk::ObjectType::eDeviceMemory, name + " device memory");
}

void Device::CopyBuffer(vk::Buffer source, vk::Buffer *destination, size_t size) const
{
    vk::CommandBuffer const commandBuffer = BeginSingleTimeCommand();

    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    commandBuffer.copyBuffer(source, *destination, 1, &copyRegion);

    EndSingleTimeCommand(commandBuffer);
}

void Device::MapMemory(vk::DeviceMemory deviceMemory, void **mappedMemory) const
{
    if (vk::Result const result =
            _device.mapMemory(deviceMemory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlagBits{}, mappedMemory);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to map memory : {}", vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::CopyBufferToImage(vk::Buffer source, vk::Image *destination, size_t width, size_t height,
                               size_t depth) const
{
    vk::CommandBuffer const commandBuffer = BeginSingleTimeCommand();

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {},
                                  {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eNone,
                                                         vk::AccessFlagBits::eTransferWrite,
                                                         vk::ImageLayout::eUndefined,
                                                         vk::ImageLayout::eTransferDstOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         *destination,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

    vk::BufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.imageOffset = 0;
    copyRegion.imageExtent = vk::Extent3D{(uint32_t)width, (uint32_t)height, (uint32_t)depth};
    copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;

    commandBuffer.copyBufferToImage(source, *destination, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllGraphics, {}, {},
                                  {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eTransferWrite,
                                                         vk::AccessFlagBits::eShaderRead,
                                                         vk::ImageLayout::eTransferDstOptimal,
                                                         vk::ImageLayout::eShaderReadOnlyOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         *destination,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

    EndSingleTimeCommand(commandBuffer);
}

void Device::FlushMappedMemoryRange(vk::MappedMemoryRange const &mappedMemoryRange) const
{
    check(_device && "Device: Must initialize device sooner");
    _device.flushMappedMemoryRanges({mappedMemoryRange});
}

void Device::CreateImageView(vk::ImageViewCreateInfo const &createInfo, vk::ImageView *imageView,
                             std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createImageView(&createInfo, nullptr, imageView);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create imageView : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*imageView, vk::ObjectType::eImageView, name);
}

vk::Sampler Device::CreateSampler(vk::SamplerCreateInfo const &createInfo, std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    vk::Sampler sampler = _device.createSampler(createInfo);

    DebugNameObject(sampler, vk::ObjectType::eSampler, name);

    return sampler;
}

void Device::AllocateDescriptorSet(vk::DescriptorSetAllocateInfo const &allocInfo, vk::DescriptorSet *descriptorSet,
                                   std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.allocateDescriptorSets(&allocInfo, descriptorSet);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to allocate descriptor set : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*descriptorSet, vk::ObjectType::eDescriptorSet, name);
}

void Device::UpdateDescriptorSets(std::vector<vk::WriteDescriptorSet> const &writeDescriptors) const
{
    check(_device && "Device: Must initialize device sooner");

    _device.updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
}

void Device::FreeDescriptorSet(vk::DescriptorPool descriptorPool, vk::DescriptorSet *descriptorSet) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.freeDescriptorSets(descriptorPool, 1, descriptorSet);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to free descriptor sets : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    *descriptorSet = VK_NULL_HANDLE;
}

void Device::CreateSwapchainKHR(vk::SwapchainCreateInfoKHR const &createInfo, vk::SwapchainKHR *swapchain,
                                std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createSwapchainKHR(&createInfo, nullptr, swapchain);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create swapchain : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*swapchain, vk::ObjectType::eSwapchainKHR, name);
}

void Device::CreateDescriptorPool(vk::DescriptorPoolCreateInfo const &createInfo, vk::DescriptorPool *descriptorPool,
                                  std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createDescriptorPool(&createInfo, nullptr, descriptorPool);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to create descriptor pool : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*descriptorPool, vk::ObjectType::eDescriptorPool, name);
}

void Device::CreateDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo const &createInfo,
                                       vk::DescriptorSetLayout *descriptorSetLayout, std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createDescriptorSetLayout(&createInfo, nullptr, descriptorSetLayout);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to create descriptor set layout : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*descriptorSetLayout, vk::ObjectType::eDescriptorSetLayout, name);
}

void Device::CreateShaderModule(std::vector<char> const &code, vk::ShaderModule *shaderModule,
                                std::string const &name) const
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<uint32_t const *>(code.data());

    if (vk::Result const result = _device.createShaderModule(&createInfo, nullptr, shaderModule);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Device: failed to create shader module : {}", vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::CreatePipelineLayout(vk::PipelineLayoutCreateInfo const &createInfo, vk::PipelineLayout *pipelineLayout,
                                  std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createPipelineLayout(&createInfo, nullptr, pipelineLayout);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to create pipeline layout : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*pipelineLayout, vk::ObjectType::ePipelineLayout, name);
}

void Device::CreateGraphicsPipeline(vk::GraphicsPipelineCreateInfo const &createInfo, vk::Pipeline *pipeline,
                                    std::string const &name) const
{
    check(_device && "Device: Must initialize device sooner");

    if (vk::Result const result = _device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &createInfo, nullptr, pipeline);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to create pipeline layout : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

    DebugNameObject(*pipeline, vk::ObjectType::ePipeline, name);
}

void Device::DestroySemaphore(vk::Semaphore *semaphore) const
{
    check(*semaphore != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroySemaphore(*semaphore, nullptr);
    *semaphore = VK_NULL_HANDLE;
}
void Device::DestroyFence(vk::Fence *fence) const
{
    check(*fence != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyFence(*fence, nullptr);
    *fence = VK_NULL_HANDLE;
}
void Device::DestroyFramebuffer(vk::Framebuffer *framebuffer) const
{
    check(*framebuffer != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyFramebuffer(*framebuffer, nullptr);
    *framebuffer = VK_NULL_HANDLE;
}
void Device::DestroyRenderPass(vk::RenderPass *renderPass) const
{
    check(*renderPass != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyRenderPass(*renderPass, nullptr);
    *renderPass = VK_NULL_HANDLE;
}
void Device::DestroyImage(vk::Image *image) const
{
    check(*image != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyImage(*image, nullptr);
    *image = VK_NULL_HANDLE;
}
void Device::DestroyBuffer(vk::Buffer *buffer) const
{
    check(*buffer != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyBuffer(*buffer, nullptr);
    *buffer = VK_NULL_HANDLE;
}
void Device::UnmapMemory(vk::DeviceMemory *deviceMemory) const
{
    check(*deviceMemory != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.unmapMemory(*deviceMemory);
    *deviceMemory = VK_NULL_HANDLE;
}
void Device::FreeDeviceMemory(vk::DeviceMemory *deviceMemory) const
{
    check(*deviceMemory != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.freeMemory(*deviceMemory, nullptr);
    *deviceMemory = VK_NULL_HANDLE;
}
void Device::DestroyImageView(vk::ImageView *imageView) const
{
    check(*imageView != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyImageView(*imageView, nullptr);
    *imageView = VK_NULL_HANDLE;
}
void Device::DestroySampler(vk::Sampler *sampler) const
{
    check(*sampler != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroySampler(*sampler, nullptr);
    *sampler = VK_NULL_HANDLE;
}
void Device::DestroySwapchainKHR(vk::SwapchainKHR *swapchainKHR) const
{
    check(*swapchainKHR != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroySwapchainKHR(*swapchainKHR, nullptr);
    *swapchainKHR = VK_NULL_HANDLE;
}
void Device::DestroyDescriptorPool(vk::DescriptorPool *descriptorPool) const
{
    check(*descriptorPool != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyDescriptorPool(*descriptorPool, nullptr);
    *descriptorPool = VK_NULL_HANDLE;
}
void Device::DestroyDescriptorSetLayout(vk::DescriptorSetLayout *descriptorSet) const
{
    check(*descriptorSet != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyDescriptorSetLayout(*descriptorSet, nullptr);
    *descriptorSet = VK_NULL_HANDLE;
}
void Device::DestroyShaderModule(vk::ShaderModule *shaderModule) const
{
    check(*shaderModule != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyShaderModule(*shaderModule, nullptr);
    *shaderModule = VK_NULL_HANDLE;
}
void Device::DestroyPipelineLayout(vk::PipelineLayout *pipelineLayout) const
{
    check(*pipelineLayout != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyPipelineLayout(*pipelineLayout, nullptr);
    *pipelineLayout = VK_NULL_HANDLE;
}
void Device::DestroyGraphicsPipeline(vk::Pipeline *pipeline) const
{
    check(*pipeline != VK_NULL_HANDLE);
    check(_device && "Device: Must initialize device sooner");
    _device.destroyPipeline(*pipeline, nullptr);
    *pipeline = VK_NULL_HANDLE;
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
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers->size());

    if (vk::Result const result = _device.allocateCommandBuffers(&allocInfo, commandBuffers->data());
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to allocate command buffers : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }
}

void Device::FreeCommandBuffers(std::vector<vk::CommandBuffer> *commandBuffers) const
{
    _device.freeCommandBuffers(_commandPool, static_cast<uint32_t>(commandBuffers->size()), commandBuffers->data());
}

void Device::AcquireNextImageKHR(vk::SwapchainKHR swapchain, vk::Semaphore semaphore, vk::Fence fence,
                                 uint32_t *imageIndex, uint64_t timeout) const
{
    if (vk::Result const result = _device.acquireNextImageKHR(swapchain, timeout, semaphore, fence, imageIndex);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Device: failed to acquire next imageKHR : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }
}

std::vector<vk::Image> Device::GetSwapchainImagesKHR(vk::SwapchainKHR swapchain, uint32_t minImageCount) const
{
    if (vk::Result const result = _device.getSwapchainImagesKHR(swapchain, &minImageCount, nullptr);
        result != vk::Result::eSuccess)
    {
        spdlog::warn(fmt::format("Device: failed to get swapchain imagesKHR : {}",
                                 vk::to_string(static_cast<vk::Result>(result))));
    }

    std::vector<vk::Image> images{};
    images.resize(minImageCount);
    if (vk::Result const result = _device.getSwapchainImagesKHR(swapchain, &minImageCount, images.data());
        result != vk::Result::eSuccess)
    {
        spdlog::warn(fmt::format("Swapchain: failed to get swapchainimageskhr : {}",
                                 vk::to_string(static_cast<vk::Result>(result))));
    }

    return images;
}

void Device::WaitIdle() const
{
    _device.waitIdle();
}
