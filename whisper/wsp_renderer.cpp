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

Renderer::Renderer() : _freed{false}
{
}

Renderer::~Renderer()
{
    if (!_freed)
    {
        spdlog::critical("Renderer: forgot to Free before deletion");
    }
}

void Renderer::Free()
{
    spdlog::info("Renderer: began termination");

    if (_freed)
    {
        spdlog::error("Renderer: already freed renderer");
        return;
    }

    _graph->Free();
    _graph.release();

    glfwTerminate();

    spdlog::info("Renderer: terminated");
    _freed = true;
}

void Renderer::Render(vk::CommandBuffer commandBuffer, size_t frameIndex) const
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    FrameMarkStart("frame render");

    check(_graph);
    _graph->Render(commandBuffer, frameIndex);

    FrameMarkEnd("frame render");
}

class Graph *Renderer::GetGraph() const
{
    return _graph.get();
}

void Renderer::Initialize(size_t width, size_t height)
{
    ZoneScopedN("initialize (graph generate + compile)");

    _graph = std::make_unique<Graph>(width, height);
    _graph->SetUboSize(sizeof(GlobalUbo));

    spdlog::info("Renderer: new window initialized");
}
