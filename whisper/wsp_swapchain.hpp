#ifndef WSP_SWAP_CHAIN
#define WSP_SWAP_CHAIN

// lib
#include <vulkan/vulkan.hpp>

// std
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>

class ImGui_ImplVulkan_InitInfo;

namespace wsp
{

class Swapchain
{
  public:
    Swapchain(class Window const *, vk::Extent2D, vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    ~Swapchain();

    Swapchain(Swapchain const &) = delete;
    Swapchain &operator=(Swapchain const &) = delete;

    void PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const;

    void SubmitCommandBuffer(vk::CommandBuffer commandBuffer);

    [[nodiscard]] vk::CommandBuffer NextCommandBuffer();

    vk::SwapchainKHR GetHandle() const;
    uint32_t GetCurrentFrameIndex() const;

    void BeginRenderPass(vk::CommandBuffer, bool isCleared) const;
    void BlitImage(vk::CommandBuffer, vk::Image, vk::Extent2D resolution) const;
    void SkipBlit(vk::CommandBuffer) const;

  protected:
    void AcquireNextImage();

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &);
    static vk::PresentModeKHR ChooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &);
    static vk::Extent2D ChooseSwapExtent(vk::Extent2D, vk::SurfaceCapabilitiesKHR const &);

    void CreateImageViews(uint32_t count);
    std::vector<vk::ImageView> _imageViews;

    void CreateRenderPass();
    vk::RenderPass _renderPass;

    void CreateFramebuffers(uint32_t count);
    std::vector<vk::Framebuffer> _framebuffers;

    void CreateSyncObjects(uint32_t count);
    std::vector<vk::Semaphore> _imageAvailableSemaphores;
    std::vector<vk::Semaphore> _renderFinishedSemaphores;
    std::vector<vk::Fence> _inFlightFences;
    std::vector<vk::Fence> _imagesInFlight;

    vk::SwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    vk::Format _imageFormat;
    vk::Extent2D _extent;
    uint32_t _minImageCount;

    void CreateCommandBuffers();
    std::vector<vk::CommandBuffer> _commandBuffers;

    uint32_t _currentImageIndex;
    uint32_t _currentFrameIndex;
};

} // namespace wsp

#endif
