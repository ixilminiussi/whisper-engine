#include <wsp_engine.hpp>

#include <wsp_devkit.hpp>
#include <wsp_renderer.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <exception>

namespace wsp
{

Renderer *renderer;

namespace engine
{

bool initialized{false};
bool terminated{false};

bool Initialize()
{
    spdlog::info("Engine: began initialization");

    if (!glfwInit())
    {
        spdlog::critical("Engine: GLFW failed to initialize!");
        return false;
    }
    if (initialized)
    {
        spdlog::error("Engine: already initialized");
        return true;
    }

    try
    {
        renderer = new Renderer();
        renderer->Initialize();
        initialized = true;
    }
    catch (std::exception const &exception)
    {
        spdlog::critical("{0}", exception.what());
        return false;
    }

    return true;
}

double GetDeltaTime()
{
    static auto start = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedSeconds = end - start;
    double dt = std::min(0.2, elapsedSeconds.count());
    start = end;

    return dt;
}

void Run()
{
    try
    {
        check(renderer);
        while (!renderer->ShouldClose())
        {
            renderer->Render();

            double dt = GetDeltaTime();

            renderer->Update(dt);
        }
    }
    catch (std::exception const &exception)
    {
        spdlog::critical("{0}", exception.what());
    }
}

void Terminate()
{
    if (!initialized)
    {
        spdlog::error("Engine: never initialized before termination");
        return;
    }
    if (terminated)
    {
        spdlog::error("Engine: already terminated");
        return;
    }

    try
    {
        renderer->Free();
        delete renderer;
    }
    catch (std::exception const &exception)
    {
        spdlog::critical("{0}", exception.what());
    }
}

} // namespace engine

} // namespace wsp
