#include "wsp_renderer.hpp"
#include "wsp_camera.hpp"
#include "wsp_device.hpp"
#include "wsp_devkit.hpp"
#include "wsp_editor.hpp"
#include "wsp_engine.hpp"
#include "wsp_global_ubo.hpp"
#include "wsp_graph.hpp"
#include "wsp_static_utils.hpp"
#include "wsp_window.hpp"

// lib
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

// std
#include <stdexcept>
#include <unordered_set>

using namespace wsp;

Device *Renderer::_device{nullptr};
vk::Instance Renderer::_vkInstance{};
TracyVkCtx Renderer::tracyCtx{};

TracyVkCtx Renderer::GetTracyCtx()
{
    return tracyCtx;
}

Renderer::Renderer()
    : _freed{false}, _validationLayers{"VK_LAYER_KHRONOS_validation"},
      _deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName}
{
    spdlog::info("Engine: began initialization");

    if (!glfwInit())
    {
        throw std::runtime_error("Engine: GLFW failed to initialize!");
    }

    CreateInstance();
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
    check(_device);

    _device->WaitIdle();

    spdlog::info("Renderer: began termination");

    _editor->Free(_device);
    delete _editor;

    _graph->Free(_device);
    delete _graph;

    _window->Free(_vkInstance);
    delete _window;

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(_vkInstance),
                                                                           "vkDestroyDebugUtilsMessengerEXT");
#ifndef NDEBUG
    if (func != nullptr)
    {
        func(static_cast<VkInstance>(_vkInstance), debugMessenger, nullptr);
    }
#endif

    TracyVkDestroy(tracyCtx);
    _device->Free();
    delete _device;

    _vkInstance.destroy();

    glfwTerminate();

    spdlog::info("Renderer: terminated");
    _freed = true;
}

void Renderer::Run()
{
    check(_device);

    while (!_window->ShouldClose())
    {
        check(_window);
        check(_graph);
        check(_editor);

        FrameMarkStart("frame");

        glfwPollEvents();

        size_t frameIndex = 0;
        vk::CommandBuffer const commandBuffer = _window->NextCommandBuffer(&frameIndex);

        GlobalUbo ubo{};
        ubo.camera.view = _camera->GetView();
        ubo.camera.projection = _camera->GetProjection();
        ubo.camera.position = _camera->GetPosition();

        _graph->FlushUbo(&ubo, frameIndex, _device);

        _graph->Render(commandBuffer);

        if (_editor->IsActive())
        {
            _window->SwapchainOpen(commandBuffer);
        }
        else
        {
            _window->SwapchainOpen(commandBuffer, _graph->GetTargetImage());
        }
        _editor->Render(commandBuffer, _camera, _graph, _device);

        _window->SwapchainFlush(commandBuffer);
        _editor->Update(0.f);

        static bool WasActive = !_editor->IsActive();
        if (WasActive != _editor->IsActive())
        {
            WasActive = _editor->IsActive();

            if (_editor->IsActive())
            {
                _window->UnbindResizeCallback(_graph);
                _window->UnbindResizeCallback(_camera);
                _graph->ChangeGoal(_device, Graph::eToDescriptorSet);
            }
            else
            {
                _window->BindResizeCallback(_graph, Graph::OnResizeCallback);
                _window->BindResizeCallback(_camera, Camera::OnResizeCallback);
                _graph->ChangeGoal(_device, Graph::eToTransfer);
            }
        }

        FrameMarkEnd("frame");
    }
}

void Renderer::Initialize()
{
    if (_device)
    {
        spdlog::error("Engine: already initialized");
    }

    _window = new Window(_vkInstance, 1024, 1024, "test");

    _device = new Device();
    _device->Initialize(_deviceExtensions, _vkInstance, _window->GetSurface());
    _device->CreateTracyContext(&tracyCtx);

    _window->Initialize(_device);

    check(_device);

    _graph = new Graph(_device, _window->GetExtent().width, _window->GetExtent().height);
    _graph->SetUboSize(sizeof(GlobalUbo));

    ResourceCreateInfo resourceInfo{};
    resourceInfo.role = ResourceRole::eColor;
    resourceInfo.format = vk::Format::eR8G8B8A8Unorm;
    resourceInfo.clear.color = vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f};
    resourceInfo.debugName = "final";

    Resource final = _graph->NewResource(resourceInfo);

    PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {final};
    passCreateInfo.reads = {};
    passCreateInfo.readsUniform = true;
    passCreateInfo.vertFile = "triangle.vert.spv";
    passCreateInfo.fragFile = "triangle.frag.spv";
    passCreateInfo.debugName = "triangle render";
    passCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(3, 1, 0, 0); };

    _graph->NewPass(passCreateInfo);
    _graph->Compile(_device, final, Graph::eToDescriptorSet);

    _resources.push_back({final});

    _editor = new Editor(_window, _device, _vkInstance);

    _camera = new Camera();
    _camera->SetPerspectiveProjection(50.f, 1., 0.01f, 1000.f);

    spdlog::info("Engine: new window initialized");
}

void Renderer::CreateInstance()
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
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Engine: failed to create _vkInstance!");
    }

#ifndef NDEBUG
    if (const vk::Result result = CreateDebugUtilsMessengerEXT(_vkInstance, &debugCreateInfo, nullptr, &debugMessenger);
        result != vk::Result::eSuccess)
    {
        spdlog::critical("Error: {}", vk::to_string(static_cast<vk::Result>(result)));
        throw std::runtime_error("Engine: failed to set up debug messenger.");
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
