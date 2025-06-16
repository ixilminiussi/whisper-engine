#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

#ifndef NDEBUG

static vk::DebugUtilsMessengerEXT debugMessenger{nullptr};

inline VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                      void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        spdlog::error("Validation layer: {0}", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        spdlog::warn("Validation layer: {0}", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        spdlog::info("Validation layer: {0}", pCallbackData->pMessage);
        break;
    default:
        spdlog::info("Validation layer: {0}", pCallbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

inline vk::Result CreateDebugUtilsMessengerEXT(const vk::Instance instance,
                                               const vk::DebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                               const vk::AllocationCallbacks *pAllocator,
                                               const vk::DebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return static_cast<vk::Result>(func(instance, (VkDebugUtilsMessengerCreateInfoEXT *)pCreateInfo,
                                            (VkAllocationCallbacks *)pAllocator,
                                            (VkDebugUtilsMessengerEXT *)pDebugMessenger));
    }
    else
    {
        return vk::Result::eErrorExtensionNotPresent;
    }
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}
#endif

} // namespace wsp
