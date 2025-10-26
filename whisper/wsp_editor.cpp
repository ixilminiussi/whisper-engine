#ifndef NDEBUG
#include <wsp_editor.hpp>

#include <wsp_devkit.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_camera.hpp>
#include <wsp_device.hpp>
#include <wsp_engine.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_input_manager.hpp>
#include <wsp_mesh.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_renderer.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_swapchain.hpp>
#include <wsp_viewport_camera.hpp>
#include <wsp_window.hpp>

#include <frost.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#include <IconsMaterialSymbols.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

using namespace wsp;

Editor::Editor() : _freed{false}, _drawList{nullptr}
{
#ifndef NDEBUG
    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);

    _windowID = renderManager->NewWindowRenderer();

    _viewportCamera = std::make_unique<ViewportCamera>(glm::vec3{0.f}, 10.f, glm::vec2{20.f, 0.f});
    _assetsManager = std::make_unique<AssetsManager>();
    _inputManager = std::make_unique<InputManager>(_windowID);

    renderManager->InitImGui(_windowID);

    renderManager->BindResizeCallback(_windowID, _viewportCamera.get(), ViewportCamera::OnResizeCallback);

    Graph *graph = renderManager->GetGraph(_windowID);

    graph->SetUboSize(sizeof(GlobalUbo));
    graph->SetPopulateUboFunction([this]() {
        GlobalUbo *ubo = new GlobalUbo();
        this->PopulateUbo(ubo);
        return ubo;
    });

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

    Resource const color = graph->NewResource(colorInfo);
    Resource const depth = graph->NewResource(depthInfo);

    PassCreateInfo passCreateInfo{};
    passCreateInfo.writes = {color, depth};
    passCreateInfo.reads = {};
    passCreateInfo.readsUniform = true;
    passCreateInfo.vertexInputInfo = Mesh::Vertex::GetVertexInputInfo();
    passCreateInfo.vertFile = "mesh.vert.spv";
    passCreateInfo.fragFile = "mesh.frag.spv";
    passCreateInfo.debugName = "mesh render";
    passCreateInfo.execute = [=](vk::CommandBuffer commandBuffer) {
        if (_drawList)
        {
            for (Drawable const *drawable : *_drawList)
            {
                if (ensure(drawable))
                {
                    drawable->BindAndDraw(commandBuffer);
                }
            }
        }
    };

    graph->NewPass(passCreateInfo);
    graph->Compile(color, Graph::eToDescriptorSet);
#endif
}

Editor::~Editor()
{
    if (!_freed)
    {
        spdlog::critical("Editor: forgot to Free before deletion");
    }
}

void Editor::Free()
{
    if (_freed)
    {
        spdlog::error("Editor: already freed");
        return;
    }

    _assetsManager->Free();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    RenderManager::Get()->Free();

    _freed = true;
}

bool Editor::ShouldClose() const
{
    return RenderManager::Get()->ShouldClose(_windowID);
}

void Editor::Render()
{
    glfwPollEvents();

    vk::CommandBuffer const commandBuffer = RenderManager::Get()->BeginRender(_windowID);

    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "editor");

    _deferredQueue.clear();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    RenderDockspace();
    RenderViewport();

    if (_viewportCamera)
    {
        ImGui::Begin("Camera");
        frost::RenderEditor(frost::Meta<ViewportCamera>{}, _viewportCamera.get());
        ImGui::End();
    }

    ImGui::Begin("Content Browser");
    RenderContentBrowser();
    ImGui::End();

    ImGui::Begin("Editor Settings");
    frost::RenderEditor(frost::Meta<InputManager>{}, _inputManager.get());
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, (void *)commandBuffer);

    RenderManager::Get()->EndRender(commandBuffer, _windowID);
}

void Editor::Update(double dt)
{
    for (std::function<void()> const &func : _deferredQueue)
    {
        func();
    }
}

void Editor::PopulateUbo(GlobalUbo *ubo)
{
    ubo->camera.viewProjection =
        _viewportCamera->GetCamera()->GetProjection() * _viewportCamera->GetCamera()->GetView();
}

void Editor::InitDockspace(unsigned int dockspaceID)
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspaceID);
    ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->Size);

    ImGuiID dock_mainID = dockspaceID;
    ImGuiID leftID = ImGui::DockBuilderSplitNode(dock_mainID, ImGuiDir_Left, 0.2f, nullptr, &dock_mainID);
    ImGuiID rightID = ImGui::DockBuilderSplitNode(dock_mainID, ImGuiDir_Right, 0.2f, nullptr, &dock_mainID);
    ImGuiID bottomID = ImGui::DockBuilderSplitNode(dock_mainID, ImGuiDir_Down, 0.3f, nullptr, &dock_mainID);

    ImGui::DockBuilderDockWindow("Content Browser", bottomID);
    ImGui::DockBuilderDockWindow("Camera", rightID);
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

    ImGuiWindowFlags const window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
                                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                          ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID const dockspaceID = ImGui::GetID("GlobalDockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0, 0));

    static bool firstCall{true};
    if (firstCall)
    {
        InitDockspace(dockspaceID);
        firstCall = false;
    }

    ImGui::End();
}

void Editor::RenderViewport()
{
    Graph *graph = RenderManager::Get()->GetGraph(_windowID);

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
            graph->Resize((size_t)size.x, (size_t)size.y);
            check(_viewportCamera);
            _viewportCamera->SetAspectRatio((float)size.x / (float)size.y);
        });
        oldSize = size;
    }

    ImGui::End();
}

void Editor::RenderContentBrowser()
{
    if (!ensure(_assetsManager.get()))
    {
        return;
    }

    static std::filesystem::path _currentDirectory = _assetsManager->_fileRoot;

    static float const padding = 16.f;
    static float const thumbnailSize = 128.f;
    float const cellSize = padding + thumbnailSize;

    float const panelWidth = ImGui::GetContentRegionAvail().x;
    int const columnCount = std::max(1, (int)(panelWidth / cellSize));

    if (std::filesystem::canonical(_currentDirectory).compare(std::filesystem::canonical(_assetsManager->_fileRoot)) !=
        0)
    {
        if (ImGui::Button("../"))
        {
            _currentDirectory = std::filesystem::path(_currentDirectory).parent_path();
        }
        ImGui::SameLine();
        if (ImGui::Button("/"))
        {
            _currentDirectory = std::filesystem::path(_assetsManager->_fileRoot);
        }
    }

    ImGui::Columns(columnCount, 0, false);

    int id = 0;
    for (auto &directory : std::filesystem::directory_iterator(_currentDirectory))
    {
        static ImGuiIO &io = ImGui::GetIO();

        static ImFont *Font = io.Fonts->AddFontFromFileTTF(
            (std::string(WSP_EDITOR_ASSETS) + std::string("MaterialIcons-Regular.ttf")).c_str(),
            thumbnailSize - padding * 2.);

        ImGui::PushID(id++);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding, padding));
        ImGui::PushFont(Font);

        if (directory.is_directory())
        {
            std::filesystem::path const path = directory.path().c_str();
            if (ImGui::Button(ICON_MS_FOLDER, ImVec2(thumbnailSize, thumbnailSize)))
            {
                _currentDirectory = directory.path();
            }
            ImGui::PopFont();
            ImGui::Text("%s", path.filename().c_str());
        }
        else
        {
            std::filesystem::path const path = directory.path();
            if (ImGui::Button(ICON_MS_DESCRIPTION, ImVec2(thumbnailSize, thumbnailSize)))
            {
                try
                {
                    std::filesystem::path const relativePath =
                        std::filesystem::relative(path, _assetsManager->_fileRoot);

                    if (_drawList)
                    {
                        _drawList->clear();
                    }
                    else
                    {
                        _drawList = new std::vector<Drawable const *>{};
                    }

                    float furthestRadius = 0.f;

                    for (Mesh const *mesh : _assetsManager->ImportMeshes(relativePath))
                    {
                        furthestRadius = std::max(furthestRadius, mesh->GetRadius());
                        _drawList->push_back((Drawable const *)mesh);
                    }

                    _viewportCamera->SetOrbitTarget({0.f, 0.f, 0.f});
                    _viewportCamera->SetOrbitDistance(furthestRadius * 2.5f);
                    _viewportCamera->Refresh();
                }
                catch (std::exception exception)
                {
                };
            }
            ImGui::PopFont();
            ImGui::Text("%s", path.filename().c_str());
        }
        ImGui::PopStyleVar();
        ImGui::NextColumn();
        ImGui::PopID();
    }
}

#endif
