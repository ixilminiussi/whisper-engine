#ifndef WSP_INPUT_ACTION
#define WSP_INPUT_ACTION

#include <wsp_devkit.hpp>
#include <wsp_inputs.hpp>

#include <tinyxml2.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <functional>
#include <initializer_list>
#include <vector>

#include <.generated/wsp_input_action.generated.hpp>

struct GLFWwindow;

namespace wsp
{

class InputAction
{
  public:
    InputAction() = default;
    ~InputAction() = default;

    virtual void Poll(GLFWwindow *, double dt) = 0;
    virtual void Bind(std::function<void(float, int)> callbackFunction) = 0;
    virtual void Bind(std::function<void(float, glm::vec2)> callbackFunction) = 0;

    virtual void UnbindAll() = 0;
};

WCLASS()
class ButtonAction : public InputAction
{
  public:
    WENUM()
    enum Usage
    {
        ePressed,  // sends when first pressed
        eHeld,     // sends as long as toggled
        eReleased, // sends when released
        eShortcut, // TODO: sends when all are held for first time
        eToggle,   // sends 1 when pressed, 0 when released
    };

    ButtonAction() : _buttonUsage{ePressed} {};

    ButtonAction(Usage buttonUsage, std::initializer_list<Button> buttons)
        : _buttonUsage{buttonUsage}, _buttons{buttons} {};

    void Poll(GLFWwindow *, double dt) override;
    void Bind(std::function<void(float, int)> callbackFunction) override;
    void Bind(std::function<void(float, glm::vec2)> callbackFunction) override;

    void UnbindAll() override;

    WCLASS_BODY$ButtonAction();

  private:
    WPROPERTY()
    Usage _buttonUsage;
    WPROPERTY()
    std::vector<Button> _buttons;

    std::vector<std::function<void(float, int)>> _functions;
    int _status{GLFW_RELEASE};
};

WCLASS()
class AxisAction : public InputAction
{
  public:
    WENUM()
    enum Usage
    {
        eButtons,
        eAxes
    };

    AxisAction() : AxisAction{WSP_AXIS_VOID, WSP_AXIS_VOID} {};

    AxisAction(Button right, Button left, Button up = WSP_BUTTON_VOID, Button down = WSP_BUTTON_VOID)
        : _buttons{right, left, up, down}, _axisUsage{eButtons} {};
    AxisAction(Axis vertical, Axis horizontal = WSP_AXIS_VOID) : _axes{vertical, horizontal}, _axisUsage{eAxes} {};

    void Poll(GLFWwindow *, double dt) override;
    void Bind(std::function<void(float, int)> callbackFunction) override;
    void Bind(std::function<void(float, glm::vec2)> callbackFunction) override;

    void UnbindAll() override;

    WCLASS_BODY$AxisAction();

  private:
    WPROPERTY()
    Usage _axisUsage;
    WPROPERTY()
    std::array<Button, 4> _buttons;
    WPROPERTY()
    std::array<Axis, 2> _axes;
    WPROPERTY()
    float _modifierX{1.f};
    WPROPERTY()
    float _modifierY{1.f};

    std::vector<std::function<void(float, glm::vec2)>> _functions;
    glm::vec2 _value{0.f};
};

} // namespace wsp

WGENERATED_META_DATA()

#endif
