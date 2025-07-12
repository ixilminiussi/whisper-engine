#include "wsp_engine.hpp"

#include "wsp_renderer.hpp"

// lib
#include <GLFW/glfw3.h>
#include <exception>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

// std
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

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

void Run()
{
    try
    {
        renderer->Run();
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
