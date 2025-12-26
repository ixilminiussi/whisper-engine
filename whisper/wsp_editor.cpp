#ifndef NDEBUG
#include <wsp_editor.hpp>

#include <wsp_constants.hpp>
#include <wsp_devkit.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_camera.hpp>
#include <wsp_custom_imgui.hpp>
#include <wsp_drawable.hpp>
#include <wsp_engine.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_handles.hpp>
#include <wsp_input_manager.hpp>
#include <wsp_inputs.hpp>
#include <wsp_mesh.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_renderer.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_swapchain.hpp>
#include <wsp_texture.hpp>
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

Editor::Editor() : _drawList{nullptr}
{
    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);

    _windowID = renderManager->NewWindowRenderer();

    _viewportCamera = std::make_unique<ViewportCamera>(glm::vec3{0.f}, 10.f, glm::vec2{20.f, 0.f});
    _inputManager = std::make_unique<InputManager>(_windowID);

    _inputManager->AddInput("look", AxisAction{WSP_MOUSE_AXIS_X_RELATIVE, WSP_MOUSE_AXIS_Y_RELATIVE});
    _inputManager->AddInput("left click on", ButtonAction{ButtonAction::Usage::ePressed, {WSP_MOUSE_BUTTON_LEFT}});
    _inputManager->AddInput("left click off", ButtonAction{ButtonAction::Usage::eReleased, {WSP_MOUSE_BUTTON_LEFT}});

    _inputManager->BindAxis("look", &ViewportCamera::OnMouseMovement, _viewportCamera.get());
    _inputManager->BindButton("left click on", &Editor::OnClick, this);
    _inputManager->BindButton("left click off", &Editor::OnClick, this);

    renderManager->InitImGui(_windowID);

    renderManager->BindResizeCallback(_windowID, _viewportCamera.get(), ViewportCamera::OnResizeCallback);

    Graph *graph = renderManager->GetGraph(_windowID);

    AssetsManager::Get()->LoadDefaults();

    graph->SetUboSize(sizeof(ubo::Ubo));
    graph->SetPopulateUboFunction([this]() {
        ubo::Ubo *uboInfo = new ubo::Ubo();
        this->PopulateUbo(uboInfo);
        return uboInfo;
    });

    ResourceCreateInfo colorInfo{};
    colorInfo.usage = ResourceUsage::eColor;
    colorInfo.format = vk::Format::eR16G16B16A16Sfloat;
    colorInfo.debugName = "color";

    ResourceCreateInfo depthInfo{};
    depthInfo.usage = ResourceUsage::eDepth;
    depthInfo.format = vk::Format::eD16Unorm;
    depthInfo.clear.depthStencil = vk::ClearDepthStencilValue{1.};
    depthInfo.debugName = "depth";

    ResourceCreateInfo postInfo{};
    postInfo.usage = ResourceUsage::eColor;
    postInfo.format = vk::Format::eR16G16B16A16Sfloat;
    postInfo.debugName = "color";

    Resource const colorResource = graph->NewResource(colorInfo);
    Resource const depthResource = graph->NewResource(depthInfo);
    Resource const postResource = graph->NewResource(postInfo);

    PassCreateInfo backgroundPassInfo{};
    backgroundPassInfo.writes = {colorResource, depthResource};
    backgroundPassInfo.readsUniform = true;
    backgroundPassInfo.staticTextures = {AssetsManager::Get()->GetStaticCubemaps()};
    backgroundPassInfo.vertFile = "background.vert.spv";
    backgroundPassInfo.fragFile = "background.frag.spv";
    backgroundPassInfo.debugName = "background render";
    backgroundPassInfo.execute = [](vk::CommandBuffer commandBuffer, vk::PipelineLayout) {
        commandBuffer.draw(6u, 1u, 0u, 0u);
    };

    graph->NewPass(backgroundPassInfo);

    PassCreateInfo meshPassInfo{};
    meshPassInfo.writes = {colorResource, depthResource};
    meshPassInfo.readsUniform = true;
    meshPassInfo.staticTextures = {AssetsManager::Get()->GetStaticTextures(),
                                   AssetsManager::Get()->GetStaticEngineTextures(),
                                   AssetsManager::Get()->GetStaticCubemaps()};
    meshPassInfo.vertexInputInfo = Mesh::Vertex::GetVertexInputInfo();
    meshPassInfo.pushConstantSize = sizeof(Mesh::PushData);
    meshPassInfo.vertFile = "mesh.vert.spv";
    meshPassInfo.fragFile = "mesh.frag.spv";
    meshPassInfo.debugName = "mesh render";
    meshPassInfo.execute = [=](vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) {
        if (_drawList)
        {
            for (Drawable const *drawable : *_drawList)
            {
                drawable->BindAndDraw(commandBuffer, pipelineLayout);
            }
        }
    };

    graph->NewPass(meshPassInfo);

    PassCreateInfo postPassInfo{};
    postPassInfo.writes = {postResource};
    postPassInfo.reads = {colorResource};
    postPassInfo.vertFile = "tonemapping.vert.spv";
    postPassInfo.fragFile = "tonemapping.frag.spv";
    postPassInfo.debugName = "tonemapping render";
    postPassInfo.execute = [](vk::CommandBuffer commandBuffer, vk::PipelineLayout) {
        commandBuffer.draw(6u, 1u, 0u, 0u);
    };

    graph->NewPass(postPassInfo);

    _rebuild = [graph, postResource]() { graph->Compile(postResource, Graph::eToDescriptorSet); };

    _rebuild();
}

Editor::~Editor()
{
    AssetsManager::Get()->UnloadAll();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    RenderManager::Get()->Free();

    spdlog::info("Editor: shut down");
}

bool Editor::ShouldClose() const
{
    return RenderManager::Get()->ShouldClose(_windowID);
}

void Editor::Render()
{
    vk::CommandBuffer const commandBuffer = RenderManager::Get()->BeginRender(_windowID);

    { // so that TracyCtx dies BEFORE flushing
        extern TracyVkCtx TRACY_CTX;
        TracyVkZone(TRACY_CTX, commandBuffer, "editor");

        _deferredQueue.clear();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static bool showViewport = true;
        static bool showViewportCamera = true;
        static bool showContentBrowser = true;

        static bool showEditorSettings = false;
        static bool showGraphicsSettings = true;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("options"))
            {
                ImGui::MenuItem("editor preferences", nullptr, &showEditorSettings);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("windows"))
            {
                ImGui::MenuItem("viewport", nullptr, &showViewport);
                ImGui::MenuItem("viewport camera", nullptr, &showViewportCamera);
                ImGui::MenuItem("content browser", nullptr, &showContentBrowser);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        RenderDockspace();

        if (showViewport)
        {
            RenderViewport(&showViewport);
        }

        if (_viewportCamera)
        {
            ImGui::Begin("Camera", &showViewportCamera);
            frost::RenderEditor(frost::Meta<ViewportCamera>{}, _viewportCamera.get());
            ImGui::End();
        }

        if (showContentBrowser)
        {
            RenderContentBrowser(&showContentBrowser);
        }

        if (showGraphicsSettings)
        {
            RenderGraphicsManager(&showGraphicsSettings);
        }

        if (showEditorSettings)
        {
            ImGui::Begin("Editor Settings", &showEditorSettings);
            frost::RenderEditor(frost::Meta<InputManager>{}, _inputManager.get());
            ImGui::End();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void *)commandBuffer);
    }

    RenderManager::Get()->EndRender(commandBuffer, _windowID);
}

void Editor::Update(double dt)
{
    _inputManager->PollEvents(dt);

    for (std::function<void()> const &func : _deferredQueue)
    {
        func();
    }

    _viewportCamera->Update(dt);
}

void Editor::PopulateUbo(ubo::Ubo *ubo)
{
    check(ubo);

    check(_viewportCamera);
    check(_viewportCamera->GetCamera());

    ubo->camera.viewProjection =
        _viewportCamera->GetCamera()->GetProjection() * _viewportCamera->GetCamera()->GetView();
    ubo->camera.view = _viewportCamera->GetCamera()->GetView();
    ubo->camera.projection = _viewportCamera->GetCamera()->GetProjection();
    ubo->camera.inverseView = glm::inverse(_viewportCamera->GetCamera()->GetView());
    ubo->camera.inverseProjection = glm::inverse(_viewportCamera->GetCamera()->GetProjection());
    ubo->camera.position = _viewportCamera->GetCamera()->GetPosition();

    memcpy(ubo->materials, AssetsManager::Get()->GetMaterialInfos().data(), MAX_MATERIALS);
}

void Editor::OnClick(double dt, int value)
{
    if (_isHoveringViewport && value)
    {
        _viewportCamera->Possess(ViewportCamera::PossessionMode::eOrbit);
        _inputManager->SetMouseCapture(true);
    }
    else
    {
        _viewportCamera->Possess(ViewportCamera::PossessionMode::eReleased);
        _inputManager->SetMouseCapture(false);
    }
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

void Editor::RenderViewport(bool *show)
{
    Graph *graph = RenderManager::Get()->GetGraph(_windowID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", show);
    ImGui::PopStyleVar(3);

    _isHoveringViewport = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
                                                 ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImVec2 const size = ImGui::GetContentRegionAvail();
    static ImVec2 oldSize = ImVec2(10, 10);
    ImGui::Image((ImTextureID)(graph->GetTargetDescriptorSet().operator VkDescriptorSet()), size);
    if (oldSize.x != size.x && oldSize.y != size.y)
    {
        _deferredQueue.push_back([=]() {
            check(graph);
            graph->Resize((uint32_t)size.x, (uint32_t)size.y);
            check(_viewportCamera);
            _viewportCamera->SetAspectRatio((float)size.x / (float)size.y);
        });
        oldSize = size;
    }

    ImGui::End();
}

void Editor::RenderContentBrowser(bool *show)
{
    AssetsManager *assetsManager = AssetsManager::Get();

    ImGui::Begin("Content Browser", show);

    ImGui::BeginTabBar("ContentTab");

    if (ImGui::BeginTabItem("files"))
    {
        static std::filesystem::path _currentDirectory = assetsManager->_fileRoot;

        static float const padding = 16.f;
        static float const thumbnailSize = 128.f;
        float const cellSize = padding + thumbnailSize;

        float const panelWidth = ImGui::GetContentRegionAvail().x;
        int const columnCount = std::max(1, (int)(panelWidth / cellSize));

        if (std::filesystem::canonical(_currentDirectory)
                .compare(std::filesystem::canonical(assetsManager->_fileRoot)) != 0)
        {
            if (wsp::VanillaButton("../"))
            {
                _currentDirectory = std::filesystem::path(_currentDirectory).parent_path();
            }
            ImGui::SameLine();
            if (wsp::VanillaButton("/"))
            {
                _currentDirectory = std::filesystem::path(assetsManager->_fileRoot);
            }
        }

        ImGui::Columns(columnCount, 0, false);

        int id = 0;
        for (auto &directory : std::filesystem::directory_iterator(_currentDirectory))
        {
            ImGui::PushID(id++);

            if (directory.is_directory())
            {
                std::filesystem::path const path = directory.path().c_str();
                if (wsp::ThumbnailButton(ICON_MS_FOLDER))
                {
                    _currentDirectory = directory.path();
                }
                ImGui::Text("%s", path.filename().c_str());
            }
            else
            {
                std::filesystem::path const path = directory.path();

                bool r = false;
                if (path.extension().compare(".png") == 0)
                {
                    Image const *image = assetsManager->FindImage(path);
                    if (image)
                    {
                        ImTextureID const textureID = assetsManager->RequestTextureID(image);
                        r = wsp::ThumbnailButton(textureID);
                    }
                    else
                    {
                        r = wsp::ThumbnailButton(ICON_MS_QUESTION_MARK);
                    }
                }
                else
                {
                    r = wsp::ThumbnailButton(ICON_MS_DESCRIPTION);
                }

                if (r)
                {
                    _deferredQueue.push_back([=]() {
                        try
                        {
                            std::filesystem::path const relativePath =
                                std::filesystem::relative(path, assetsManager->_fileRoot);

                            if (relativePath.extension().compare(".gltf") == 0 ||
                                relativePath.extension().compare(".glb") == 0)
                            {
                                if (_drawList)
                                {
                                    _drawList->clear();
                                }
                                else
                                {
                                    _drawList = new std::vector<Drawable const *>{};
                                }

                                float furthestRadius = 0.f;

                                for (Drawable const *drawable : assetsManager->ImportGlTF(relativePath))
                                {
                                    furthestRadius = std::max(furthestRadius, drawable->GetRadius());
                                    _drawList->push_back((Drawable const *)drawable);
                                }

                                _viewportCamera->SetOrbitTarget({0.f, 0.f, 0.f});
                                _viewportCamera->SetOrbitDistance(furthestRadius * 2.5f);
                                _viewportCamera->Refresh();
                            }
                            else if (relativePath.extension().compare(".png") == 0 && _skyboxFlag)
                            {
                                // assetsManager->ImportCubemap(relativePath);
                                _skyboxFlag = false;
                            }
                        }
                        catch (std::exception const &exception)
                        {
                            spdlog::critical("{}", exception.what());
                        };
                    });
                }
                ImGui::Text("%s", path.filename().c_str());
            }
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);

        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("materials"))
    {
        static float const columnSize = 256.f;

        float const panelWidth = ImGui::GetContentRegionAvail().x;
        int const columnCount = std::max(1, (int)(panelWidth / columnSize));

        ImGui::Columns(columnCount, 0, false);

        for (auto &material : assetsManager->_materials)
        {
            ImGui::BeginChild(material.GetName().c_str());
            ImGui::Text("%s", material.GetName().c_str());
            frost::RenderEditor(frost::Meta<Material>{}, &material);
            ImGui::EndChild();

            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void Editor::RenderGraphicsManager(bool *show)
{
    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);

    check(show);

    ImGui::Begin("Graphics Manager", show);

    if (wsp::VanillaButton("Rebuild"))
    {
        _rebuild();
    }

    ImGui::BeginDisabled(_skyboxFlag);
    if (wsp::GreenButton("Select Skybox"))
    {
        _skyboxFlag = true;
    }
    ImGui::EndDisabled();

    ImGui::End();
}

#endif
