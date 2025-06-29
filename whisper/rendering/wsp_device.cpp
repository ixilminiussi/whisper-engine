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

    check(_device);
    _device.destroyCommandPool(_commandPool);
    _device.destroy();

    _freed = true;

    spdlog::info("Device: succesfully freed");
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

vk::CommandBuffer Device::BeginSingleTimeCommand() const
{
    check(_device);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    if (const vk::Result result = _device.allocateCommandBuffers(&allocInfo, &commandBuffer);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to allocate command buffer");
    }

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    if (const vk::Result result = commandBuffer.begin(&beginInfo); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to begin command buffer");
    }

    return commandBuffer;
}

void Device::EndSingleTimeCommand(vk::CommandBuffer commandBuffer) const
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (const vk::Result result = _graphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to submit graphics queue");
    }
    _graphicsQueue.waitIdle();

    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
}

void Device::CreateLogicalDevice(const std::vector<const char *> &requiredExtensions, vk::PhysicalDevice physicalDevice,
                                 vk::SurfaceKHR surface, const std::string &name)
{
    const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    const float queuePriority = 1.0f;
    for (const uint32_t queueFamily : uniqueQueueFamilies)
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

    if (const vk::Result result = physicalDevice.createDevice(&createInfo, nullptr, &_device);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("failed to create logical device");
    }

    _graphicsQueue = _device.getQueue(indices.graphicsFamily, 0);
    _presentQueue = _device.getQueue(indices.presentFamily, 0);

    DebugNameObject(_device, vk::ObjectType::eDevice, name);
    DebugNameObject(_graphicsQueue, vk::ObjectType::eQueue, "main graphics queue");
    DebugNameObject(_presentQueue, vk::ObjectType::eQueue, "main present queue");
}

void Device::CreateCommandPool(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, const std::string &name)
{
    check(_device);

    const QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    if (const vk::Result result = _device.createCommandPool(&poolInfo, nullptr, &_commandPool);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create command pool");
    }

    DebugNameObject(_commandPool, vk::ObjectType::eCommandPool, name);
}

void Device::CreateTracyContext(TracyVkCtx *tracyCtx)
{
    check(_device);
    check(_physicalDevice);
    check(_graphicsQueue);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    if (const vk::Result result = _device.allocateCommandBuffers(&allocInfo, &commandBuffer);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to allocate command buffer");
    }

    *tracyCtx = TracyVkContext(_physicalDevice, _device, _graphicsQueue, commandBuffer);

    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
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

void Device::CreateSemaphore(const vk::SemaphoreCreateInfo &createInfo, vk::Semaphore *semaphore,
                             const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createSemaphore(&createInfo, nullptr, semaphore);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create semaphore");
    }

    DebugNameObject(*semaphore, vk::ObjectType::eSemaphore, name);
}

void Device::CreateFence(const vk::FenceCreateInfo &createInfo, vk::Fence *fence, const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createFence(&createInfo, nullptr, fence); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create fence");
    }

    DebugNameObject(*fence, vk::ObjectType::eFence, name);
}

void Device::CreateFramebuffer(const vk::FramebufferCreateInfo &createInfo, vk::Framebuffer *framebuffer,
                               const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createFramebuffer(&createInfo, nullptr, framebuffer);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create framebuffer");
    }

    DebugNameObject(*framebuffer, vk::ObjectType::eFramebuffer, name);
}

void Device::CreateRenderPass(const vk::RenderPassCreateInfo &createInfo, vk::RenderPass *renderPass,
                              const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createRenderPass(&createInfo, nullptr, renderPass);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create renderPass");
    }

    DebugNameObject(*renderPass, vk::ObjectType::eRenderPass, name);
}

void Device::CreateImageAndBindMemory(const vk::ImageCreateInfo &createInfo, vk::Image *image,
                                      vk::DeviceMemory *imageMemory, const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createImage(&createInfo, nullptr, image); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create imageView");
    }

    const vk::MemoryRequirements memRequirements = _device.getImageMemoryRequirements(*image);

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

void Device::CreateBufferAndBindMemory(const vk::BufferCreateInfo &createInfo, vk::Buffer *buffer,
                                       vk::DeviceMemory *bufferMemory, const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createBuffer(&createInfo, nullptr, buffer); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create buffer");
    }

    const vk::MemoryRequirements memRequirements = _device.getBufferMemoryRequirements(*buffer);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(memRequirements.memoryTypeBits, {vk::MemoryPropertyFlagBits::eHostVisible});

    if (vk::Result result = _device.allocateMemory(&allocInfo, nullptr, bufferMemory); result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to allocate buffer memory");
    }

    _device.bindBufferMemory(*buffer, *bufferMemory, 0);

    DebugNameObject(*buffer, vk::ObjectType::eBuffer, name);
    DebugNameObject(*bufferMemory, vk::ObjectType::eDeviceMemory, name + " device memory");
}

void Device::MapMemory(vk::DeviceMemory deviceMemory, void **mappedMemory) const
{
    if (const vk::Result result =
            _device.mapMemory(deviceMemory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlagBits{}, mappedMemory);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to map memory");
    }
}

void Device::FlushMappedMemoryRange(const vk::MappedMemoryRange &mappedMemoryRange) const
{
    check(_device);
    _device.flushMappedMemoryRanges({mappedMemoryRange});
}

void Device::CreateImageView(const vk::ImageViewCreateInfo &createInfo, vk::ImageView *imageView,
                             const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createImageView(&createInfo, nullptr, imageView);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create imageView");
    }

    DebugNameObject(*imageView, vk::ObjectType::eImageView, name);
}

vk::Sampler Device::CreateSampler(const vk::SamplerCreateInfo &createInfo, const std::string &name) const
{
    check(_device);

    vk::Sampler sampler = _device.createSampler(createInfo);

    DebugNameObject(sampler, vk::ObjectType::eSampler, name);

    return sampler;
}

void Device::AllocateDescriptorSet(const vk::DescriptorSetAllocateInfo &allocInfo, vk::DescriptorSet *descriptorSet,
                                   const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.allocateDescriptorSets(&allocInfo, descriptorSet);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to allocate descriptor set");
    }

    DebugNameObject(*descriptorSet, vk::ObjectType::eDescriptorSet, name);
}

void Device::UpdateDescriptorSet(const vk::WriteDescriptorSet &writeDescriptor) const
{
    check(_device);

    _device.updateDescriptorSets(1, &writeDescriptor, 0, nullptr);
}

void Device::FreeDescriptorSets(vk::DescriptorPool descriptorPool, std::vector<vk::DescriptorSet> descriptorSets) const
{
    check(_device);

    if (const vk::Result result =
            _device.freeDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to free descriptor sets");
    }
}

void Device::CreateSwapchainKHR(const vk::SwapchainCreateInfoKHR &createInfo, vk::SwapchainKHR *swapchain,
                                const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createSwapchainKHR(&createInfo, nullptr, swapchain);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create swapchain");
    }

    DebugNameObject(*swapchain, vk::ObjectType::eSwapchainKHR, name);
}

void Device::CreateDescriptorPool(const vk::DescriptorPoolCreateInfo &createInfo, vk::DescriptorPool *descriptorPool,
                                  const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createDescriptorPool(&createInfo, nullptr, descriptorPool);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create descriptor pool");
    }

    DebugNameObject(*descriptorPool, vk::ObjectType::eDescriptorPool, name);
}

void Device::CreateDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo &createInfo,
                                       vk::DescriptorSetLayout *descriptorSetLayout, const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createDescriptorSetLayout(&createInfo, nullptr, descriptorSetLayout);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create descriptor set layout");
    }

    DebugNameObject(*descriptorSetLayout, vk::ObjectType::eDescriptorSetLayout, name);
}

void Device::CreateShaderModule(const std::vector<char> &code, vk::ShaderModule *shaderModule,
                                const std::string &name) const
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (const vk::Result result = _device.createShaderModule(&createInfo, nullptr, shaderModule);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create shader module");
    }
}

void Device::CreatePipelineLayout(const vk::PipelineLayoutCreateInfo &createInfo, vk::PipelineLayout *pipelineLayout,
                                  const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createPipelineLayout(&createInfo, nullptr, pipelineLayout);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create pipeline layout");
    }

    DebugNameObject(*pipelineLayout, vk::ObjectType::ePipelineLayout, name);
}

void Device::CreateGraphicsPipeline(const vk::GraphicsPipelineCreateInfo &createInfo, vk::Pipeline *pipeline,
                                    const std::string &name) const
{
    check(_device);

    if (const vk::Result result = _device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &createInfo, nullptr, pipeline);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("ErrorMsg: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Device: failed to create pipeline layout");
    }

    DebugNameObject(*pipeline, vk::ObjectType::ePipeline, name);
}

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
void Device::DestroyImage(vk::Image image) const
{
    check(_device);
    _device.destroyImage(image, nullptr);
}
void Device::DestroyBuffer(vk::Buffer buffer) const
{
    check(_device);
    _device.destroyBuffer(buffer, nullptr);
}
void Device::UnmapMemory(vk::DeviceMemory deviceMemory) const
{
    check(_device);
    _device.unmapMemory(deviceMemory);
}
void Device::FreeDeviceMemory(vk::DeviceMemory deviceMemory) const
{
    check(_device);
    _device.freeMemory(deviceMemory, nullptr);
}
void Device::DestroyImageView(vk::ImageView imageView) const
{
    check(_device);
    _device.destroyImageView(imageView, nullptr);
}
void Device::DestroySampler(vk::Sampler sampler) const
{
    check(_device);
    _device.destroySampler(sampler, nullptr);
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
void Device::DestroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSet) const
{
    check(_device);
    _device.destroyDescriptorSetLayout(descriptorSet, nullptr);
}
void Device::DestroyShaderModule(vk::ShaderModule shaderModule) const
{
    check(_device);
    _device.destroyShaderModule(shaderModule, nullptr);
}
void Device::DestroyPipelineLayout(vk::PipelineLayout pipelineLayout) const
{
    check(_device);
    _device.destroyPipelineLayout(pipelineLayout, nullptr);
}
void Device::DestroyGraphicsPipeline(vk::Pipeline pipeline) const
{
    check(_device);
    _device.destroyPipeline(pipeline, nullptr);
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
