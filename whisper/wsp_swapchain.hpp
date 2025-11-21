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
    Swapchain(class Window const *, class Device const *, vk::Extent2D, vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    ~Swapchain();

    Swapchain(Swapchain const &) = delete;
    Swapchain &operator=(Swapchain const &) = delete;

    void Free(class Device const *, bool silent = false);

#ifndef NDEBUG
    void PopulateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *initInfo) const;
#endif

    void SubmitCommandBuffer(class Device const *, vk::CommandBuffer commandBuffer);

    [[nodiscard]] vk::CommandBuffer NextCommandBuffer(class Device const *);

    vk::SwapchainKHR GetHandle() const;
    size_t GetCurrentFrameIndeex() const;

    void BeginRenderPass(vk::CommandBuffer, bool isCleared) const;
    void BlitImage(vk::CommandBuffer, vk::Image, vk::Extent2D resolution) const;
    void SkipBlit(vk::CommandBuffer) const;

  protected:
    void AcquireNextImage(class Device const *);

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &);
    static vk::PresentModeKHR ChooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &);
    static vk::Extent2D ChooseSwapExtent(vk::Extent2D, vk::SurfaceCapabilitiesKHR const &);

    void CreateImageViews(class Device const *, size_t count);
    std::vector<vk::ImageView> _imageViews;

    void CreateRenderPass(class Device const *);
    vk::RenderPass _renderPass;

    void CreateFramebuffers(class Device const *, size_t count);
    std::vector<vk::Framebuffer> _framebuffers;

    void CreateSyncObjects(class Device const *, size_t count);
    std::vector<vk::Semaphore> _imageAvailableSemaphores;
    std::vector<vk::Semaphore> _renderFinishedSemaphores;
    std::vector<vk::Fence> _inFlightFences;
    std::vector<vk::Fence> _imagesInFlight;

    vk::SwapchainKHR _swapchain;
    std::vector<vk::Image> _images;
    vk::Format _imageFormat;
    vk::Extent2D _extent;
    uint32_t _minImageCount;

    void CreateCommandBuffers(class Device const *);
    std::vector<vk::CommandBuffer> _commandBuffers;

    uint32_t _currentImageIndex;
    uint32_t _currentFrameIndex;

    bool _freed;
};

} // namespace wsp

#endif
