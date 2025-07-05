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
      _deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName}, _graphs{}, _windows{},
      _editors{}
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

    while (_windows.size() > 0)
    {
        FreeWindow(_device, 0);
    }

    _editors.clear();
    _graphs.clear();
    _windows.clear();

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

    while (_windows.size() > 0)
    {
        for (size_t ID = 0; ID < _windows.size();)
        {
            Window *window = _windows[ID];
            Graph *graph = _graphs[ID];
            Editor *editor = _editors[ID];
            Camera *camera = _camera[ID];

            check(window);
            check(graph);
            check(editor);

            if (window->ShouldClose())
            {
                FreeWindow(_device, ID);
                continue;
            }

            std::string frameName = "frame" + std::to_string(ID);
            FrameMarkStart(frameName.c_str());

            glfwPollEvents();

            size_t frameIndex = 0;
            vk::CommandBuffer const commandBuffer = window->NextCommandBuffer(&frameIndex);

            GlobalUbo ubo{};
            ubo.camera.view = camera->GetView();
            ubo.camera.projection = camera->GetProjection();
            ubo.camera.position = camera->GetPosition();

            graph->FlushUbo(&ubo, frameIndex, _device);

            graph->Render(commandBuffer);

            if (editor->IsActive())
            {
                window->SwapchainOpen(commandBuffer);
            }
            else
            {
                window->SwapchainOpen(commandBuffer, graph->GetTargetImage());
            }
            editor->Render(commandBuffer, camera, graph, _device);

            window->SwapchainFlush(commandBuffer);
            editor->Update(0.f);

            static bool WasActive = !editor->IsActive();
            if (WasActive != editor->IsActive())
            {
                WasActive = editor->IsActive();

                if (editor->IsActive())
                {
                    window->UnbindResizeCallback(graph);
                    window->UnbindResizeCallback(camera);
                    graph->ChangeGoal(_device, Graph::eToDescriptorSet);
                }
                else
                {
                    window->BindResizeCallback(graph, Graph::OnResizeCallback);
                    window->BindResizeCallback(camera, Camera::OnResizeCallback);
                    graph->ChangeGoal(_device, Graph::eToTransfer);
                }
            }

            FrameMarkEnd(frameName.c_str());
            ID++;
        }
    }
}

size_t Renderer::NewWindow()
{
    _windows.emplace_back(new Window(_vkInstance, 1024, 1024, "test"));
    size_t ID = _windows.size() - 1;

    if (!_device)
    {
        _device = new Device();
        _device->Initialize(_deviceExtensions, _vkInstance, _windows[ID]->GetSurface());
        _device->CreateTracyContext(&tracyCtx);
    }

    _windows[ID]->Initialize(_device);

    check(_device);

    _graphs.emplace_back(new Graph(_device, _windows[ID]->GetExtent().width, _windows[ID]->GetExtent().height));
    _graphs[ID]->SetUboSize(sizeof(GlobalUbo));

    ResourceCreateInfo resourceInfo{};
    resourceInfo.role = ResourceRole::eColor;
    resourceInfo.format = vk::Format::eR8G8B8A8Unorm;
    resourceInfo.clear.color = vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f};
    resourceInfo.debugName = "final";

    Resource final = _graphs[ID]->NewResource(resourceInfo);

    PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {final};
    passCreateInfo.reads = {};
    passCreateInfo.readsUniform = true;
    passCreateInfo.vertFile = "triangle.vert.spv";
    passCreateInfo.fragFile = "triangle.frag.spv";
    passCreateInfo.debugName = "triangle render";
    passCreateInfo.execute = [](vk::CommandBuffer commandBuffer) { commandBuffer.draw(3, 1, 0, 0); };

    _graphs[ID]->NewPass(passCreateInfo);
    _graphs[ID]->Compile(_device, final, Graph::eToTransfer);

    _resources.push_back({final});

    _editors.emplace_back(new Editor(_windows[ID], _device, _vkInstance));

    Camera *camera = new Camera();
    camera->SetPerspectiveProjection(50.f, 1., 0.01f, 1000.f);
    _camera.emplace_back(camera);

    spdlog::info("Engine: new window initialized");

    return ID;
}

void Renderer::FreeWindow(Device const *device, size_t ID)
{
    check(ID < _windows.size());

    _editors[ID]->Free(device);
    delete _editors[ID];
    _editors.erase(_editors.begin() + ID);

    _graphs[ID]->Free(device);
    delete _graphs[ID];
    _graphs.erase(_graphs.begin() + ID);

    _windows[ID]->Free(_vkInstance);
    delete _windows[ID];
    _windows.erase(_windows.begin() + ID);
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
