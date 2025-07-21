#include <wsp_editor.hpp>

#include <wsp_camera.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_editor_camera.hpp>
#include <wsp_engine.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_renderer.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_swapchain.hpp>
#include <wsp_window.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

using namespace wsp;

Editor::Editor(Window const *window, Device const *device, vk::Instance instance) : _freed{false}, _active{true}
{
    check(device);
    check(window);

    // _camera = new EditorCamera();
    _camera = new EditorCamera();
    // _camera->SetPerspectiveProjection(40.f, 1., 0.01f, 1000.f);
    // _camera->LookAt({-0.5f, 0.f, -0.5f}, {0.f, 0.f, 0.f});

    InitImGui(window, device, instance);
}

Editor::~Editor()
{
    if (!_freed)
    {
        spdlog::critical("Editor: forgot to Free before deletion");
    }
}

void Editor::Free(Device const *device)
{
    check(device);

    if (_freed)
    {
        spdlog::error("Editor: already freed window");
        return;
    }

    device->WaitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    device->DestroyDescriptorPool(_imguiDescriptorPool);

    ImGui::DestroyContext();

    _freed = true;
}

void Editor::Render(vk::CommandBuffer commandBuffer, Graph *graph, Window *window, Device const *device)
{
    check(window);
    check(graph);
    check(device);

    TracyVkZone(Renderer::GetTracyCtx(), commandBuffer, "editor");

    _deferredQueue.clear();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (_active)
    {
        RenderDockspace();
        RenderViewport(device, graph);
    }

    ImGui::Begin("Controls");
    if (ImGui::Checkbox("editor mode", &_active))
    {
        if (_active)
        {
            window->UnbindResizeCallback(_camera);
        }
        else
        {
            window->BindResizeCallback(_camera, EditorCamera::OnResizeCallback);
        }
    }
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, (void *)commandBuffer);
}

void Editor::Update(double dt)
{
    for (std::function<void()> const &func : _deferredQueue)
    {
        func();
    }

    _camera->OrbitYaw(1.f * dt);
}

bool Editor::IsActive() const
{
    return _active;
}

void Editor::PopulateUbo(GlobalUbo *ubo)
{
    ubo->camera.viewProjection = _camera->GetCamera()->GetProjection() * _camera->GetCamera()->GetView();
}

static int ImGui_CreateVkSurface(ImGuiViewport *viewport, ImU64 vk_instance, void const *vk_allocator,
                                 ImU64 *out_surface)
{
    return (int)glfwCreateWindowSurface(
        reinterpret_cast<VkInstance>(vk_instance), static_cast<GLFWwindow *>(viewport->PlatformHandle),
        reinterpret_cast<VkAllocationCallbacks const *>(vk_allocator), reinterpret_cast<VkSurfaceKHR *>(out_surface));
}

void Editor::InitImGui(Window const *window, Device const *device, vk::Instance instance)
{
    check(device);
    check(window);

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

    device->CreateDescriptorPool(poolInfo, &_imguiDescriptorPool, "imgui descriptor pool");

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
    initInfo.Instance = instance;
    initInfo.DescriptorPool = _imguiDescriptorPool.operator VkDescriptorPool();
    initInfo.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

    ImGui_ImplVulkan_Init(&initInfo);

    spdlog::info("EDITOR FILES at : {0}", EDITOR_FILES);
    io.Fonts->AddFontFromFileTTF((std::string(EDITOR_FILES) + std::string("JetBrainsMonoNL-Regular.ttf")).c_str(),
                                 16.0f);

    ApplyImGuiTheme();
}

void Editor::InitDockspace(unsigned int dockspaceID)
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspaceID);
    ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->Size);

    ImGuiID dock_mainID = dockspaceID;
    ImGuiID leftID = ImGui::DockBuilderSplitNode(dock_mainID, ImGuiDir_Left, 0.2f, nullptr, &dock_mainID);
    ImGuiID left_bottomID = ImGui::DockBuilderSplitNode(leftID, ImGuiDir_Down, 0.2f, nullptr, &leftID);
    ImGuiID rightID = ImGui::DockBuilderSplitNode(dock_mainID, ImGuiDir_Right, 0.2f, nullptr, &dock_mainID);
    ImGuiID bottomID = ImGui::DockBuilderSplitNode(dock_mainID, ImGuiDir_Down, 0.3f, nullptr, &dock_mainID);

    // ImGui::DockBuilderDockWindow("Graphics Manager", left_bottomID);
    // ImGui::DockBuilderDockWindow("World Manager", left_bottomID);
    // ImGui::DockBuilderDockWindow("Inspector", rightID);
    // ImGui::DockBuilderDockWindow("Logger", bottomID);
    ImGui::DockBuilderDockWindow("Viewport", dock_mainID);
    ImGui::DockBuilderDockWindow("Controls", leftID);

    ImGui::DockBuilderFinish(dockspaceID);
}

void Editor::RenderDockspace()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("GlobalDockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0, 0));

    static bool firstCall{true};
    if (firstCall)
    {
        InitDockspace(dockspaceID);
        firstCall = false;
    }

    ImGui::End();
}

void Editor::RenderViewport(Device const *device, Graph *graph)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar(3);

    ImVec2 const size = ImGui::GetContentRegionAvail();
    static ImVec2 oldSize = ImVec2(10, 10);
    ImGui::Image((ImTextureID)(graph->GetTargetDescriptorSet().operator VkDescriptorSet()), size);
    if (oldSize.x != size.x && oldSize.y != size.y)
    {
        _deferredQueue.push_back([=]() {
            check(graph);
            graph->Resize(device, (size_t)size.x, (size_t)size.y);
            check(_camera);
            _camera->SetAspectRatio((float)size.x / (float)size.y);
        });
        oldSize = size;
    }

    ImGui::End();
}

void Editor::ApplyImGuiTheme()
{
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
}
