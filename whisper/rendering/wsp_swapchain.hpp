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

    Swapchain(const Swapchain &) = delete;
    Swapchain &operator=(const Swapchain &) = delete;

    void Free(const class Device *, bool silent = false);

#ifndef NDEBUG
    void PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const;
#endif

    void SubmitCommandBuffer(const class Device *, vk::CommandBuffer commandBuffer);

    [[nodiscard]] vk::CommandBuffer NextCommandBuffer(const class Device *);

    vk::SwapchainKHR GetHandle() const;
    size_t GetCurrentFrameIndeex() const;

    void BeginRenderPass(vk::CommandBuffer, bool isCleared) const;
    void BlitImage(vk::CommandBuffer, vk::Image, vk::Extent2D resolution) const;
    void SkipBlit(vk::CommandBuffer) const;

  protected:
    void AcquireNextImage(const class Device *);

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

    vk::SwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    vk::Format _imageFormat;
    vk::Extent2D _extent;
    uint32_t _minImageCount;

    void CreateCommandBuffers(const class Device *);
    std::vector<vk::CommandBuffer> _commandBuffers;

    uint32_t _currentImageIndex;
    uint32_t _currentFrameIndex;

    bool _freed;
};

} // namespace wsp

#endif
