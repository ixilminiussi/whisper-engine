#include <wsp_input_manager.hpp>

#include <wsp_devkit.hpp>
#include <wsp_editor.hpp>
#include <wsp_input_action.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_window.hpp>

#include <GLFW/glfw3.h>
#include <IconsMaterialSymbols.h>
#include <imgui.h>
#include <tinyxml2.h>

using namespace wsp;

InputManager::InputManager(WindowID windowID, std::string const &filepath)
    : _windowID{windowID}, _buttonDictionary{}, _axisDictionary{}
{
    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);
    check(renderManager->Validate(windowID));

    GLFWwindow *handle = renderManager->GetGLFWHandle(windowID);

    _gamepadDetected = glfwJoystickPresent(GLFW_JOYSTICK_1);
    glfwSetInputMode(handle, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetScrollCallback(handle, InputManager::ScrollCallback);
}

InputManager::~InputManager()
{
    _buttonDictionary.clear();
    _axisDictionary.clear();
}

void InputManager::AddInput(std::string const &name, ButtonAction const &newInput)
{
    if (ensure(!_buttonDictionary.contains(name)))
    {
        _buttonDictionary[name] = newInput;
        spdlog::info("InputManager: New button [{0}] added", name.c_str());
        return;
    }

    spdlog::error("InputManager: Attempt at rebinding existing button [{0}]", name.c_str());
}

void InputManager::AddInput(std::string const &name, AxisAction const &newInput)
{
    if (ensure(!_axisDictionary.contains(name)))
    {
        _axisDictionary[name] = newInput;
        spdlog::info("InputManager: New axis [{0}] added", name.c_str());
        return;
    }

    spdlog::error("InputManager: Attempt at rebinding existing axis [{0}]", name.c_str());
}

void InputManager::PollEvents(double dt)
{
    isScrolling = false;
    glfwPollEvents();
    if (!isScrolling)
    {
        OnScrollStop();
    }

    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);
    GLFWwindow *handle = renderManager->GetGLFWHandle(_windowID);

    for (auto &[name, button] : _buttonDictionary)
    {
        button.Poll(handle, dt);
    }
    for (auto &[name, axis] : _axisDictionary)
    {
        axis.Poll(handle, dt);
    }
}

void InputManager::UnbindAll()
{
    for (auto &[name, button] : _buttonDictionary)
    {
        button.UnbindAll();
    }
    for (auto &[name, axis] : _axisDictionary)
    {
        axis.UnbindAll();
    }
}

void InputManager::SetMouseCapture(bool b) const
{
    // used to hide the cursor
    unsigned char transparent_pixel[4] = {0, 0, 0, 0};
    GLFWimage blankImg;
    blankImg.width = 1;
    blankImg.height = 1;
    blankImg.pixels = transparent_pixel;

    static GLFWcursor *blankCursor = glfwCreateCursor(&blankImg, 0, 0);
    // end

    RenderManager *renderManager = RenderManager::Get();
    check(renderManager);
    GLFWwindow *handle = renderManager->GetGLFWHandle(_windowID);

    if (b)
    {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursor(handle, blankCursor);
    }
    else
    {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

double InputManager::scrollX{0.};
double InputManager::scrollY{0.};
bool InputManager::isScrolling{false};

void InputManager::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    scrollX = xoffset;
    scrollY = yoffset;
    isScrolling = true;
}

void InputManager::OnScrollStop()
{
    scrollX = 0.0;
    scrollY = 0.0;
}
