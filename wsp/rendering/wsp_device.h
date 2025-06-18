#ifndef WSP_DEVICE_H
#define WSP_DEVICE_H

// lib
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

struct QueueFamilyIndices
{
    int graphicsFamily{-1};
    int presentFamily{-1};
    bool isComplete()
    {
        return graphicsFamily != -1 && presentFamily != -1;
    }
};

class Device
{
  public:
    Device();
    ~Device();

    void Initialize(const std::vector<const char *> requiredExtensions, const vk::Instance &, const vk::SurfaceKHR &);
    void Free();

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilitiesKHR(const vk::SurfaceKHR &) const;

    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormatsKHR(const vk::SurfaceKHR &) const;

    std::vector<vk::PresentModeKHR> GetSurfacePresentModesKHR(const vk::SurfaceKHR &) const;

    QueueFamilyIndices FindQueueFamilies(const vk::SurfaceKHR &) const;

    void ResetFences(const std::vector<vk::Fence> &) const;
    void WaitForFences(const std::vector<vk::Fence> &, bool waitAll = true, uint64_t timer = UINT64_MAX) const;

    void SubmitToGraphicsQueue(const std::vector<vk::SubmitInfo *> &, const vk::Fence &) const;
    void PresentKHR(const vk::PresentInfoKHR *) const;

    void CreateSemaphore(const vk::SemaphoreCreateInfo *, vk::Semaphore *) const;
    void CreateFence(const vk::FenceCreateInfo *, vk::Fence *) const;
    void CreateFramebuffer(const vk::FramebufferCreateInfo *, vk::Framebuffer *) const;
    void CreateRenderPass(const vk::RenderPassCreateInfo *, vk::RenderPass *) const;
    void CreateImageView(const vk::ImageViewCreateInfo *, vk::ImageView *) const;
    void CreateSwapchainKHR(const vk::SwapchainCreateInfoKHR *, vk::SwapchainKHR *) const;

    void DestroySemaphore(const vk::Semaphore &) const;
    void DestroyFence(const vk::Fence &) const;
    void DestroyFramebuffer(const vk::Framebuffer &) const;
    void DestroyRenderPass(const vk::RenderPass &) const;
    void DestroyImageView(const vk::ImageView &) const;
    void DestroySwapchainKHR(const vk::SwapchainKHR &) const;

    std::vector<vk::Image> GetSwapchainImagesKHR(const vk::SwapchainKHR &, uint32_t *minImageCount) const;

  protected:
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice, const vk::SurfaceKHR &) const;
    void PickPhysicalDevice(const std::vector<const char *> &requiredExtensions, const vk::Instance &,
                            const vk::SurfaceKHR &);
    vk::PhysicalDevice _physicalDevice;

    void CreateLogicalDevice(const std::vector<const char *> &requiredExtensions, const vk::PhysicalDevice &,
                             const vk::SurfaceKHR &);
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    void CreateCommandPool(const vk::PhysicalDevice &, const vk::SurfaceKHR &);
    vk::CommandPool _commandPool;

    bool _freed;
};

} // namespace wsp

#endif
