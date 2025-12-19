#ifndef WSP_INPUT_MANAGER
#define WSP_INPUT_MANAGER

#include <wsp_input_action.hpp>
#include <wsp_typedefs.hpp>
#include <wsp_types/dictionary.hpp>

#include <spdlog/spdlog.h>

#include <wsp_devkit.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>

#include <.generated/wsp_input_manager.generated.hpp>

class GLFWwindow;

namespace wsp
{

WCLASS()
class InputManager
{
  public:
    InputManager(WindowID,
                 std::string const &filepath = std::string(GAME_FILES) + std::string("properties/input-manager.xml"));
    ~InputManager();

    template <typename T>
    void BindAxis(std::string const &name, void (T::*callbackFunction)(double, glm::vec2), T *instance);
    template <typename T>
    void BindButton(std::string const &name, void (T::*callbackFunction)(double, int), T *instance);
    void UnbindAll();
    void AddInput(std::string const &name, class ButtonAction const &);
    void AddInput(std::string const &name, class AxisAction const &);
    void PollEvents(double dt);

    void SetMouseCapture(bool) const;

    static void ScrollCallback(GLFWwindow *, double xoffset, double yoffset);
    static void OnScrollStop();
    static double scrollX, scrollY;
    static bool isScrolling;

    WCLASS_BODY$InputManager();

  private:
    bool _gamepadDetected{false};

    WPROPERTY()
    dictionary<std::string, class ButtonAction> _buttonDictionary;
    WPROPERTY()
    dictionary<std::string, class AxisAction> _axisDictionary;

    WindowID _windowID;
};

template <typename T>
inline void InputManager::BindAxis(std::string const &name, void (T::*callbackFunction)(double, glm::vec2), T *instance)
{
    if (!_axisDictionary.contains(name))
    {
        spdlog::error("InputManager: Attempt at binding non existant input [{0}]", name);
        return;
    }

    _axisDictionary[name].Bind([instance, callbackFunction](double dt, glm::vec2 value) {
        if (instance)
            (instance->*callbackFunction)(dt, value);
        else
            spdlog::warn("InputManager: action being called on deleted object");
    });
    spdlog::info("InputManager: [{0}] bound to new function", name);
}

template <typename T>
inline void InputManager::BindButton(std::string const &name, void (T::*callbackFunction)(double, int), T *instance)
{
    if (!_buttonDictionary.contains(name))
    {
        spdlog::warn("InputManager: Attempt at binding non existant input [{0}]", name);
        return;
    }

    _buttonDictionary[name].Bind([instance, callbackFunction](double dt, int val) {
        if (instance)
            (instance->*callbackFunction)(dt, val);
        else
            spdlog::warn("InputManager: Action being called on deleted object");
    });
    spdlog::info("InputManager: [{0}] bound to new function", name);
}

} // namespace wsp

WGENERATED_META_DATA();

#endif
