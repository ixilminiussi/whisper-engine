#ifndef WSP_STATIC_UTILS
#define WSP_STATIC_UTILS

// lib
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

// std
#include <fstream>

namespace wsp
{

inline float decodeSRGB(float c)
{
    if (c <= 0.04045f)
        return c * (1.0f / 12.92f);
    else
        return powf((c + 0.055f) / 1.055f, 2.4f);
}

#ifndef NDEBUG
inline ImVec4 decodeSRGB(ImVec4 const &srgb)
{
    return ImVec4(decodeSRGB(srgb.x), decodeSRGB(srgb.y), decodeSRGB(srgb.z), srgb.w);
}
#endif
inline glm::vec4 decodeSRGB(glm::vec4 const &srgb)
{
    return glm::vec4{decodeSRGB(srgb.x), decodeSRGB(srgb.y), decodeSRGB(srgb.z), srgb.w};
}

#ifndef NDEBUG

static vk::DebugUtilsMessengerEXT debugMessenger{nullptr};

inline VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
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

inline vk::Result CreateDebugUtilsMessengerEXT(vk::Instance const instance,
                                               vk::DebugUtilsMessengerCreateInfoEXT const *pCreateInfo,
                                               vk::AllocationCallbacks const *pAllocator,
                                               vk::DebugUtilsMessengerEXT const *pDebugMessenger)
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
                                          VkAllocationCallbacks const *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}
#endif

inline std::vector<char> ReadShaderFile(std::string const &filepath)
{
    std::ifstream file{std::string(SHADER_FILES) + filepath, std::ios::ate | std::ios::binary};
    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file: " + filepath);
    }

    size_t const fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

} // namespace wsp

#endif
