#include "wsp_assets_manager.hpp"
#include <memory>
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

std::unique_ptr<Renderer> renderer;
std::unique_ptr<AssetsManager> assetsManager;

std::vector<Drawable const *> drawList{};

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
        assetsManager = std::make_unique<AssetsManager>();
        renderer = std::make_unique<Renderer>();

        renderer->Initialize(&drawList);
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
    std::chrono::duration<double> const elapsedSeconds = end - start;
    double const dt = std::min(0.2, elapsedSeconds.count());
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

            double const dt = GetDeltaTime();

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
        assetsManager->Free();
        renderer->Free();
        renderer.release();
    }
    catch (std::exception const &exception)
    {
        spdlog::critical("{0}", exception.what());
    }
}

void Inspect(std::vector<Drawable const *> drawables)
{
    drawList.clear();

    drawList.insert(drawList.end(), drawables.begin(), drawables.end());
}

} // namespace engine

} // namespace wsp
