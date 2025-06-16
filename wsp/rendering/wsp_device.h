#ifndef WSP_DEVICE_H
#define WSP_DEVICE_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

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

    void Initialize();
    void Free();

    void BindWindow(class Window *);

  protected:
#ifndef NDEBUG
    bool CheckValidationLayerSupport();

    vk::Instance _vkInstance;
#endif

    void CreateInstance();
    vk::SurfaceKHR _surface;

    void PickPhysicalDevice(const vk::SurfaceKHR &);
    vk::PhysicalDevice _physicalDevice;

    void CreateLogicalDevice(const vk::PhysicalDevice &, const vk::SurfaceKHR &);
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    void CreateCommandPool(const vk::PhysicalDevice &, const vk::SurfaceKHR &);
    vk::CommandPool _commandPool;

    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice, const vk::SurfaceKHR &) const;

    void ExtensionsCompatibilityTest();
    std::vector<const char *> GetRequiredExtensions();

    const std::vector<const char *> _validationLayers;
    const std::vector<const char *> _deviceExtensions;

    bool _freed;
};

} // namespace wsp

#endif
