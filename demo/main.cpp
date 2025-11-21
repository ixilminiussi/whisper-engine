#define TRACY_GPU_CONTEXT
#define TRACY_GPU_VULKAN

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#include <vulkan/vulkan.h>

#include <wsp_editor.hpp>
#include <wsp_render_manager.hpp>

using namespace wsp;

double GetDeltaTime()
{
    static auto start = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> const elapsedSeconds = end - start;
    double const dt = std::min(0.2, elapsedSeconds.count());
    start = end;

    return dt;
}

int main()
{
    spdlog::default_logger()->set_level(spdlog::level::trace);

    Editor *editor = new Editor();

    try
    {
        while (!editor->ShouldClose())
        {
            editor->Render();
            editor->Update(GetDeltaTime());
        }
        editor->Free();
    }
    catch (std::exception exception)
    {
        spdlog::critical("{}", exception.what());
    }
}
