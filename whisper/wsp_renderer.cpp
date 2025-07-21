#include <wsp_renderer.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_editor.hpp>
#include <wsp_engine.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_model.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_window.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include <vulkan/vulkan.hpp>

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

bool Renderer::ShouldClose()
{
    check(_window);

    return _window->ShouldClose();
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

void Renderer::Render() const
{
    check(_device);
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
    _editor->Render(commandBuffer, _graph, _window, _device);

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
            _window->UnbindResizeCallback(_graph);
            _graph->ChangeGoal(_device, Graph::eToDescriptorSet);
        }
        else
        {
            _window->BindResizeCallback(_graph, Graph::OnResizeCallback);
            _graph->ChangeGoal(_device, Graph::eToTransfer);
        }
    }
}

void Renderer::Initialize()
{
    ZoneScopedN("initialize");

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

    Resource color = _graph->NewResource(colorInfo);
    Resource depth = _graph->NewResource(depthInfo);

    Model *model = new Model(_device, "assets/bunny.obj");

    PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {color, depth};
    passCreateInfo.reads = {};
    passCreateInfo.readsUniform = true;
    passCreateInfo.vertexInputInfo = Model::Vertex::GetVertexInputInfo();
    passCreateInfo.vertFile = "model.vert.spv";
    passCreateInfo.fragFile = "model.frag.spv";
    passCreateInfo.debugName = "triangle render";
    passCreateInfo.execute = [model](vk::CommandBuffer commandBuffer) {
        model->Bind(commandBuffer);
        model->Draw(commandBuffer);
    };

    _graph->NewPass(passCreateInfo);
    _graph->Compile(_device, color, Graph::eToDescriptorSet);

    _resources.push_back({color});

    _editor = new Editor(_window, _device, _vkInstance);

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
