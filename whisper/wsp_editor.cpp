#include "wsp_devkit.h"
#include "wsp_static_utils.h"
#include <vulkan/vulkan_handles.hpp>
#ifndef NDEBUG
#include "wsp_editor.h"

// wsp
#include "wsp_device.h"
#include "wsp_window.h"

// lib
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

namespace wsp
{

bool Editor::_active{true};

bool Editor::IsActive()
{
    return _active;
}

Editor::Editor(const Window *window, const Device *device, vk::Instance instance) : _freed{false}
{
    check(device);
    check(window);

    InitImGui(window, device, instance);
}

Editor::~Editor()
{
    if (!_freed)
    {
        spdlog::critical("Editor: forgot to Free before deletion");
    }
}

void Editor::Free(const Device *device)
{
    check(device);

    if (_freed)
    {
        spdlog::error("Editor: already freed window");
        return;
    }

    device->WaitIdle();

    ImGui_ImplVulkan_DestroyFontsTexture();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    device->DestroyDescriptorPool(_imguiDescriptorPool);

    ImGui::DestroyContext();

    _freed = true;
}

void Editor::Render(vk::CommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("test");
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Editor::InitImGui(const Window *window, const Device *device, vk::Instance instance)
{
    check(device);
    check(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    const ImGuiIO &io = ImGui::GetIO();
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

    device->CreateDescriptorPool(poolInfo, &_imguiDescriptorPool);

    const vk::Format format = vk::Format::eB8G8R8A8Unorm;
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    ImGui_ImplVulkan_InitInfo initInfo = {};
    device->PopulateImGuiInitInfo(&initInfo);
    window->GetSwapchain()->PopulateImGuiInitInfo(&initInfo);
    initInfo.Instance = instance;
    initInfo.DescriptorPool = _imguiDescriptorPool.operator VkDescriptorPool();
    initInfo.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

    ImGui_ImplVulkan_Init(&initInfo);

    spdlog::info("EDITOR FILES at : {0}", EDITOR_FILES);
    io.Fonts->AddFontFromFileTTF((std::string(EDITOR_FILES) + std::string("JetBrainsMonoNL-Regular.ttf")).c_str(),
                                 16.0f);

    ApplyImGuiTheme();
}

void Editor::ApplyImGuiTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;
    style.Alpha = 1.0f;

    style.FrameRounding = 2;

    const ImVec4 dracula_background{0.157f, 0.165f, 0.212f, 1.f};
    const ImVec4 dracula_currentLine{0.267f, 0.278f, 0.353f, 1.f};
    const ImVec4 dracula_foreground{0.973f, 0.973f, 0.949f, 1.f};
    const ImVec4 dracula_comment{0.384f, 0.447f, 0.643f, 1.f};
    const ImVec4 dracula_cyan{0.545f, 0.914f, 0.992f, 1.f};
    const ImVec4 dracula_green{0.314f, 0.98f, 0.482f, 1.f};
    const ImVec4 dracula_orange{1.f, 0.722f, 0.424f, 1.f};
    const ImVec4 dracula_pink{1.f, 0.475f, 0.776f, 1.f};
    const ImVec4 dracula_purple{0.741f, 0.576f, 0.976f, 1.f};
    const ImVec4 dracula_red{1.f, 0.333f, 0.333f, 1.f};
    const ImVec4 dracula_yellow{0.945f, 0.98f, 0.549f, 1.f};
    const ImVec4 transparent{0.f, 0.f, 0.f, 0.f};
    const ImVec4 dracula_comment_highlight{0.53f, 0.58f, 0.74f, 1.f};
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

} // namespace wsp
#endif
