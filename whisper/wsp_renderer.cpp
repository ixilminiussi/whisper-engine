#include "wsp_drawable.hpp"
#include <wsp_renderer.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_editor.hpp>
#include <wsp_engine.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_mesh.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_window.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include <vulkan/vulkan.hpp>

#include <stdexcept>
#include <unordered_set>

using namespace wsp;

vk::Instance Renderer::_vkInstance{};
TracyVkCtx Renderer::tracyCtx{};

TracyVkCtx Renderer::GetTracyCtx()
{
    return tracyCtx;
}

bool Renderer::ShouldClose()
{
    check(_window);

    return _window->ShouldClose();
}

Renderer::Renderer()
    : _freed{false}, _validationLayers{"VK_LAYER_KHRONOS_validation"},
      _deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName}
{
    ZoneScopedN("initialize");
    spdlog::info("Engine: began initialization");

    if (!glfwInit())
    {
        throw std::runtime_error("Engine: GLFW failed to initialize!");
    }

    CreateInstance();

    if (_vkInstance)
    {
        spdlog::error("Engine: already initialized");
    }

    _window = std::make_unique<Window>(_vkInstance, 1024, 1024, "test");

    Device *device = DeviceAccessor::Get();
    check(device);

    device->Initialize(_deviceExtensions, _vkInstance, _window->GetSurface());
    device->CreateTracyContext(&tracyCtx);

    _window->Initialize(device);
}

Renderer::~Renderer()
{
    if (!_freed)
    {
        spdlog::critical("Window: forgot to Free before deletion");
    }
}

void Renderer::Free()
{
    Device *device = DeviceAccessor::Get();
    check(device);

    device->WaitIdle();

    spdlog::info("Renderer: began termination");

    _editor->Free(device);
    _editor.release();

    _graph->Free(device);
    _graph.release();

    _window->Free(_vkInstance);
    _window.release();

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(_vkInstance),
                                                                           "vkDestroyDebugUtilsMessengerEXT");
#ifndef NDEBUG
    if (func != nullptr)
    {
        func(static_cast<VkInstance>(_vkInstance), debugMessenger, nullptr);
    }
#endif

    TracyVkDestroy(tracyCtx);
    device->Free();

    _vkInstance.destroy();

    glfwTerminate();

    spdlog::info("Renderer: terminated");
    _freed = true;
}

void Renderer::Render() const
{
    Device *device = DeviceAccessor::Get();

    check(device);
    check(_window);
    check(_graph);
    check(_editor);

    FrameMarkStart("frame render");

    glfwPollEvents();

    size_t frameIndex = 0;
    vk::CommandBuffer const commandBuffer = _window->NextCommandBuffer(&frameIndex);

    GlobalUbo ubo{};

    if (_editor->IsActive())
    {
        _editor->PopulateUbo(&ubo);
    }

    _graph->FlushUbo(&ubo, frameIndex, device);

    _graph->Render(commandBuffer);

    if (_editor->IsActive())
    {
        _window->SwapchainOpen(commandBuffer);
    }
    else
    {
        _window->SwapchainOpen(commandBuffer, _graph->GetTargetImage());
    }
    _editor->Render(commandBuffer, _graph.get(), _window.get(), device);

    _window->SwapchainFlush(commandBuffer);
    FrameMarkEnd("frame render");
}

void Renderer::Update(double dt)
{
    _editor->Update(dt);

    static bool WasActive = !_editor->IsActive();
    if (WasActive != _editor->IsActive())
    {
        WasActive = _editor->IsActive();

        if (_editor->IsActive())
        {
            _window->UnbindResizeCallback(_graph.get());
            _graph->ChangeUsage(DeviceAccessor::Get(), Graph::eToDescriptorSet);
        }
        else
        {
            _window->BindResizeCallback(_graph.get(), Graph::OnResizeCallback);
            _graph->ChangeUsage(DeviceAccessor::Get(), Graph::eToTransfer);
        }
    }
}

void Renderer::Initialize(std::vector<Drawable const *> const *drawList)
{
    ZoneScopedN("initialize (graph generate + compile)");

    Device *device = DeviceAccessor::Get();
    check(device);

    _graph = std::make_unique<Graph>(device, _window->GetExtent().width, _window->GetExtent().height);
    _graph->SetUboSize(sizeof(GlobalUbo));

    ResourceCreateInfo colorInfo{};
    colorInfo.role = ResourceRole::eColor;
    colorInfo.format = vk::Format::eR8G8B8A8Unorm;
    colorInfo.clear.color = vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f};
    colorInfo.debugName = "color";

    ResourceCreateInfo depthInfo{};
    depthInfo.role = ResourceRole::eDepth;
    depthInfo.format = vk::Format::eD16Unorm;
    depthInfo.clear.depthStencil = vk::ClearDepthStencilValue{1.};
    depthInfo.debugName = "depth";

    Resource const color = _graph->NewResource(colorInfo);
    Resource const depth = _graph->NewResource(depthInfo);

    PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {color, depth};
    passCreateInfo.reads = {};
    passCreateInfo.readsUniform = true;
    passCreateInfo.vertexInputInfo = Mesh::Vertex::GetVertexInputInfo();
    passCreateInfo.vertFile = "mesh.vert.spv";
    passCreateInfo.fragFile = "mesh.frag.spv";
    passCreateInfo.debugName = "mesh render";
    passCreateInfo.execute = [drawList](vk::CommandBuffer commandBuffer) {
        check(drawList);

        for (Drawable const *drawable : *drawList)
        {
            if (ensure(drawable))
            {
                drawable->BindAndDraw(commandBuffer);
            }
        }
    };

    _graph->NewPass(passCreateInfo);
    _graph->Compile(device, color, Graph::eToDescriptorSet);

    _resources.push_back({color});

    _editor = std::make_unique<Editor>(_window.get(), device, _vkInstance);

    spdlog::info("Engine: new window initialized");
}

void Renderer::CreateInstance()
{
    ZoneScopedN("create vulkan instance");

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

    std::vector<char const *> extensions = GetRequiredExtensions();

    for (char const *ext : extensions)
    {
        spdlog::info("Engine: Requested extension: {}", ext);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
    createInfo.ppEnabledLayerNames = _validationLayers.data();

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

    if (const vk::Result result = vk::createInstance(&createInfo, nullptr, &_vkInstance);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            fmt::format("Engine: failed to create _vkInstance : {}", vk::to_string(static_cast<vk::Result>(result))));
    }

#ifndef NDEBUG
    if (const vk::Result result = CreateDebugUtilsMessengerEXT(_vkInstance, &debugCreateInfo, nullptr, &debugMessenger);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("Engine: failed to set up debug messenger : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }
#endif

    ExtensionsCompatibilityTest();
}

void Renderer::ExtensionsCompatibilityTest()
{
    std::vector<vk::ExtensionProperties> const extensions = vk::enumerateInstanceExtensionProperties();

    spdlog::info("available extensions:");
    std::unordered_set<std::string> available;
    for (vk::ExtensionProperties const &extension : extensions)
    {
        spdlog::info("\t{0}", extension.extensionName.data());
        available.insert(extension.extensionName);
    }

    spdlog::info("required extensions:");
    std::vector<char const *> requiredExtensions = GetRequiredExtensions();
    for (char const *&required : requiredExtensions)
    {
        spdlog::info("\t{0}", required);
        if (available.find(required) == available.end())
        {
            throw std::runtime_error("Engine: missing required glfw extension");
        }
    }
}

std::vector<char const *> Renderer::GetRequiredExtensions()
{
    uint32_t extensionCount = 0;
    char const **extensions;
    extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<char const *> requiredExtensions(extensions, extensions + extensionCount);

#ifndef NDEBUG
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return requiredExtensions;
}

#ifndef NDEBUG
bool Renderer::CheckValidationLayerSupport()
{
    std::vector<vk::LayerProperties> const availableLayers = vk::enumerateInstanceLayerProperties();

    for (char const *layerName : _validationLayers)
    {
        bool layerFound = false;

        for (auto const &layerProperties : availableLayers)
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
