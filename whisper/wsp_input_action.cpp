#include <wsp_input_action.hpp>

#include <wsp_editor.hpp>
#include <wsp_input_manager.hpp>
#include <wsp_inputs.hpp>
#include <wsp_window.hpp>

#include <GLFW/glfw3.h>
#include <IconsMaterialSymbols.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <stdexcept>

using namespace wsp;

void ButtonAction::Poll(GLFWwindow *handle, double dt)
{
    int newStatus = (_buttonUsage == Usage::eShortcut && _buttons.size() > 0) ? 1 : 0;

    for (Button &button : _buttons)
    {
        switch (button.source)
        {
        case InputSource::eKeyboard:
            button.status = glfwGetKey(handle, button.code);

            if (_buttonUsage == Usage::eShortcut)
            {
                newStatus &= button.status;
            }
            else
            {
                newStatus |= button.status;
            }
            break;
        case InputSource::eMouse:
            button.status = glfwGetMouseButton(handle, button.code);
            if (_buttonUsage == Usage::eShortcut)
            {
                newStatus &= button.status;
            }
            else
            {
                newStatus |= button.status;
            }
            break;
        case InputSource::eGamepad:
            // TODO: Gamepad support
            break;
        }
    }

    bool success = false;

    switch (_buttonUsage)
    {
    case Usage::eHeld:
        if (newStatus == 1)
        {
            success = true;
        }
        break;

    case Usage::ePressed:
    case Usage::eShortcut:
        if (newStatus == 1 && _status == 0)
        {
            success = true;
        }
        break;

    case Usage::eReleased:
        if (newStatus == 0 && _status == 1)
        {
            success = true;
        }
        break;
    case Usage::eToggle:
        if (newStatus != _status)
        {
            success = true;
        }
        break;
    }
    _status = newStatus;

    if (success)
    {
        for (std::function<void(float, int)> func : _functions)
        {
            func(dt, newStatus);
        }
    }
}

void AxisAction::Poll(GLFWwindow *handle, double dt)
{
    switch (_axisUsage)
    {
    case Usage::eButtons:
        for (int i = 0; i < 4; i++)
        {
            if (_buttons[i] == WSP_BUTTON_VOID)
            {
                continue;
            }

            switch (_buttons[i].source)
            {
            case InputSource::eKeyboard:
                _buttons[i].status = glfwGetKey(handle, _buttons[i].code);
                break;
            case InputSource::eMouse:
                _buttons[i].status = glfwGetMouseButton(handle, _buttons[i].code);
                break;
            case InputSource::eGamepad:
                // TODO: Gamepad support
                break;
            }
        }

        _value =
            glm::vec2{float(_buttons[0].status - _buttons[1].status), float(_buttons[2].status - _buttons[3].status)};

        break;
    case Usage::eAxes:
        int windowWidth, windowHeight;
        glfwGetWindowSize(handle, &windowWidth, &windowHeight);
        for (int i = 0; i < 2; i++)
        {
            if (_axes[i] == WSP_AXIS_VOID)
            {
                continue;
            }
            if (_axes[i] == WSP_MOUSE_AXIS_X_ABSOLUTE)
            {
                double x, y;
                glfwGetCursorPos(handle, &x, &y);
                x *= windowWidth * 0.01f;
                _axes[i].value = float(x);
            }
            if (_axes[i] == WSP_MOUSE_AXIS_Y_ABSOLUTE)
            {
                double x, y;
                glfwGetCursorPos(handle, &x, &y);
                y *= windowHeight * 0.01f;
                _axes[i].value = float(y);
            }
            if (_axes[i] == WSP_MOUSE_AXIS_X_RELATIVE)
            {
                double x, y;
                glfwGetCursorPos(handle, &x, &y);
                x *= windowWidth * 0.01f;
                _axes[i].value = float(x) - _axes[i].absValue;
                _axes[i].absValue = float(x);
            }
            if (_axes[i] == WSP_MOUSE_AXIS_Y_RELATIVE)
            {
                double x, y;
                glfwGetCursorPos(handle, &x, &y);
                y *= windowHeight * 0.01f;
                _axes[i].value = float(y) - _axes[i].absValue;
                _axes[i].absValue = float(y);
            }
            if (_axes[i] == WSP_MOUSE_SCROLL_X_RELATIVE)
            {
                _axes[i].value = float(InputManager::scrollX);
            }
            if (_axes[i] == WSP_MOUSE_SCROLL_Y_RELATIVE)
            {
                _axes[i].value = float(InputManager::scrollY);
            }
        }

        _value = glm::vec2{_axes[0].value, _axes[1].value};

        break;
    }

    if (glm::length(_value) > glm::epsilon<float>())
    {
        glm::vec2 const modifierApplied{_value.x * _modifierX, _value.y * _modifierY};
        for (std::function<void(float, glm::vec2)> const func : _functions)
        {
            func(dt, modifierApplied);
        }
    }
}

void ButtonAction::Bind(std::function<void(float, int)> callbackFunction)
{
    _functions.push_back(callbackFunction);
}

void ButtonAction::Bind(std::function<void(float, glm::vec2)> callbackFunction)
{
    throw std::runtime_error("ButtonAction: can only be bound to std::function<void(float, int)>");
}

void ButtonAction::UnbindAll()
{
    _functions.clear();
}

void AxisAction::Bind(std::function<void(float, int)> callbackFunction)
{
    throw std::runtime_error("AxisAction: can only be bound to std::function<void(float, glm::vec2)>");
}

void AxisAction::Bind(std::function<void(float, glm::vec2)> callbackFunction)
{
    _functions.push_back(callbackFunction);
}

void AxisAction::UnbindAll()
{
    _functions.clear();
}
