#ifndef WSP_DEVICE
#define WSP_DEVICE

// lib
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

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

    void Initialize(const std::vector<const char *> requiredExtensions, vk::Instance, vk::SurfaceKHR);
    void Free();

#ifndef NDEBUG
    void PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const;
#endif

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilitiesKHR(vk::SurfaceKHR) const;
    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormatsKHR(vk::SurfaceKHR) const;
    std::vector<vk::PresentModeKHR> GetSurfacePresentModesKHR(vk::SurfaceKHR) const;

    QueueFamilyIndices FindQueueFamilies(vk::SurfaceKHR) const;

    void ResetFences(const std::vector<vk::Fence> &) const;
    void WaitForFences(const std::vector<vk::Fence> &, bool waitAll = true, uint64_t timer = UINT64_MAX) const;

    void SubmitToGraphicsQueue(const std::vector<vk::SubmitInfo *> &, vk::Fence) const;
    void PresentKHR(const vk::PresentInfoKHR *) const;

    void CreateSemaphore(const vk::SemaphoreCreateInfo &, vk::Semaphore *) const;
    void CreateFence(const vk::FenceCreateInfo &, vk::Fence *) const;
    void CreateFramebuffer(const vk::FramebufferCreateInfo &, vk::Framebuffer *) const;
    void CreateRenderPass(const vk::RenderPassCreateInfo &, vk::RenderPass *) const;
    void CreateImageAndBindMemory(const vk::ImageCreateInfo &, vk::Image *, vk::DeviceMemory *) const;
    void CreateImageView(const vk::ImageViewCreateInfo &, vk::ImageView *) const;
    vk::Sampler CreateSampler(const vk::SamplerCreateInfo &) const;
    void CreateSwapchainKHR(const vk::SwapchainCreateInfoKHR &, vk::SwapchainKHR *) const;
    void CreateDescriptorPool(const vk::DescriptorPoolCreateInfo &, vk::DescriptorPool *) const;

    void AllocateCommandBuffers(std::vector<vk::CommandBuffer> *) const;
    void FreeCommandBuffers(std::vector<vk::CommandBuffer> *) const;

    void DestroySemaphore(vk::Semaphore) const;
    void DestroyFence(vk::Fence) const;
    void DestroyFramebuffer(vk::Framebuffer) const;
    void DestroyRenderPass(vk::RenderPass) const;
    void DestroyImage(vk::Image) const;
    void FreeDeviceMemory(vk::DeviceMemory) const;
    void DestroyImageView(vk::ImageView) const;
    void DestroySampler(vk::Sampler) const;
    void DestroySwapchainKHR(vk::SwapchainKHR) const;
    void DestroyDescriptorPool(vk::DescriptorPool) const;

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags) const;

    void AcquireNextImageKHR(vk::SwapchainKHR, vk::Semaphore, vk::Fence, uint32_t *imageIndex,
                             uint64_t timeout = UINT64_MAX) const;
    std::vector<vk::Image> GetSwapchainImagesKHR(vk::SwapchainKHR, uint32_t minImageCount) const;

    void WaitIdle() const;

  protected:
    static QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice, vk::SurfaceKHR);
    void PickPhysicalDevice(const std::vector<const char *> &requiredExtensions, vk::Instance, vk::SurfaceKHR);
    vk::PhysicalDevice _physicalDevice;

    void CreateLogicalDevice(const std::vector<const char *> &requiredExtensions, vk::PhysicalDevice, vk::SurfaceKHR);
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    void CreateCommandPool(vk::PhysicalDevice, vk::SurfaceKHR);
    vk::CommandPool _commandPool;

    bool _freed;
};

} // namespace wsp

#endif
