#ifndef WSP_DEVICE
#define WSP_DEVICE

#include <wsp_constants.hpp>
#include <wsp_devkit.hpp>

#include <vulkan/vulkan.hpp>

#include <tracy/TracyVulkan.hpp>

class ImGui_ImplVulkan_InitInfo;

namespace wsp
{

struct QueueFamilyIndices
{
    int graphicsFamily{INVALID_ID};
    int presentFamily{INVALID_ID};
    bool isComplete() const
    {
        return graphicsFamily != INVALID_ID && presentFamily != INVALID_ID;
    }
};

class Device
{
  public:
    ~Device();

  protected:
    friend class DeviceAccessor;
    friend class SafeDeviceAccessor;

    static Device *Get();
    Device();

    static Device *_instance;

  public:
    Device(Device const &) = delete;
    Device &operator=(Device const &) = delete;

    void Initialize(std::vector<char const *> const requiredExtensions, vk::Instance, vk::SurfaceKHR);
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

    void ResetFences(std::vector<vk::Fence> const &) const;
    void WaitForFences(std::vector<vk::Fence> const &, bool waitAll = true, uint64_t timer = UINT64_MAX) const;

    void SubmitToGraphicsQueue(std::vector<vk::SubmitInfo *> const &, vk::Fence) const;
    void PresentKHR(vk::PresentInfoKHR const *) const;

    void CreateSemaphore(vk::SemaphoreCreateInfo const &, vk::Semaphore *, std::string const &name) const;
    void CreateFence(vk::FenceCreateInfo const &, vk::Fence *, std::string const &name) const;
    void CreateFramebuffer(vk::FramebufferCreateInfo const &, vk::Framebuffer *, std::string const &name) const;
    void CreateRenderPass(vk::RenderPassCreateInfo const &, vk::RenderPass *, std::string const &name) const;
    void CreateImageAndBindMemory(vk::ImageCreateInfo const &, vk::Image *, vk::DeviceMemory *,
                                  std::string const &name) const;
    void CreateBufferAndBindMemory(vk::BufferCreateInfo const &, vk::Buffer *, vk::DeviceMemory *,
                                   vk::MemoryPropertyFlags const &, std::string const &name) const;
    void CopyBuffer(vk::Buffer source, vk::Buffer *destination, uint32_t size) const;
    void MapMemory(vk::DeviceMemory, void **mappedMemory) const;
    void UnmapMemory(vk::DeviceMemory) const;
    void CopyBufferToImage(vk::Buffer source, vk::Image *destination, uint32_t width, uint32_t height,
                           uint32_t depth = 1, uint32_t layerCount = 1) const;
    void FlushMappedMemoryRange(vk::MappedMemoryRange const &mappedMemoryRange) const;
    void CreateImageView(vk::ImageViewCreateInfo const &, vk::ImageView *, std::string const &name) const;
    vk::Sampler CreateSampler(vk::SamplerCreateInfo const &, std::string const &name) const;
    void AllocateDescriptorSet(vk::DescriptorSetAllocateInfo const &, vk::DescriptorSet *,
                               std::string const &name) const;
    void UpdateDescriptorSets(std::vector<vk::WriteDescriptorSet> const &) const;
    void FreeDescriptorSet(vk::DescriptorPool, vk::DescriptorSet *) const;
    void CreateSwapchainKHR(vk::SwapchainCreateInfoKHR const &, vk::SwapchainKHR *, std::string const &name) const;
    void CreateDescriptorPool(vk::DescriptorPoolCreateInfo const &, vk::DescriptorPool *,
                              std::string const &name) const;
    void CreateDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo const &, vk::DescriptorSetLayout *,
                                   std::string const &name) const;
    void CreateShaderModule(std::vector<char> const &code, vk::ShaderModule *, std::string const &name) const;
    void CreatePipelineLayout(vk::PipelineLayoutCreateInfo const &, vk::PipelineLayout *,
                              std::string const &name) const;
    void CreateGraphicsPipeline(vk::GraphicsPipelineCreateInfo const &, vk::Pipeline *, std::string const &name) const;

    void AllocateCommandBuffers(std::vector<vk::CommandBuffer> *) const;
    void FreeCommandBuffers(std::vector<vk::CommandBuffer> *) const;

    void DestroySemaphore(vk::Semaphore *) const;
    void DestroyFence(vk::Fence *) const;
    void DestroyFramebuffer(vk::Framebuffer *) const;
    void DestroyRenderPass(vk::RenderPass *) const;
    void DestroyImage(vk::Image *) const;
    void FreeDeviceMemory(vk::DeviceMemory *) const;
    void DestroyBuffer(vk::Buffer *) const;
    void DestroyImageView(vk::ImageView *) const;
    void DestroySampler(vk::Sampler *) const;
    void DestroySwapchainKHR(vk::SwapchainKHR *) const;
    void DestroyDescriptorPool(vk::DescriptorPool *) const;
    void DestroyDescriptorSetLayout(vk::DescriptorSetLayout *) const;
    void DestroyShaderModule(vk::ShaderModule *) const;
    void DestroyPipelineLayout(vk::PipelineLayout *) const;
    void DestroyGraphicsPipeline(vk::Pipeline *) const;

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags) const;

    void GetMemoryProperties(vk::PhysicalDeviceMemoryProperties2 *,
                             vk::PhysicalDeviceMemoryBudgetPropertiesEXT *) const;

    void AcquireNextImageKHR(vk::SwapchainKHR, vk::Semaphore, vk::Fence, uint32_t *imageIndex,
                             uint64_t timeout = UINT64_MAX) const;
    std::vector<vk::Image> GetSwapchainImagesKHR(vk::SwapchainKHR, uint32_t minImageCount) const;

    void WaitIdle() const;

  protected:
    static QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice, vk::SurfaceKHR);
    void PickPhysicalDevice(std::vector<char const *> const &requiredExtensions, vk::Instance, vk::SurfaceKHR);
    vk::PhysicalDevice _physicalDevice;

    void CreateLogicalDevice(std::vector<char const *> const &requiredExtensions, vk::PhysicalDevice, vk::SurfaceKHR,
                             std::string const &name);
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    void CreateCommandPool(vk::PhysicalDevice, vk::SurfaceKHR, std::string const &name);
    vk::CommandPool _commandPool;

    vk::DispatchLoaderDynamic _debugDispatch;

    bool _freed;
};

template <typename T>
inline void Device::DebugNameObject(T const &object, vk::ObjectType objectType, std::string const &name) const
{
    check(_device);

    uint64_t handle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));

    vk::DebugUtilsObjectNameInfoEXT const nameInfo{objectType, handle, name.c_str()};

    _device.setDebugUtilsObjectNameEXT(nameInfo, _debugDispatch);
}

class DeviceAccessor
{
  protected:
    static Device *Get();

    friend class RenderManager;
};

class SafeDeviceAccessor
{
  protected:
    static Device const *Get();

    friend class Renderer;
    friend class Window;
    friend class Graph;
    friend class StaticTextures;
    friend class AssetsManager;
    friend class Image;
    friend class Texture;
    friend class Sampler;
    friend class Mesh;
    friend class Swapchain;
    friend class Editor;
};

} // namespace wsp

#endif
