#include <wsp_render_manager.hpp>

#include <wsp_devkit.hpp>

#include <wsp_device.hpp>
#include <wsp_graph.hpp>
#include <wsp_renderer.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_swapchain.hpp>
#include <wsp_window.hpp>

#ifndef NDEBUG
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#include <IconsMaterialSymbols.h>
#endif

#include <tracy/TracyVulkan.hpp>

#include <unordered_set>

using namespace wsp;

RenderManager *RenderManager::_instance{nullptr};

RenderManager *RenderManager::Get()
{
    if (!_instance)
    {
        _instance = new RenderManager{};
    }

    return _instance;
}

vk::Instance RenderManager::_vkInstance{};

namespace wsp
{
TracyVkCtx TRACY_CTX;
}

RenderManager::RenderManager()
    : _validationLayers{"VK_LAYER_KHRONOS_validation"},
      _deviceExtensions{vk::KHRSwapchainExtensionName, vk::KHRMaintenance2ExtensionName}

{
    ZoneScopedN("initialize");
    spdlog::info("RenderManager: began initialization");

    if (!glfwInit())
    {
        throw std::runtime_error("RenderManager: GLFW failed to initialize!");
    }

    if (_vkInstance)
    {
        spdlog::error("RenderManager: already initialized");
        return;
    }

    CreateInstance();

    Device *device = DeviceAccessor::Get();
    check(device);

    Window *window = new Window(_vkInstance, 10, 10, "temp"); // temp window for device init

    device->Initialize(_deviceExtensions, _vkInstance, window->GetSurface());
    device->CreateTracyContext(&TRACY_CTX);

    window->Free(_vkInstance);
    delete window;
}

RenderManager::~RenderManager()
{
    if (!_freed)
    {
        spdlog::critical("RenderManager: forgot to Free before deletion");
    }
}

void RenderManager::Free()
{
    spdlog::info("RenderManager: began termination");

    if (_freed)
    {
        spdlog::error("RenderManager: already freed renderer");
        return;
    }

    Device *device = DeviceAccessor::Get();
    check(device);

    device->WaitIdle();

    for (auto &[windowID, pair] : _windowRenderers)
    {
        if (_imguiDescriptorPools.find(windowID) != _imguiDescriptorPools.end())
        {
            device->DestroyDescriptorPool(_imguiDescriptorPools.at(windowID));
        }

        pair.window->Free(_vkInstance);
        pair.window.release();
        pair.renderer->Free();
        pair.renderer.release();
    }
    _windowRenderers.clear();

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(_vkInstance),
                                                                           "vkDestroyDebugUtilsMessengerEXT");
#ifndef NDEBUG
    if (func != nullptr)
    {
        func(static_cast<VkInstance>(_vkInstance), debugMessenger, nullptr);
    }
#endif

    TracyVkDestroy(TRACY_CTX);
    device->Free();

    _vkInstance.destroy();

    spdlog::info("RenderManager: terminated");
    _freed = true;
}

void RenderManager::CreateInstance()
{
    ZoneScopedN("create vulkan instance");

#ifndef NDEBUG
    if (!CheckValidationLayerSupport())
    {
        throw std::runtime_error("RenderManager: validation layers requested, but not available!");
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
        spdlog::info("RenderManager: Requested extension: {}", ext);
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
        throw std::runtime_error(fmt::format("RenderManager: failed to create _vkInstance : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }

#ifndef NDEBUG
    if (const vk::Result result = CreateDebugUtilsMessengerEXT(_vkInstance, &debugCreateInfo, nullptr, &debugMessenger);
        result != vk::Result::eSuccess)
    {
        throw std::runtime_error(fmt::format("RenderManager: failed to set up debug messenger : {}",
                                             vk::to_string(static_cast<vk::Result>(result))));
    }
#endif

    ExtensionsCompatibilityTest();
}

WindowID RenderManager::NewWindowRenderer()
{
    static WindowID IDProvider{0};
    IDProvider++;

    WindowRenderer &pair = _windowRenderers[IDProvider];
    pair.window = std::make_unique<Window>(_vkInstance, 1024, 1024, "test");
    pair.window->Initialize();

    vk::Extent2D const extent = pair.window->GetExtent();
    pair.renderer = std::make_unique<Renderer>();
    pair.renderer->Initialize(extent.width, extent.height);

    return IDProvider;
}

bool RenderManager::ShouldClose(WindowID windowID)
{
    check(_windowRenderers.find(windowID) != _windowRenderers.end());

    return _windowRenderers.at(windowID).window->ShouldClose();
}

void RenderManager::CloseWindow(WindowID windowID)
{
    check(_windowRenderers.find(windowID) != _windowRenderers.end());

    Device *device = DeviceAccessor::Get();
    check(device);
    if (_imguiDescriptorPools.find(windowID) != _imguiDescriptorPools.end())
    {
        device->DestroyDescriptorPool(_imguiDescriptorPools.at(windowID));
    }

    _windowRenderers.at(windowID).window->Free(_vkInstance);
    _windowRenderers.at(windowID).renderer->Free();

    _windowRenderers.erase(windowID);
}

void RenderManager::BindResizeCallback(WindowID windowID, void *pointer, void (*function)(void *, size_t, size_t))
{
    if (ensure(_windowRenderers.find(windowID) != _windowRenderers.end()))
    {
        _windowRenderers.at(windowID).window->BindResizeCallback(pointer, function);
    }
}

class Graph *RenderManager::GetGraph(WindowID windowID) const
{
    check(_windowRenderers.find(windowID) != _windowRenderers.end());

    return _windowRenderers.at(windowID).renderer->GetGraph();
}

vk::CommandBuffer RenderManager::BeginRender(WindowID windowID, bool blit)
{
    check(_windowRenderers.find(windowID) != _windowRenderers.end());

    WindowRenderer &windowRenderer = _windowRenderers.at(windowID);

    size_t frameIndex{0};
    vk::CommandBuffer const commandBuffer = windowRenderer.window->NextCommandBuffer(&frameIndex);

    TracyVkZone(TRACY_CTX, commandBuffer, "frame");

    windowRenderer.renderer->Render(commandBuffer, frameIndex);

    if (blit)
    {
        windowRenderer.window->SwapchainOpen(commandBuffer, windowRenderer.renderer->GetGraph()->GetTargetImage());
    }
    else
    {
        windowRenderer.window->SwapchainOpen(commandBuffer);
    }

    return commandBuffer;
}

void RenderManager::EndRender(vk::CommandBuffer commandBuffer, WindowID windowID)
{
    check(_windowRenderers.find(windowID) != _windowRenderers.end());

    WindowRenderer &windowRenderer = _windowRenderers.at(windowID);
    windowRenderer.window->SwapchainFlush(commandBuffer);
}

static int ImGui_CreateVkSurface(ImGuiViewport *viewport, ImU64 vk_instance, void const *vk_allocator,
                                 ImU64 *out_surface)
{
#ifndef NDEBUG
    return (int)glfwCreateWindowSurface(
        reinterpret_cast<VkInstance>(vk_instance), static_cast<GLFWwindow *>(viewport->PlatformHandle),
        reinterpret_cast<VkAllocationCallbacks const *>(vk_allocator), reinterpret_cast<VkSurfaceKHR *>(out_surface));
#else
    return -1;
#endif
}

static void ApplyImGuiTheme()
{
#ifndef NDEBUG
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;
    style.Alpha = 1.0f;

    style.FrameRounding = 2;

    ImVec4 const dracula_background{0.157f, 0.165f, 0.212f, 1.f};
    ImVec4 const dracula_currentLine{0.267f, 0.278f, 0.353f, 1.f};
    ImVec4 const dracula_foreground{0.973f, 0.973f, 0.949f, 1.f};
    ImVec4 const dracula_comment{0.384f, 0.447f, 0.643f, 1.f};
    ImVec4 const dracula_cyan{0.545f, 0.914f, 0.992f, 1.f};
    ImVec4 const dracula_green{0.314f, 0.98f, 0.482f, 1.f};
    ImVec4 const dracula_orange{1.f, 0.722f, 0.424f, 1.f};
    ImVec4 const dracula_pink{1.f, 0.475f, 0.776f, 1.f};
    ImVec4 const dracula_purple{0.741f, 0.576f, 0.976f, 1.f};
    ImVec4 const dracula_red{1.f, 0.333f, 0.333f, 1.f};
    ImVec4 const dracula_yellow{0.945f, 0.98f, 0.549f, 1.f};
    ImVec4 const transparent{0.f, 0.f, 0.f, 0.f};
    ImVec4 const dracula_comment_highlight{0.53f, 0.58f, 0.74f, 1.f};
    colors[ImGuiCol_Text] = dracula_foreground;
    colors[ImGuiCol_TextDisabled] = dracula_comment;
    colors[ImGuiCol_WindowBg] = dracula_background;
    colors[ImGuiCol_ChildBg] = transparent;
    colors[ImGuiCol_PopupBg] = dracula_background;
    colors[ImGuiCol_Border] = dracula_purple;
    colors[ImGuiCol_BorderShadow] = transparent;
    colors[ImGuiCol_FrameBg] = dracula_currentLine;
    colors[ImGuiCol_FrameBgHovered] = dracula_comment;
    colors[ImGuiCol_FrameBgActive] = dracula_comment_highlight;
    colors[ImGuiCol_TitleBg] = dracula_comment;
    colors[ImGuiCol_TitleBgActive] = dracula_purple;
    colors[ImGuiCol_TitleBgCollapsed] = dracula_comment;
    colors[ImGuiCol_MenuBarBg] = dracula_currentLine;
    colors[ImGuiCol_ScrollbarBg] = {0.02f, 0.02f, 0.02f, 0.5f};
    colors[ImGuiCol_ScrollbarGrab] = dracula_currentLine;
    colors[ImGuiCol_ScrollbarGrabActive] = dracula_comment;
    colors[ImGuiCol_ScrollbarGrabHovered] = dracula_comment;
    colors[ImGuiCol_CheckMark] = dracula_green;
    colors[ImGuiCol_SliderGrab] = dracula_pink;
    colors[ImGuiCol_SliderGrabActive] = dracula_pink;
    colors[ImGuiCol_Button] = dracula_comment;
    colors[ImGuiCol_ButtonActive] = dracula_comment_highlight;
    colors[ImGuiCol_ButtonHovered] = dracula_comment_highlight;
    colors[ImGuiCol_Header] = dracula_comment;
    colors[ImGuiCol_HeaderActive] = dracula_comment_highlight;
    colors[ImGuiCol_HeaderHovered] = dracula_comment_highlight;
    colors[ImGuiCol_Separator] = dracula_purple;
    colors[ImGuiCol_SeparatorActive] = dracula_purple;
    colors[ImGuiCol_SeparatorHovered] = dracula_purple;
    colors[ImGuiCol_ResizeGrip] = dracula_comment;
    colors[ImGuiCol_ResizeGripActive] = dracula_comment_highlight;
    colors[ImGuiCol_ResizeGripHovered] = dracula_comment_highlight;
    colors[ImGuiCol_Tab] = dracula_comment;
    colors[ImGuiCol_TabHovered] = dracula_comment_highlight;
    colors[ImGuiCol_TabSelected] = dracula_pink;
    colors[ImGuiCol_TabSelectedOverline] = dracula_pink;
    colors[ImGuiCol_TabDimmed] = dracula_background;
    colors[ImGuiCol_TabDimmedSelected] = dracula_currentLine;
    colors[ImGuiCol_TabDimmedSelectedOverline] = transparent;
    colors[ImGuiCol_PlotLines] = dracula_cyan;
    colors[ImGuiCol_PlotLinesHovered] = dracula_red;
    colors[ImGuiCol_PlotHistogram] = dracula_yellow;
    colors[ImGuiCol_PlotHistogramHovered] = dracula_orange;
    colors[ImGuiCol_TableBorderLight] = transparent;
    colors[ImGuiCol_TableBorderStrong] = dracula_red;
    colors[ImGuiCol_TableHeaderBg] = dracula_red;
    colors[ImGuiCol_TableRowBg] = transparent;
    colors[ImGuiCol_TableRowBgAlt] = {1.f, 1.f, 1.f, 0.06f};
    colors[ImGuiCol_TextLink] = dracula_yellow;
    colors[ImGuiCol_TextSelectedBg] = dracula_comment_highlight;
    colors[ImGuiCol_DragDropTarget] = dracula_yellow;
    colors[ImGuiCol_NavCursor] = dracula_cyan;
    colors[ImGuiCol_DockingEmptyBg] = dracula_background;
    colors[ImGuiCol_DockingPreview] = dracula_cyan;
    colors[ImGuiCol_ModalWindowDimBg] = transparent;
    colors[ImGuiCol_TabActive] = dracula_pink;
    colors[ImGuiCol_TabUnfocused] = dracula_comment_highlight;
    colors[ImGuiCol_TabUnfocusedActive] = dracula_comment_highlight;

    colors[ImGuiCol_NavHighlight] = transparent;
    colors[ImGuiCol_NavWindowingDimBg] = transparent;
    colors[ImGuiCol_NavWindowingHighlight] = transparent;

    for (int i = 0; i < ImGuiCol_COUNT; ++i)
        style.Colors[i] = decodeSRGB(style.Colors[i]);
#endif
}

void RenderManager::InitImGui(WindowID windowID)
{
#ifndef NDEBUG
    Device *device = DeviceAccessor::Get();
    check(device);

    check(_windowRenderers.find(windowID) != _windowRenderers.end());
    Window *window = _windowRenderers.at(windowID).window.get();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window->GetGLFWHandle(), true);

    std::vector<vk::DescriptorPoolSize> poolSizes = {{vk::DescriptorType::eSampler, 1000},
                                                     {vk::DescriptorType::eCombinedImageSampler, 1000},
                                                     {vk::DescriptorType::eSampledImage, 1000},
                                                     {vk::DescriptorType::eStorageImage, 1000},
                                                     {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                                     {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                                     {vk::DescriptorType::eUniformBuffer, 1000},
                                                     {vk::DescriptorType::eStorageBuffer, 1000},
                                                     {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                                     {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                                     {vk::DescriptorType::eInputAttachment, 1000}};

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = 1000 * poolSizes.size();
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    device->CreateDescriptorPool(poolInfo, &_imguiDescriptorPools[windowID], "imgui descriptor pool");

    vk::Format const format = vk::Format::eB8G8R8A8Unorm;
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateVkSurface = &ImGui_CreateVkSurface;

    ImGui_ImplVulkan_InitInfo initInfo = {};
#ifndef NDEBUG
    device->PopulateImGuiInitInfo(&initInfo);
    window->GetSwapchain()->PopulateImGuiInitInfo(&initInfo);
#endif
    initInfo.Instance = _vkInstance;
    initInfo.DescriptorPool = _imguiDescriptorPools[windowID].operator VkDescriptorPool();
    initInfo.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

    ImGui_ImplVulkan_Init(&initInfo);

    spdlog::info("EDITOR FILES at : {0}", WSP_EDITOR_ASSETS);
    float const fontSize = 16.0f;
    io.Fonts->AddFontFromFileTTF((std::string(WSP_EDITOR_ASSETS) + std::string("JetBrainsMonoNL-Regular.ttf")).c_str(),
                                 fontSize);

    float const iconFontSize = 16.0f;
    static ImWchar const icons_ranges[] = {ICON_MIN_MS, ICON_MAX_16_MS, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphOffset = {0.0f, 4.0f};
    io.Fonts->AddFontFromFileTTF((std::string(WSP_EDITOR_ASSETS) + std::string("MaterialIcons-Regular.ttf")).c_str(),
                                 iconFontSize, &icons_config, icons_ranges);

    ApplyImGuiTheme();
#endif
}

void RenderManager::ExtensionsCompatibilityTest()
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
            throw std::runtime_error("RenderManager: missing required glfw extension");
        }
    }
}

std::vector<char const *> RenderManager::GetRequiredExtensions()
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

bool RenderManager::CheckValidationLayerSupport()
{
#ifndef NDEBUG
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
#endif

    return true;
}
