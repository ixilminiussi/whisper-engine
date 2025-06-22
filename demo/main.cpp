#include <fl_graph.h>
#include <wsp_engine.h>
#include <wsp_renderer.h>

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
