#define TRACY_GPU_CONTEXT
#define TRACY_GPU_VULKAN

#include <vulkan/vulkan.h>

#include <wsp_engine.h>
#include <wsp_graph.h>

// lib
#include <spdlog/spdlog.h>

int main()
{
    if (!wsp::engine::Initialize())
    {
        return 1;
    }

    wsp::engine::Run();

    wsp::engine::Terminate();
}
