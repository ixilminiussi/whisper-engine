#include <wsp_assets_manager.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_editor.hpp>
#include <wsp_engine.hpp>
#include <wsp_global_ubo.hpp>
#include <wsp_graph.hpp>
#include <wsp_mesh.hpp>
#include <wsp_renderer.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_window.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include <vulkan/vulkan.hpp>

using namespace wsp;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    delete _graph.release();

    glfwTerminate();

    spdlog::info("Renderer: freed");
}

void Renderer::Render(vk::CommandBuffer commandBuffer, uint32_t frameIndex) const
{
    ZoneScopedN("graph render");

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    FrameMarkStart("frame render");

    check(_graph);
    _graph->Render(commandBuffer, frameIndex);

    FrameMarkEnd("frame render");
}

Graph *Renderer::GetGraph() const
{
    return _graph.get();
}

void Renderer::Initialize(uint32_t width, uint32_t height)
{
    ZoneScopedN("initialize (graph generate + compile)");

    _graph = std::make_unique<Graph>(width, height);
    _graph->SetUboSize(sizeof(ubo::Ubo));

    spdlog::info("Renderer: new window initialized");
}
