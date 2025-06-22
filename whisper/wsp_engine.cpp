#include "wsp_engine.h"

#include "fl_graph.h"
#include "wsp_device.h"
#include "wsp_editor.h"
#include "wsp_renderer.h"
#include "wsp_static_utils.h"
#include "wsp_window.h"

// lib
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

// std
#include <stdexcept>
#include <unordered_set>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

namespace engine
{

const std::vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName};

bool initialized{false};
bool terminated{false};

vk::Instance vkInstance;
Window *window{nullptr};
Device *device{nullptr};
Renderer *renderer{nullptr};
Editor *editor{nullptr};

fl::Graph *graph{nullptr};

const vk::Instance &GetVulkanInstance()
{
    return vkInstance;
}

Device *GetDevice()
{
    return device;
}

#ifndef NDEBUG
bool CheckValidationLayerSupport()
{
    const std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}
#endif

std::vector<const char *> GetRequiredExtensions()
{
    uint32_t extensionCount = 0;
    const char **extensions;
    extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char *> requiredExtensions(extensions, extensions + extensionCount);

#ifndef NDEBUG
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return requiredExtensions;
}

void ExtensionsCompatibilityTest()
{
    const std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    spdlog::info("available extensions:");
    std::unordered_set<std::string> available;
    for (const vk::ExtensionProperties &extension : extensions)
    {
        spdlog::info("\t{0}", extension.extensionName.data());
        available.insert(extension.extensionName);
    }

    spdlog::info("required extensions:");
    std::vector<const char *> requiredExtensions = GetRequiredExtensions();
    for (const char *&required : requiredExtensions)
    {
        spdlog::info("\t{0}", required);
        if (available.find(required) == available.end())
        {
            throw std::runtime_error("Engine: missing required glfw extension");
        }
    }
}

void CreateInstance()
{
#ifndef NDEBUG
    if (!CheckValidationLayerSupport())
    {
        throw std::runtime_error("Engine: validation layers requested, but not available!");
    }
#endif

    vk::ApplicationInfo appInfo = {};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "Whisper App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Whisper Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo = {};
    createInfo.sType = vk::StructureType::eInstanceCreateInfo;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char *> extensions = GetRequiredExtensions();

    for (const char *ext : extensions)
    {
        spdlog::info("Engine: Requested extension: {}", ext);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;
    debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                  vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                  vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugCreateInfo.pfnUserCallback = DebugCallback;

    debugCreateInfo.pNext = nullptr;

    createInfo.pNext = &debugCreateInfo;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif

    if (const vk::Result result = vk::createInstance(&createInfo, nullptr, &vkInstance); result != vk::Result::eSuccess)
    {
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Engine: failed to create vkInstance!");
    }

#ifndef NDEBUG
    if (const vk::Result result = CreateDebugUtilsMessengerEXT(vkInstance, &debugCreateInfo, nullptr, &debugMessenger);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Engine: failed to set up debug messenger.");
    }
#endif

    ExtensionsCompatibilityTest();
}

bool Initialize()
{
    spdlog::info("Engine: began initialization");

    if (!glfwInit())
    {
        spdlog::critical("Engine: GLFW failed to initialize!");
        return false;
    }
    if (initialized)
    {
        spdlog::error("Engine: already initialized");
        return true;
    }

    try
    {
        CreateInstance();

        window = new Window(vkInstance, 1024, 1024, "test");

        device = new Device();
        device->Initialize(deviceExtensions, vkInstance, *window->GetSurface());

        window->SetDevice(device);
        window->BuildSwapchain();

        renderer = new Renderer(device, window);
        editor = new Editor(window, device, vkInstance);

        graph = new fl::Graph(device);

        initialized = true;
        terminated = false;

        spdlog::info("Engine: succesfully initialized");
    }
    catch (const std::exception &exception)
    {
        spdlog::critical("{0}", exception.what());
        return false;
    }

    return true;
}

void Run()
{
    while (window && !window->ShouldClose())
    {
        glfwPollEvents();

        const vk::CommandBuffer commandBuffer = renderer->BeginRender(device);

        editor->Render(commandBuffer);

        renderer->EndRender(device);
    }
}

void Terminate()
{
    if (!initialized)
    {
        spdlog::error("Engine: never initialized before termination");
        return;
    }
    if (terminated)
    {
        spdlog::error("Engine: already terminated");
        return;
    }

    try
    {
        device->WaitIdle();

        spdlog::info("Engine: began termination");

        editor->Free(device);
        delete editor;

        renderer->Free(device);
        delete renderer;

        window->Free(vkInstance);
        delete window;

#ifndef NDEBUG
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(vkInstance),
                                                                               "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(static_cast<VkInstance>(vkInstance), debugMessenger, nullptr);
        }
#endif

        device->Free();
        delete device;

        vkInstance.destroy();

        glfwTerminate();

        initialized = false;
        terminated = true;

        spdlog::info("Engine: succesfully terminated");
    }
    catch (const std::exception &exception)
    {
        spdlog::critical("{0}", exception.what());
        return;
    }
}

} // namespace engine

} // namespace wsp
