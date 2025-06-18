#ifndef WSP_SWAP_CHAIN_H
#define WSP_SWAP_CHAIN_H

// lib
#include <vulkan/vulkan.hpp>

// std
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>

namespace wsp
{

class Swapchain
{
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  public:
    Swapchain(const class Window *, const class Device *, const vk::Extent2D &,
              const vk::SwapchainKHR &oldSwapchain = VK_NULL_HANDLE);
    ~Swapchain();

    void Free(const class Device *);

    Swapchain(const Swapchain &) = delete;
    Swapchain &operator=(const Swapchain &) = delete;

    void SubmitCommandBuffer(const class Device *, const vk::CommandBuffer *commandBuffer, uint32_t *imageIndex);

    vk::SwapchainKHR GetHandle() const;

  protected:
    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &);
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &);
    vk::Extent2D ChooseSwapExtent(const vk::Extent2D &, const vk::SurfaceCapabilitiesKHR &);

    void CreateImageViews(const class Device *, size_t count);
    std::vector<vk::ImageView> _imageViews;

    void CreateRenderPass(const class Device *);
    vk::RenderPass _renderPass;

    void CreateFramebuffers(const class Device *, size_t count);
    std::vector<vk::Framebuffer> _framebuffers;

    void CreateSyncObjects(const class Device *);
    std::vector<vk::Semaphore> _imageAvailableSemaphores;
    std::vector<vk::Semaphore> _renderFinishedSemaphores;
    std::vector<vk::Fence> _inFlightFences;
    std::vector<vk::Fence> _imagesInFlight;

    size_t _currentFrame = 0;

    vk::SwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    vk::Format _imageFormat;
    vk::Extent2D _extent;

    bool _freed;
};

} // namespace wsp

#endif
