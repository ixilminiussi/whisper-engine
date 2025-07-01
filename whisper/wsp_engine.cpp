#include "wsp_engine.h"

#include "wsp_device.h"
#include "wsp_editor.h"
#include "wsp_graph.h"
#include "wsp_static_utils.h"
#include "wsp_window.h"

// lib
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

// std
#include <stdexcept>
#include <unordered_set>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace wsp
{

namespace engine
{

struct GlobalUbo
{
    float a;
};

const std::vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName};

bool initialized{false};
bool terminated{false};

vk::Instance vkInstance;
Window *window{nullptr};
Device *device{nullptr};
Graph *graph{nullptr};

Resource color{0};
Resource reverse{0};

Editor *editor{nullptr};

TracyVkCtx tracyCtx;

TracyVkCtx TracyCtx()
{
    return tracyCtx;
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
    ZoneScopedN("CreateInstance");
#ifndef NDEBUG
    if (!CheckValidationLayerSupport())
    {
        throw std::runtime_error("Engine: validation layers requested, but not available!");
    }
#endif

    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName = "Whisper App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Whisper Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo = {};
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

void OnEditorToggle(void *, bool isActive)
{
    if (!isActive)
    {
        graph->Compile(device, color, Graph::GraphGoal::eToTransfer);
        window->BindResizeCallback(graph, Graph::OnResizeCallback);
    }
    else
    {
        graph->Compile(device, color, Graph::GraphGoal::eToDescriptorSet);
        window->UnbindResizeCallback(graph);
    }
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
        device->Initialize(deviceExtensions, vkInstance, window->GetSurface());
        device->CreateTracyContext(&tracyCtx);

        window->Initialize(device);

        check(device);

        graph = new Graph(device, window->GetExtent().width, window->GetExtent().height);
        graph->SetUboSize(sizeof(GlobalUbo));

        ResourceCreateInfo colorResourceInfo{};
        colorResourceInfo.role = ResourceRole::eColor;
        colorResourceInfo.format = vk::Format::eR8G8B8A8Unorm;
        colorResourceInfo.clear.color = vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f};
        colorResourceInfo.debugName = "color";

        color = graph->NewResource(colorResourceInfo);

        PassCreateInfo passCreateInfo{};
        passCreateInfo.writes = {color};
        passCreateInfo.reads = {};
        passCreateInfo.readsUniform = true;
        passCreateInfo.vertFile = "triangle.vert.spv";
        passCreateInfo.fragFile = "triangle.frag.spv";
        passCreateInfo.debugName = "white triangle";
        passCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(3, 1, 0, 0); };

        graph->NewPass(passCreateInfo);

        colorResourceInfo.debugName = "reverse color";
        reverse = graph->NewResource(colorResourceInfo);

        PassCreateInfo postPassCreateInfo{};
        postPassCreateInfo.writes = {reverse};
        postPassCreateInfo.reads = {color};
        postPassCreateInfo.vertFile = "postprocess.vert.spv";
        postPassCreateInfo.fragFile = "postprocess.frag.spv";
        postPassCreateInfo.debugName = "post processing";
        postPassCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(6, 1, 0, 0); };

        graph->NewPass(postPassCreateInfo);
        graph->Compile(device, reverse, Graph::GraphGoal::eToDescriptorSet);

#ifndef NDEBUG
        editor = new Editor(window, device, vkInstance);
        editor->BindToggle(nullptr, OnEditorToggle);
#endif

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
        FrameMarkStart("frame");
        glfwPollEvents();

        size_t frameIndex = 0;
        const vk::CommandBuffer commandBuffer = window->NextCommandBuffer(&frameIndex);

        GlobalUbo ubo{};
        ubo.a = 0.5;

        graph->FlushUbo(&ubo, frameIndex, device);

        graph->Render(commandBuffer);

        if (editor->isActive())
        {
            window->SwapchainOpen(commandBuffer);
        }
        else
        {
            window->SwapchainOpen(commandBuffer, graph->GetTargetImage());
        }
        editor->Render(commandBuffer, graph, device);

        window->SwapchainFlush(commandBuffer);
        editor->Update(0.f);

        FrameMarkEnd("frame");
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

        graph->Free(device);
        delete graph;

        window->Free(vkInstance);
        delete window;

        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(vkInstance),
                                                                               "vkDestroyDebugUtilsMessengerEXT");
#ifndef NDEBUG
        if (func != nullptr)
        {
            func(static_cast<VkInstance>(vkInstance), debugMessenger, nullptr);
        }
#endif

        TracyVkDestroy(tracyCtx);
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
