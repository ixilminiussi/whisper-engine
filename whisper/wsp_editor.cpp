#include <wsp_editor.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_camera.hpp>
#include <wsp_constants.hpp>
#include <wsp_custom_imgui.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_drawable.hpp>
#include <wsp_engine.hpp>
#include <wsp_environment.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_handles.hpp>
#include <wsp_input_manager.hpp>
#include <wsp_inputs.hpp>
#include <wsp_mesh.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_renderer.hpp>
#include <wsp_scene.hpp>
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

Editor::Editor() : _scene{nullptr}
{
    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);

    _windowID = renderManager->NewWindowRenderer();

    _viewportCamera = std::make_unique<ViewportCamera>(glm::vec3{0.f}, 10.f, glm::vec2{20.f, 0.f});
    _inputManager = std::make_unique<InputManager>(_windowID);

    _inputManager->AddInput("look", AxisAction{WSP_MOUSE_AXIS_X_RELATIVE, WSP_MOUSE_AXIS_Y_RELATIVE});
    _inputManager->AddInput("left click on", ButtonAction{ButtonAction::Usage::ePressed, {WSP_MOUSE_BUTTON_LEFT}});
    _inputManager->AddInput("left click off", ButtonAction{ButtonAction::Usage::eReleased, {WSP_MOUSE_BUTTON_LEFT}});
    _inputManager->AddInput("right click on", ButtonAction{ButtonAction::Usage::ePressed, {WSP_MOUSE_BUTTON_RIGHT}});
    _inputManager->AddInput("right click off", ButtonAction{ButtonAction::Usage::eReleased, {WSP_MOUSE_BUTTON_RIGHT}});
    _inputManager->AddInput("lift", ButtonAction{ButtonAction::Usage::eHeld, {WSP_KEY_SPACE}});
    _inputManager->AddInput("sink", ButtonAction{ButtonAction::Usage::eHeld, {WSP_KEY_LEFT_SHIFT}});
    _inputManager->AddInput("move", AxisAction{WSP_KEY_D, WSP_KEY_A, WSP_KEY_W, WSP_KEY_S});
    _inputManager->AddInput("scroll", AxisAction{WSP_MOUSE_SCROLL_X_RELATIVE, WSP_MOUSE_SCROLL_Y_RELATIVE});

    _inputManager->BindAxis("look", &ViewportCamera::OnMouseMovement, _viewportCamera.get());
    _inputManager->BindAxis("move", &ViewportCamera::OnKeyboardMovement, _viewportCamera.get());
    _inputManager->BindAxis("move", &ViewportCamera::OnKeyboardMovement, _viewportCamera.get());
    _inputManager->BindAxis("scroll", &ViewportCamera::OnMouseScroll, _viewportCamera.get());
    _inputManager->BindButton("lift", &ViewportCamera::Lift, _viewportCamera.get());
    _inputManager->BindButton("sink", &ViewportCamera::Sink, _viewportCamera.get());
    _inputManager->BindButton("left click on", &Editor::OnLeftClick, this);
    _inputManager->BindButton("left click off", &Editor::OnLeftClick, this);
    _inputManager->BindButton("right click on", &Editor::OnRightClick, this);
    _inputManager->BindButton("right click off", &Editor::OnRightClick, this);

    renderManager->InitImGui(_windowID);

    renderManager->BindResizeCallback(_windowID, _viewportCamera.get(), ViewportCamera::OnResizeCallback);

    Graph *graph = renderManager->GetGraph(_windowID);

    _environments.emplace_back(
        "alpes",
        std::make_unique<Environment>(
            (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"alpes-skybox.exr"}).lexically_normal(),
            (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"alpes-irradiance.exr"})
                .lexically_normal(),
            glm::vec2{3.35f, .87f}, glm::vec3{1.f, 1.f, 1.f}, 8.f));

    _environments.emplace_back(
        "puresky day",
        std::make_unique<Environment>(
            (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"puresky-day-skybox.exr"})
                .lexically_normal(),
            (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"puresky-day-irradiance.exr"})
                .lexically_normal(),
            glm::vec2{-.66f, -.97f}, glm::vec3{1.f, 1.f, 1.f}, 8.f));

    _environments.emplace_back(
        "venice sunset",
        std::make_unique<Environment>(
            (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"venice-sunset-skybox.exr"})
                .lexically_normal(),
            (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"venice-sunset-irradiance.exr"})
                .lexically_normal(),
            glm::vec2{4.08f, 3.04f}, glm::vec3{1.f, .406, .177}, 4.7f));

    _environments.emplace_back(
        "workshop", std::make_unique<Environment>(
                        (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"workshop-skybox.exr"})
                            .lexically_normal(),
                        (std::filesystem::path{WSP_ENGINE_ASSETS} / std::filesystem::path{"workshop-irradiance.exr"})
                            .lexically_normal(),
                        glm::vec2{3.35f, .87f}, glm::vec3{1.f, 1.f, 1.f}, 0.f));

    AssetsManager::Get()->LoadDefaults();

    SelectEnvironment(0);

    graph->SetUboSize(sizeof(ubo::Ubo));
    graph->SetPopulateUboFunction([this]() {
        ubo::Ubo *uboInfo = new ubo::Ubo();
        this->PopulateUbo(uboInfo);
        return uboInfo;
    });

    ResourceCreateInfo shadowInfo{};
    shadowInfo.usage = ResourceUsage::eDepth;
    shadowInfo.format = vk::Format::eD32Sfloat;
    shadowInfo.clear.depthStencil = vk::ClearDepthStencilValue{1.};
    shadowInfo.debugName = "shadowMap";
    shadowInfo.extent = vk::Extent2D{1024, 1024};

    ResourceCreateInfo colorInfo{};
    colorInfo.usage = ResourceUsage::eColor;
    colorInfo.format = vk::Format::eR16G16B16A16Sfloat;
    colorInfo.debugName = "color";

    ResourceCreateInfo depthInfo{};
    depthInfo.usage = ResourceUsage::eDepth;
    depthInfo.format = vk::Format::eD32Sfloat;
    depthInfo.clear.depthStencil = vk::ClearDepthStencilValue{1.};
    depthInfo.debugName = "depth";

    ResourceCreateInfo prepassInfo{};
    prepassInfo.usage = ResourceUsage::eColor;
    prepassInfo.format = vk::Format::eR16G16B16A16Sfloat;
    prepassInfo.clear.color = vk::ClearColorValue{0.f, 0.f, 0.f, 1.f};
    prepassInfo.debugName = "prepass";

    ResourceCreateInfo postInfo{};
    postInfo.usage = ResourceUsage::eColor;
    postInfo.format = vk::Format::eR16G16B16A16Sfloat;
    postInfo.debugName = "post process";

    Resource const shadowResource = graph->NewResource(shadowInfo);
    Resource const colorResource = graph->NewResource(colorInfo);
    Resource const depthResource = graph->NewResource(depthInfo);
    Resource const postResource = graph->NewResource(postInfo);
    Resource const prepassResource = graph->NewResource(prepassInfo);

    PassCreateInfo prepassPassInfo{};
    prepassPassInfo.writes = {depthResource, prepassResource};
    prepassPassInfo.readsUniform = true;
    prepassPassInfo.vertexInputInfo = Mesh::Vertex::GetVertexInputInfo();
    prepassPassInfo.pushConstantSize = sizeof(Mesh::PushData);
    prepassPassInfo.staticTextures = {AssetsManager::Get()->GetStaticTextures()};
    prepassPassInfo.vertFile = "prepass.vert.spv";
    prepassPassInfo.fragFile = "prepass.frag.spv";
    prepassPassInfo.debugName = "prepass render";
    prepassPassInfo.execute = [&](vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) {
        ZoneScopedN("draw calls");
        if (_scene)
        {
            _scene->Draw(commandBuffer, pipelineLayout, {});
        }
    };

    graph->NewPass(prepassPassInfo);

    PassCreateInfo shadowMapPassInfo{};
    shadowMapPassInfo.writes = {shadowResource};
    shadowMapPassInfo.readsUniform = true;
    shadowMapPassInfo.vertexInputInfo = Mesh::Vertex::GetVertexInputInfo();
    shadowMapPassInfo.pushConstantSize = sizeof(Mesh::PushData);
    shadowMapPassInfo.vertFile = "shadowmapping.vert.spv";
    shadowMapPassInfo.fragFile = "shadowmapping.frag.spv";
    shadowMapPassInfo.debugName = "shadowMap render";
    shadowMapPassInfo.execute = [&](vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) {
        ZoneScopedN("draw calls");
        if (_scene)
        {
            _scene->Draw(commandBuffer, pipelineLayout, {});
        }
    };

    graph->NewPass(shadowMapPassInfo);

    PassCreateInfo meshPassInfo{};
    meshPassInfo.reads = {shadowResource, prepassResource};
    meshPassInfo.writes = {colorResource, depthResource};
    meshPassInfo.readsUniform = true;
    meshPassInfo.staticTextures = {AssetsManager::Get()->GetStaticTextures(), AssetsManager::Get()->GetStaticNoises(),
                                   AssetsManager::Get()->GetStaticCubemaps()};
    meshPassInfo.vertexInputInfo = Mesh::Vertex::GetVertexInputInfo();
    meshPassInfo.pushConstantSize = sizeof(Mesh::PushData);
    meshPassInfo.vertFile = "mesh.vert.spv";
    meshPassInfo.fragFile = "mesh.frag.spv";
    meshPassInfo.debugName = "mesh render";
    meshPassInfo.execute = [&](vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) {
        ZoneScopedN("draw calls");
        if (_scene)
        {
            _scene->Draw(commandBuffer, pipelineLayout, {});
        }
    };

    Pass const meshPass = graph->NewPass(meshPassInfo);

    PassCreateInfo backgroundPassInfo{};
    backgroundPassInfo.writes = {colorResource, depthResource};
    backgroundPassInfo.passDependencies = {meshPass};
    backgroundPassInfo.readsUniform = true;
    backgroundPassInfo.staticTextures = {AssetsManager::Get()->GetStaticCubemaps()};
    backgroundPassInfo.vertFile = "background.vert.spv";
    backgroundPassInfo.fragFile = "background.frag.spv";
    backgroundPassInfo.debugName = "background render";
    backgroundPassInfo.execute = [](vk::CommandBuffer commandBuffer, vk::PipelineLayout) {
        commandBuffer.draw(6u, 1u, 0u, 0u);
    };

    graph->NewPass(backgroundPassInfo);

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
    SafeDeviceAccessor::Get()->WaitIdle();

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
        ZoneScopedN("render editor");

        extern TracyVkCtx TRACY_CTX;
        TracyVkZone(TRACY_CTX, commandBuffer, "editor");

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static bool showViewport = true;
        static bool showViewSettings = true;
        static bool showContentBrowser = true;

        static bool showEditorSettings = false;
        static bool showFrameInfo = true;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("options"))
            {
                ImGui::MenuItem("editor preferences", nullptr, &showEditorSettings);
                if (ImGui::MenuItem("reload shaders"))
                {
                    _rebuild();
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("windows"))
            {
                ImGui::MenuItem("viewport", nullptr, &showViewport);
                ImGui::MenuItem("view settings", nullptr, &showViewSettings);
                ImGui::MenuItem("content browser", nullptr, &showContentBrowser);
                ImGui::MenuItem("frame info", nullptr, &showFrameInfo);
                ImGui::EndMenu();
            }

            if (showFrameInfo)
            {
                RenderFrameInfoMenuBar();
            }

            ImGui::EndMainMenuBar();
        }

        RenderDockspace();

        if (showViewport)
        {
            RenderViewport(&showViewport);
        }

        ImGui::Begin("View Settings", &showViewSettings);
        if (_viewportCamera)
        {
            ImGui::SeparatorText("Viewport Camera");
            frost::RenderEditor(frost::Meta<ViewportCamera>{}, _viewportCamera.get());
        }

        ImGui::SeparatorText("Environment");
        auto const &[selectedName, selectedEnvironment] = _environments[_selectedEnvironment];
        if (ImGui::BeginCombo("environment", selectedName.c_str()))
        {
            int i = 0;
            for (auto const &[name, environment] : _environments)
            {
                bool const isSelected = name.compare(selectedName) == 0;
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    _deferredQueue.push_back([i, this]() { SelectEnvironment(i); });
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
                i++;
            }
            ImGui::EndCombo();
        }
        frost::RenderEditor(frost::Meta<Environment>{}, _environments[_selectedEnvironment].second.get());
        ImGui::End();

        if (showContentBrowser)
        {
            RenderContentBrowser(&showContentBrowser);
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
    _deferredQueue.clear();

    _deltaTime = dt;

    // _viewportCamera->Orbit({60.f * dt, 0.});
    _viewportCamera->Refresh();
}

void Editor::PopulateUbo(ubo::Ubo *ubo) const
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

    check(_environments[_selectedEnvironment].second);
    _environments[_selectedEnvironment].second->PopulateUbo(ubo);

    auto const &materialInfos = AssetsManager::Get()->GetMaterialInfos();
    memcpy(ubo->materials, materialInfos.data(), materialInfos.size() * sizeof(ubo::Material));
}

void Editor::OnLeftClick(double dt, int value)
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

void Editor::OnRightClick(double dt, int value)
{
    if (_isHoveringViewport && value)
    {
        _viewportCamera->Possess(ViewportCamera::PossessionMode::eMove);
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
    ImGui::DockBuilderDockWindow("View Settings", rightID);
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
                if (path.extension().compare(".png") == 0 || path.extension().compare(".jpg") == 0 ||
                    path.extension().compare(".jpeg") == 0)
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
                                _scene = assetsManager->ImportGlTF(relativePath);

                                _viewportCamera->SetOrbitPoint({0.f, 0.f, 0.f});

                                _environments[_selectedEnvironment].second->SetShadowMapRadius(100.f);
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

        int i = 0;
        for (auto &material : assetsManager->_materials)
        {
            ImGui::PushID(i++);

            if (ImGui::TreeNode(material.GetName().c_str()))
            {
                frost::RenderEditor(frost::Meta<Material>{}, &material);
                ImGui::TreePop();
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }
        spdlog::info(assetsManager->_materials.size());

        ImGui::Columns(1);

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void Editor::RenderFrameInfoMenuBar() const
{
    vk::PhysicalDeviceMemoryProperties2 memProps2{};
    vk::PhysicalDeviceMemoryBudgetPropertiesEXT budgetProps{};
    Device const *device = SafeDeviceAccessor::Get();

    device->GetMemoryProperties(&memProps2, &budgetProps);

    auto const &heaps = memProps2.memoryProperties.memoryHeaps;

    float const FPS = 1. / (float)_deltaTime;
    ImGui::TextUnformatted("FPS:");
    ImGui::SameLine();
    if (FPS > 60.f)
    {
        wsp::GreenText("%.1f", FPS);
    }
    else if (FPS > 45.f)
    {
        wsp::YellowText("%.1f", FPS);
    }
    else
    {
        wsp::RedText("%.1f", FPS);
    }

    ImGui::SameLine();

    for (uint32_t i = 0; i < memProps2.memoryProperties.memoryHeapCount; ++i)
    {
        if (!(heaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal))
        {
            continue;
        }

        constexpr double toGiB = 1024.0 * 1024.0 * 1024.0;
        double const usage = budgetProps.heapUsage[i] / toGiB;
        double const budget = budgetProps.heapBudget[i] / toGiB;

        ImGui::TextUnformatted("VRAM:");
        ImGui::SameLine();

        if (usage > budget)
            wsp::RedText("%.2f / %.2f GiB", usage, budget);
        else if (usage > 0.9 * budget)
            wsp::YellowText("%.2f / %.2f GiB", usage, budget);
        else
            wsp::GreenText("%.2f / %.2f GiB", usage, budget);

        break; // one heap only
    }

    ImGui::SameLine();
    wsp::YellowText(device->GetDeviceName().c_str());
}

void Editor::SelectEnvironment(int i)
{
    check(i < _environments.size());
    _selectedEnvironment = i;

    _environments[i].second->Load();
}
