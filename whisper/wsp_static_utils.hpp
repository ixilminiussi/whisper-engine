#ifndef WSP_STATIC_UTILS
#define WSP_STATIC_UTILS

// lib
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

// std
#include <fstream>

namespace wsp
{

template <typename T> inline bool inbetween(T val, T min, T max)
{
    return val >= min && val < max;
}

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

    uint32_t const fileSize = static_cast<uint32_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

inline vk::Format SelectFormat(uint32_t channels, size_t size, bool normalized = false)
{
    if (size == 1 && channels == 4)
    {
        return !normalized ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Snorm;
    }
    if (size == 2 && channels == 4)
    {
        return !normalized ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR16G16B16A16Snorm;
    }
    if (size == 4 && channels == 4)
    {
        return vk::Format::eR32G32B32A32Sfloat;
    }
    if (size == 8 && channels == 4)
    {
        return vk::Format::eR64G64B64A64Sfloat;
    }
    if (size == 1 && channels == 3)
    {
        return !normalized ? vk::Format::eR8G8B8Srgb : vk::Format::eR8G8B8Snorm;
    }
    if (size == 2 && channels == 3)
    {
        return !normalized ? vk::Format::eR16G16B16Sfloat : vk::Format::eR16G16B16Snorm;
    }
    if (size == 4 && channels == 3)
    {
        return vk::Format::eR32G32B32Sfloat;
    }
    if (size == 8 && channels == 3)
    {
        return vk::Format::eR64G64B64Sfloat;
    }
    if (size == 1 && channels == 2)
    {
        return !normalized ? vk::Format::eR8G8Srgb : vk::Format::eR8G8Snorm;
    }
    if (size == 2 && channels == 2)
    {
        return !normalized ? vk::Format::eR16G16Sfloat : vk::Format::eR16G16Snorm;
    }
    if (size == 4 && channels == 2)
    {
        return vk::Format::eR32G32Sfloat;
    }
    if (size == 8 && channels == 2)
    {
        return vk::Format::eR64G64Sfloat;
    }
    if (size == 1 && channels == 1)
    {
        return !normalized ? vk::Format::eR8Srgb : vk::Format::eR8Snorm;
    }
    if (size == 2 && channels == 1)
    {
        return !normalized ? vk::Format::eR16Sfloat : vk::Format::eR16Snorm;
    }
    if (size == 4 && channels == 1)
    {
        return !normalized ? vk::Format::eR32Sfloat : vk::Format::eD32Sfloat;
    }
    if (size == 8 && channels == 1)
    {
        return vk::Format::eR64Sfloat;
    }

    return vk::Format::eUndefined;
}

inline void DecomposeFormat(vk::Format format, uint32_t *channels, size_t *size)
{
    switch (format)
    {
    case vk::Format::eR8G8B8A8Sint:
    case vk::Format::eR8G8B8A8Snorm:
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eR8G8B8A8Sscaled:
    case vk::Format::eR8G8B8A8Uint:
    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eR8G8B8A8Uscaled:
    case vk::Format::eB8G8R8A8Sint:
    case vk::Format::eB8G8R8A8Snorm:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eB8G8R8A8Sscaled:
    case vk::Format::eB8G8R8A8Uint:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eB8G8R8A8Uscaled:
    case vk::Format::eA8B8G8R8UnormPack32:
    case vk::Format::eA8B8G8R8SnormPack32:
    case vk::Format::eA8B8G8R8UscaledPack32:
    case vk::Format::eA8B8G8R8SscaledPack32:
    case vk::Format::eA8B8G8R8UintPack32:
    case vk::Format::eA8B8G8R8SintPack32:
    case vk::Format::eA8B8G8R8SrgbPack32:
        *size = 1;
        *channels = 4;
        break;
    case vk::Format::eR16G16B16A16Sint:
    case vk::Format::eR16G16B16A16Snorm:
    case vk::Format::eR16G16B16A16Sfloat:
    case vk::Format::eR16G16B16A16Sscaled:
    case vk::Format::eR16G16B16A16Uint:
    case vk::Format::eR16G16B16A16Unorm:
    case vk::Format::eR16G16B16A16Uscaled:
        *size = 2;
        *channels = 4;
        break;
    case vk::Format::eR32G32B32A32Sint:
    case vk::Format::eR32G32B32A32Sfloat:
    case vk::Format::eR32G32B32A32Uint:
        *size = 4;
        *channels = 4;
        break;
    case vk::Format::eR64G64B64A64Uint:
    case vk::Format::eR64G64B64A64Sint:
    case vk::Format::eR64G64B64A64Sfloat:
        *size = 8;
        *channels = 4;
        break;
    case vk::Format::eR8G8B8Sint:
    case vk::Format::eR8G8B8Snorm:
    case vk::Format::eR8G8B8Srgb:
    case vk::Format::eR8G8B8Sscaled:
    case vk::Format::eR8G8B8Uint:
    case vk::Format::eR8G8B8Unorm:
    case vk::Format::eR8G8B8Uscaled:
    case vk::Format::eB8G8R8Sint:
    case vk::Format::eB8G8R8Snorm:
    case vk::Format::eB8G8R8Srgb:
    case vk::Format::eB8G8R8Sscaled:
    case vk::Format::eB8G8R8Uint:
    case vk::Format::eB8G8R8Unorm:
    case vk::Format::eB8G8R8Uscaled:
        *size = 1;
        *channels = 3;
        break;
    case vk::Format::eR16G16B16Sint:
    case vk::Format::eR16G16B16Snorm:
    case vk::Format::eR16G16B16Sfloat:
    case vk::Format::eR16G16B16Sscaled:
    case vk::Format::eR16G16B16Uint:
    case vk::Format::eR16G16B16Unorm:
    case vk::Format::eR16G16B16Uscaled:
        *size = 2;
        *channels = 3;
        break;
    case vk::Format::eR32G32B32Sint:
    case vk::Format::eR32G32B32Sfloat:
    case vk::Format::eR32G32B32Uint:
        *size = 4;
        *channels = 3;
        break;
    case vk::Format::eR64G64B64Uint:
    case vk::Format::eR64G64B64Sint:
    case vk::Format::eR64G64B64Sfloat:
        *size = 8;
        *channels = 3;
        break;
    case vk::Format::eR8G8Sint:
    case vk::Format::eR8G8Snorm:
    case vk::Format::eR8G8Srgb:
    case vk::Format::eR8G8Sscaled:
    case vk::Format::eR8G8Uint:
    case vk::Format::eR8G8Unorm:
    case vk::Format::eR8G8Uscaled:
        *size = 1;
        *channels = 2;
        break;
    case vk::Format::eR16G16Sint:
    case vk::Format::eR16G16Snorm:
    case vk::Format::eR16G16Sfloat:
    case vk::Format::eR16G16Sscaled:
    case vk::Format::eR16G16Uint:
    case vk::Format::eR16G16Unorm:
    case vk::Format::eR16G16Uscaled:
        *size = 2;
        *channels = 2;
        break;
    case vk::Format::eR32G32Sint:
    case vk::Format::eR32G32Sfloat:
    case vk::Format::eR32G32Uint:
        *size = 4;
        *channels = 2;
        break;
    case vk::Format::eR64G64Uint:
    case vk::Format::eR64G64Sint:
    case vk::Format::eR64G64Sfloat:
        *size = 8;
        *channels = 2;
        break;
    case vk::Format::eR8Sint:
    case vk::Format::eR8Snorm:
    case vk::Format::eR8Srgb:
    case vk::Format::eR8Sscaled:
    case vk::Format::eR8Uint:
    case vk::Format::eR8Unorm:
    case vk::Format::eR8Uscaled:
    case vk::Format::eA8UnormKHR:
    case vk::Format::eS8Uint:
        *size = 1;
        *channels = 1;
        break;
    case vk::Format::eR16Sint:
    case vk::Format::eR16Snorm:
    case vk::Format::eR16Sfloat:
    case vk::Format::eR16Sscaled:
    case vk::Format::eR16Uint:
    case vk::Format::eR16Unorm:
    case vk::Format::eR16Uscaled:
    case vk::Format::eD16Unorm:
        *size = 2;
        *channels = 1;
        break;
    case vk::Format::eR32Sint:
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32Uint:
    case vk::Format::eD32Sfloat:
        *size = 4;
        *channels = 1;
        break;
    case vk::Format::eR64Uint:
    case vk::Format::eR64Sint:
    case vk::Format::eR64Sfloat:
        *size = 8;
        *channels = 1;
        break;
    case vk::Format::eUndefined:
        throw std::invalid_argument("Cannot decompose undefined format");
        break;
    default:
        throw std::invalid_argument("Yet to support specific format");
        break;
    }
}

inline std::string FormatToString(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eR8G8B8A8Sint:
        return std::string("eR8G8B8A8Sint");
        break;
    case vk::Format::eR8G8B8A8Snorm:
        return std::string("eR8G8B8A8Snorm");
        break;
    case vk::Format::eR8G8B8A8Srgb:
        return std::string("eR8G8B8A8Srgb");
        break;
    case vk::Format::eR8G8B8A8Sscaled:
        return std::string("eR8G8B8A8Sscaled");
        break;
    case vk::Format::eR8G8B8A8Uint:
        return std::string("eR8G8B8A8Uint");
        break;
    case vk::Format::eR8G8B8A8Unorm:
        return std::string("eR8G8B8A8Unorm");
        break;
    case vk::Format::eR8G8B8A8Uscaled:
        return std::string("eR8G8B8A8Uscaled");
        break;
    case vk::Format::eB8G8R8A8Sint:
        return std::string("eB8G8R8A8Sint");
        break;
    case vk::Format::eB8G8R8A8Snorm:
        return std::string("eB8G8R8A8Snorm");
        break;
    case vk::Format::eB8G8R8A8Srgb:
        return std::string("eB8G8R8A8Srgb");
        break;
    case vk::Format::eB8G8R8A8Sscaled:
        return std::string("eB8G8R8A8Sscaled");
        break;
    case vk::Format::eB8G8R8A8Uint:
        return std::string("eB8G8R8A8Uint");
        break;
    case vk::Format::eB8G8R8A8Unorm:
        return std::string("eB8G8R8A8Unorm");
        break;
    case vk::Format::eB8G8R8A8Uscaled:
        return std::string("eB8G8R8A8Uscaled");
        break;
    case vk::Format::eA8B8G8R8UnormPack32:
        return std::string("eA8B8G8R8UnormPack32");
        break;
    case vk::Format::eA8B8G8R8SnormPack32:
        return std::string("eA8B8G8R8SnormPack32");
        break;
    case vk::Format::eA8B8G8R8UscaledPack32:
        return std::string("eA8B8G8R8UscaledPack32");
        break;
    case vk::Format::eA8B8G8R8SscaledPack32:
        return std::string("eA8B8G8R8SscaledPack32");
        break;
    case vk::Format::eA8B8G8R8UintPack32:
        return std::string("eA8B8G8R8UintPack32");
        break;
    case vk::Format::eA8B8G8R8SintPack32:
        return std::string("eA8B8G8R8SintPack32");
        break;
    case vk::Format::eA8B8G8R8SrgbPack32:
        return std::string("eA8B8G8R8SrgbPack32");
        break;
    case vk::Format::eR16G16B16A16Sint:
        return std::string("eR16G16B16A16Sint");
        break;
    case vk::Format::eR16G16B16A16Snorm:
        return std::string("eR16G16B16A16Snorm");
        break;
    case vk::Format::eR16G16B16A16Sfloat:
        return std::string("eR16G16B16A16Sfloat");
        break;
    case vk::Format::eR16G16B16A16Sscaled:
        return std::string("eR16G16B16A16Sscaled");
        break;
    case vk::Format::eR16G16B16A16Uint:
        return std::string("eR16G16B16A16Uint");
        break;
    case vk::Format::eR16G16B16A16Unorm:
        return std::string("eR16G16B16A16Unorm");
        break;
    case vk::Format::eR16G16B16A16Uscaled:
        return std::string("eR16G16B16A16Uscaled");
        break;
    case vk::Format::eR32G32B32A32Sint:
        return std::string("eR32G32B32A32Sint");
        break;
    case vk::Format::eR32G32B32A32Sfloat:
        return std::string("eR32G32B32A32Sfloat");
        break;
    case vk::Format::eR32G32B32A32Uint:
        return std::string("eR32G32B32A32Uint");
        break;
    case vk::Format::eR64G64B64A64Uint:
        return std::string("eR64G64B64A64Uint");
        break;
    case vk::Format::eR64G64B64A64Sint:
        return std::string("eR64G64B64A64Sint");
        break;
    case vk::Format::eR64G64B64A64Sfloat:
        return std::string("eR64G64B64A64Sfloat");
        break;
    case vk::Format::eR8G8B8Sint:
        return std::string("eR8G8B8Sint");
        break;
    case vk::Format::eR8G8B8Snorm:
        return std::string("eR8G8B8Snorm");
        break;
    case vk::Format::eR8G8B8Srgb:
        return std::string("eR8G8B8Srgb");
        break;
    case vk::Format::eR8G8B8Sscaled:
        return std::string("eR8G8B8Sscaled");
        break;
    case vk::Format::eR8G8B8Uint:
        return std::string("eR8G8B8Uint");
        break;
    case vk::Format::eR8G8B8Unorm:
        return std::string("eR8G8B8Unorm");
        break;
    case vk::Format::eR8G8B8Uscaled:
        return std::string("eR8G8B8Uscaled");
        break;
    case vk::Format::eB8G8R8Sint:
        return std::string("eB8G8R8Sint");
        break;
    case vk::Format::eB8G8R8Snorm:
        return std::string("eB8G8R8Snorm");
        break;
    case vk::Format::eB8G8R8Srgb:
        return std::string("eB8G8R8Srgb");
        break;
    case vk::Format::eB8G8R8Sscaled:
        return std::string("eB8G8R8Sscaled");
        break;
    case vk::Format::eB8G8R8Uint:
        return std::string("eB8G8R8Uint");
        break;
    case vk::Format::eB8G8R8Unorm:
        return std::string("eB8G8R8Unorm");
        break;
    case vk::Format::eB8G8R8Uscaled:
        return std::string("eB8G8R8Uscaled");
        break;
    case vk::Format::eR16G16B16Sint:
        return std::string("eR16G16B16Sint");
        break;
    case vk::Format::eR16G16B16Snorm:
        return std::string("eR16G16B16Snorm");
        break;
    case vk::Format::eR16G16B16Sfloat:
        return std::string("eR16G16B16Sfloat");
        break;
    case vk::Format::eR16G16B16Sscaled:
        return std::string("eR16G16B16Sscaled");
        break;
    case vk::Format::eR16G16B16Uint:
        return std::string("eR16G16B16Uint");
        break;
    case vk::Format::eR16G16B16Unorm:
        return std::string("eR16G16B16Unorm");
        break;
    case vk::Format::eR16G16B16Uscaled:
        return std::string("eR16G16B16Uscaled");
        break;
    case vk::Format::eR32G32B32Sint:
        return std::string("eR32G32B32Sint");
        break;
    case vk::Format::eR32G32B32Sfloat:
        return std::string("eR32G32B32Sfloat");
        break;
    case vk::Format::eR32G32B32Uint:
        return std::string("eR32G32B32Uint");
        break;
    case vk::Format::eR64G64B64Uint:
        return std::string("eR64G64B64Uint");
        break;
    case vk::Format::eR64G64B64Sint:
        return std::string("eR64G64B64Sint");
        break;
    case vk::Format::eR64G64B64Sfloat:
        return std::string("eR64G64B64Sfloat");
        break;
    case vk::Format::eR8G8Sint:
        return std::string("eR8G8Sint");
        break;
    case vk::Format::eR8G8Snorm:
        return std::string("eR8G8Snorm");
        break;
    case vk::Format::eR8G8Srgb:
        return std::string("eR8G8Srgb");
        break;
    case vk::Format::eR8G8Sscaled:
        return std::string("eR8G8Sscaled");
        break;
    case vk::Format::eR8G8Uint:
        return std::string("eR8G8Uint");
        break;
    case vk::Format::eR8G8Unorm:
        return std::string("eR8G8Unorm");
        break;
    case vk::Format::eR8G8Uscaled:
        return std::string("eR8G8Uscaled");
        break;
    case vk::Format::eR16G16Sint:
        return std::string("eR16G16Sint");
        break;
    case vk::Format::eR16G16Snorm:
        return std::string("eR16G16Snorm");
        break;
    case vk::Format::eR16G16Sfloat:
        return std::string("eR16G16Sfloat");
        break;
    case vk::Format::eR16G16Sscaled:
        return std::string("eR16G16Sscaled");
        break;
    case vk::Format::eR16G16Uint:
        return std::string("eR16G16Uint");
        break;
    case vk::Format::eR16G16Unorm:
        return std::string("eR16G16Unorm");
        break;
    case vk::Format::eR16G16Uscaled:
        return std::string("eR16G16Uscaled");
        break;
    case vk::Format::eR32G32Sint:
        return std::string("eR32G32Sint");
        break;
    case vk::Format::eR32G32Sfloat:
        return std::string("eR32G32Sfloat");
        break;
    case vk::Format::eR32G32Uint:
        return std::string("eR32G32Uint");
        break;
    case vk::Format::eR64G64Uint:
        return std::string("eR64G64Uint");
        break;
    case vk::Format::eR64G64Sint:
        return std::string("eR64G64Sint");
        break;
    case vk::Format::eR64G64Sfloat:
        return std::string("eR64G64Sfloat");
        break;
    case vk::Format::eR8Sint:
        return std::string("eR8Sint");
        break;
    case vk::Format::eR8Snorm:
        return std::string("eR8Snorm");
        break;
    case vk::Format::eR8Srgb:
        return std::string("eR8Srgb");
        break;
    case vk::Format::eR8Sscaled:
        return std::string("eR8Sscaled");
        break;
    case vk::Format::eR8Uint:
        return std::string("eR8Uint");
        break;
    case vk::Format::eR8Unorm:
        return std::string("eR8Unorm");
        break;
    case vk::Format::eR8Uscaled:
        return std::string("eR8Uscaled");
        break;
    case vk::Format::eA8UnormKHR:
        return std::string("eA8UnormKHR");
        break;
    case vk::Format::eS8Uint:
        return std::string("eS8Uint");
        break;
    case vk::Format::eR16Sint:
        return std::string("eR16Sint");
        break;
    case vk::Format::eR16Snorm:
        return std::string("eR16Snorm");
        break;
    case vk::Format::eR16Sfloat:
        return std::string("eR16Sfloat");
        break;
    case vk::Format::eR16Sscaled:
        return std::string("eR16Sscaled");
        break;
    case vk::Format::eR16Uint:
        return std::string("eR16Uint");
        break;
    case vk::Format::eR16Unorm:
        return std::string("eR16Unorm");
        break;
    case vk::Format::eR16Uscaled:
        return std::string("eR16Uscaled");
        break;
    case vk::Format::eD16Unorm:
        return std::string("eD16Unorm");
        break;
    case vk::Format::eR32Sint:
        return std::string("eR32Sint");
        break;
    case vk::Format::eR32Sfloat:
        return std::string("eR32Sfloat");
        break;
    case vk::Format::eR32Uint:
        return std::string("eR32Uint");
        break;
    case vk::Format::eD32Sfloat:
        return std::string("eD32Sfloat");
        break;
    case vk::Format::eR64Uint:
        return std::string("eR64Uint");
        break;
    case vk::Format::eR64Sint:
        return std::string("eR64Sint");
        break;
    case vk::Format::eR64Sfloat:
        return std::string("eR64Sfloat");
        break;
    case vk::Format::eUndefined:
        return std::string("eUndefined");
        break;
    default:
        throw std::invalid_argument("Yet to support specific format");
        break;
    }
}

} // namespace wsp

#endif
