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
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Swapchain(const class Window *, const class Device *, vk::Extent2D, vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    ~Swapchain();

    void Free(const class Device *);

#ifndef NDEBUG
    void PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const;
#endif

    Swapchain(const Swapchain &) = delete;
    Swapchain &operator=(const Swapchain &) = delete;

    void SubmitCommandBuffer(const class Device *, vk::CommandBuffer commandBuffer, uint32_t imageIndex,
                             uint32_t frameIndex);
    void AcquireNextImage(const class Device *, uint32_t frameIndex, uint32_t *imageIndex) const;

    vk::SwapchainKHR GetHandle() const;
    size_t GetCurrentFrameIndex() const;

    void BeginRenderPass(vk::CommandBuffer, uint32_t frameIndex) const;

  protected:
    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &);
    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &);
    static vk::Extent2D ChooseSwapExtent(vk::Extent2D, const vk::SurfaceCapabilitiesKHR &);

    void CreateImageViews(const class Device *, size_t count);
    std::vector<vk::ImageView> _imageViews;

    void CreateRenderPass(const class Device *);
    vk::RenderPass _renderPass;

    void CreateFramebuffers(const class Device *, size_t count);
    std::vector<vk::Framebuffer> _framebuffers;

    void CreateSyncObjects(const class Device *, size_t count);
    std::vector<vk::Semaphore> _imageAvailableSemaphores;
    std::vector<vk::Semaphore> _renderFinishedSemaphores;
    std::vector<vk::Fence> _inFlightFences;
    std::vector<vk::Fence> _imagesInFlight;

    size_t _currentFrameIndex;

    vk::SwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    vk::Format _imageFormat;
    vk::Extent2D _extent;
    uint32_t _minImageCount;

    bool _freed;
};

} // namespace wsp

#endif
