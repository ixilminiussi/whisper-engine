#ifndef WSP_INPUT_MANAGER
#define WSP_INPUT_MANAGER

#include <wsp_input_action.hpp>

#include <spdlog/spdlog.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <unordered_map>

namespace wsp
{

class InputManager
{
  public:
    InputManager(class Window &,
                 std::string const &filepath = std::string(GAME_FILES) + std::string("properties/input-manager.xml"));
    ~InputManager();

    void save();
    void saveAs(std::string const &);
    void load();

    template <typename T>
    void bindAxis(std::string const &name, void (T::*callbackFunction)(float, glm::vec2), T *instance);
    template <typename T>
    void bindButton(std::string const &name, void (T::*callbackFunction)(float, int), T *instance);
    void unbindAll();
    void addInput(std::string const &name, class InputAction *);
    void pollEvents(float dt);
    static void setMouseCapture(bool, bool force = false);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    static void onScrollStop();
    static double scrollX, scrollY;
    static bool isScrolling;

    void editor();

  private:
    std::string const _filepath;
    bool _gamepadDetected{false};
    std::unordered_map<std::string, class InputAction *> _inputDictionary;
    class Window &_window;
};

template <typename T>
inline void InputManager::bindAxis(std::string const &name, void (T::*callbackFunction)(float, glm::vec2), T *instance)
{
    auto mappedInput = _inputDictionary.find(name);
    if (mappedInput == _inputDictionary.end())
    {
        spdlog::warn("InputManager: attempt at binding non existant input [{0}]", name);
        return;
    }
    else
    {
        mappedInput->second->bind([instance, callbackFunction](float dt, glm::vec2 value) {
            if (instance)
                (instance->*callbackFunction)(dt, value);
            else
                spdlog::warn("InputManager: action being called on deleted object");
        });
        spdlog::info("InputManager: [{0}] bound to new function", name);
    }
}

template <typename T>
inline void InputManager::bindButton(std::string const &name, void (T::*callbackFunction)(float, int), T *instance)
{
    auto mappedInput = _inputDictionary.find(name);
    if (mappedInput == _inputDictionary.end())
    {
        spdlog::warn("InputManager: attempt at binding non existant input [{0}]", name);
        return;
    }
    else
    {
        mappedInput->second->bind([instance, callbackFunction](float dt, int val) {
            if (instance)
                (instance->*callbackFunction)(dt, val);
            else
                spdlog::warn("InputManager: action being called on deleted object");
        });
        spdlog::info("InputManager: [{0}] bound to new function", name);
    }
}

} // namespace wsp

#endif
