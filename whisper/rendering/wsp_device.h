#ifndef WSP_DEVICE
#define WSP_DEVICE

// lib
#include "wsp_devkit.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
//
#include <tracy/TracyVulkan.hpp>

class ImGui_ImplVulkan_InitInfo;

namespace wsp
{

struct QueueFamilyIndices
{
    int graphicsFamily{-1};
    int presentFamily{-1};
    bool isComplete() const
    {
        return graphicsFamily != -1 && presentFamily != -1;
    }
};

class Device
{
  public:
    Device();
    ~Device();

    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    void Initialize(const std::vector<const char *> requiredExtensions, vk::Instance, vk::SurfaceKHR);
    void Free();

#ifndef NDEBUG
    void PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const;
#endif

    template <typename T> void DebugNameObject(const T &object, vk::ObjectType, const std::string &name) const;

    void CreateTracyContext(TracyVkCtx *);

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilitiesKHR(vk::SurfaceKHR) const;
    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormatsKHR(vk::SurfaceKHR) const;
    std::vector<vk::PresentModeKHR> GetSurfacePresentModesKHR(vk::SurfaceKHR) const;

    vk::CommandBuffer BeginSingleTimeCommand() const;
    void EndSingleTimeCommand(vk::CommandBuffer commandBuffer) const;

    QueueFamilyIndices FindQueueFamilies(vk::SurfaceKHR) const;

    void ResetFences(const std::vector<vk::Fence> &) const;
    void WaitForFences(const std::vector<vk::Fence> &, bool waitAll = true, uint64_t timer = UINT64_MAX) const;

    void SubmitToGraphicsQueue(const std::vector<vk::SubmitInfo *> &, vk::Fence) const;
    void PresentKHR(const vk::PresentInfoKHR *) const;

    void CreateSemaphore(const vk::SemaphoreCreateInfo &, vk::Semaphore *, const std::string &name) const;
    void CreateFence(const vk::FenceCreateInfo &, vk::Fence *, const std::string &name) const;
    void CreateFramebuffer(const vk::FramebufferCreateInfo &, vk::Framebuffer *, const std::string &name) const;
    void CreateRenderPass(const vk::RenderPassCreateInfo &, vk::RenderPass *, const std::string &name) const;
    void CreateImageAndBindMemory(const vk::ImageCreateInfo &, vk::Image *, vk::DeviceMemory *,
                                  const std::string &name) const;
    void CreateBufferAndBindMemory(const vk::BufferCreateInfo &, vk::Buffer *, vk::DeviceMemory *,
                                   const std::string &name) const;
    void MapMemory(vk::DeviceMemory, void **mappedMemory) const;
    void FlushMappedMemoryRange(const vk::MappedMemoryRange &mappedMemoryRange) const;
    void CreateImageView(const vk::ImageViewCreateInfo &, vk::ImageView *, const std::string &name) const;
    vk::Sampler CreateSampler(const vk::SamplerCreateInfo &, const std::string &name) const;
    void AllocateDescriptorSet(const vk::DescriptorSetAllocateInfo &, vk::DescriptorSet *,
                               const std::string &name) const;
    void UpdateDescriptorSet(const vk::WriteDescriptorSet &) const;
    void FreeDescriptorSets(vk::DescriptorPool, std::vector<vk::DescriptorSet>) const;
    void CreateSwapchainKHR(const vk::SwapchainCreateInfoKHR &, vk::SwapchainKHR *, const std::string &name) const;
    void CreateDescriptorPool(const vk::DescriptorPoolCreateInfo &, vk::DescriptorPool *,
                              const std::string &name) const;
    void CreateDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo &, vk::DescriptorSetLayout *,
                                   const std::string &name) const;
    void CreateShaderModule(const std::vector<char> &code, vk::ShaderModule *, const std::string &name) const;
    void CreatePipelineLayout(const vk::PipelineLayoutCreateInfo &, vk::PipelineLayout *,
                              const std::string &name) const;
    void CreateGraphicsPipeline(const vk::GraphicsPipelineCreateInfo &, vk::Pipeline *, const std::string &name) const;

    void AllocateCommandBuffers(std::vector<vk::CommandBuffer> *) const;
    void FreeCommandBuffers(std::vector<vk::CommandBuffer> *) const;

    void DestroySemaphore(vk::Semaphore) const;
    void DestroyFence(vk::Fence) const;
    void DestroyFramebuffer(vk::Framebuffer) const;
    void DestroyRenderPass(vk::RenderPass) const;
    void DestroyImage(vk::Image) const;
    void FreeDeviceMemory(vk::DeviceMemory) const;
    void DestroyBuffer(vk::Buffer) const;
    void UnmapMemory(vk::DeviceMemory) const;
    void DestroyImageView(vk::ImageView) const;
    void DestroySampler(vk::Sampler) const;
    void DestroySwapchainKHR(vk::SwapchainKHR) const;
    void DestroyDescriptorPool(vk::DescriptorPool) const;
    void DestroyDescriptorSetLayout(vk::DescriptorSetLayout) const;
    void DestroyShaderModule(vk::ShaderModule) const;
    void DestroyPipelineLayout(vk::PipelineLayout) const;
    void DestroyGraphicsPipeline(vk::Pipeline) const;

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags) const;

    void AcquireNextImageKHR(vk::SwapchainKHR, vk::Semaphore, vk::Fence, uint32_t *imageIndex,
                             uint64_t timeout = UINT64_MAX) const;
    std::vector<vk::Image> GetSwapchainImagesKHR(vk::SwapchainKHR, uint32_t minImageCount) const;

    void WaitIdle() const;

  protected:
    static QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice, vk::SurfaceKHR);
    void PickPhysicalDevice(const std::vector<const char *> &requiredExtensions, vk::Instance, vk::SurfaceKHR);
    vk::PhysicalDevice _physicalDevice;

    void CreateLogicalDevice(const std::vector<const char *> &requiredExtensions, vk::PhysicalDevice, vk::SurfaceKHR,
                             const std::string &name);
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    void CreateCommandPool(vk::PhysicalDevice, vk::SurfaceKHR, const std::string &name);
    vk::CommandPool _commandPool;

    vk::DispatchLoaderDynamic _debugDispatch;

    bool _freed;
};

template <typename T>
inline void Device::DebugNameObject(const T &object, vk::ObjectType objectType, const std::string &name) const
{
    check(_device);

    uint64_t handle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));

    const vk::DebugUtilsObjectNameInfoEXT nameInfo{objectType, handle, name.c_str()};

    _device.setDebugUtilsObjectNameEXT(nameInfo, _debugDispatch);
}

} // namespace wsp

#endif
