#include "wsp_engine.h"

#include "wsp_device.h"
#include "wsp_devkit.h"
#include "wsp_window.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace wsp
{

Engine *Engine::_instance{nullptr};

Engine::Engine()
{
}

Engine *Engine::Get()
{
    if (!_instance)
    {
        throw std::runtime_error("Engine: forgot to Kickstart engine before getting it");
    }

    return _instance;
}

void Engine::Kickstart()
{
    if (_instance)
    {
        spdlog::error("Engine: already kickstarted engine");
        return;
    }

    _instance = new Engine();
    _instance->Initialize();
}

void Engine::Shutdown()
{
    if (!_instance)
    {
        spdlog::error("Engine: forgot to Kickstart engine before shutting it down");
        return;
    }

    _instance->Deinitialize();

    delete _instance;
    _instance = nullptr;
}

void Engine::Run()
{
    while (_window && !_window->ShouldClose())
    {
        glfwPollEvents();
    }
}

void Engine::Initialize()
{
    if (!glfwInit())
    {
        throw std::runtime_error("GLFW failed to initialize!");
    }

    _device = new Device();
    _device->Initialize();

    _window = Window::OpenWindow(1024, 1024, "test");
    _device->BindWindow(_window);
}

void Engine::Deinitialize()
{
    _device->Free();
    delete _device;

    _window->Free();
    delete _window;

    glfwTerminate();
}

Device *Engine::GetDevice()
{
    check(_device);
    return _device;
}

} // namespace wsp
