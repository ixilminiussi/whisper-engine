#ifndef WSP_INPUT_ACTION
#define WSP_INPUT_ACTION

#include <wsp_inputs.hpp>

#include <tinyxml2.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <functional>
#include <initializer_list>
#include <vector>

namespace wsp
{

class InputAction
{
  public:
    InputAction() = default;
    ~InputAction() = default;

    virtual void poll(const class Window &, float dt) = 0;
    virtual void bind(std::function<void(float, int)> callbackFunction) = 0;
    virtual void bind(std::function<void(float, glm::vec2)> callbackFunction) = 0;

    virtual void unbindAll() = 0;

    virtual tinyxml2::XMLElement &save(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parentElement) = 0;
    virtual void load(tinyxml2::XMLElement *parentElement) = 0;
    virtual void editor() = 0;
};

class ButtonAction : public InputAction
{
  public:
    enum Type
    {
        PRESSED,  // sends when first pressed
        HELD,     // sends as long as toggled
        RELEASED, // sends when released
        SHORTCUT, // TODO: sends when all are held for first time
        TOGGLE,   // sends 1 when pressed, 0 when released
    };

    ButtonAction() : _buttonType{PRESSED} {};

    ButtonAction(Type buttonType, std::initializer_list<Button> buttons)
        : _buttonType{buttonType}, _buttons{buttons} {};

    void poll(const class Window &, float dt) override;
    void bind(std::function<void(float, int)> callbackFunction) override;
    void bind(std::function<void(float, glm::vec2)> callbackFunction) override;

    void unbindAll() override;

    tinyxml2::XMLElement &save(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parentElement) override;
    void load(tinyxml2::XMLElement *parentElement) override;
    void editor() override;

  private:
    std::vector<Button> _buttons;
    std::vector<std::function<void(float, int)>> _functions;

    int _status{GLFW_RELEASE};
    Type _buttonType;
};

class AxisAction : public InputAction
{
  public:
    enum Type
    {
        BUTTONS,
        AXES
    };

    AxisAction() : AxisAction{CMX_AXIS_VOID, CMX_AXIS_VOID} {};

    AxisAction(Button right, Button left, Button up = CMX_BUTTON_VOID, Button down = CMX_BUTTON_VOID)
        : _buttons{right, left, up, down}, _type{BUTTONS} {};
    AxisAction(Axis vertical, Axis horizontal = CMX_AXIS_VOID) : _axes{vertical, horizontal}, _type{AXES} {};

    void poll(const class Window &, float dt) override;
    void bind(std::function<void(float, int)> callbackFunction) override;
    void bind(std::function<void(float, glm::vec2)> callbackFunction) override;

    void unbindAll() override;

    tinyxml2::XMLElement &save(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parentElement) override;
    void load(tinyxml2::XMLElement *parentElement) override;
    void editor() override;

  private:
    Type _type;
    Button _buttons[4];
    Axis _axes[2];

    std::vector<std::function<void(float, glm::vec2)>> _functions;

    glm::vec2 _value{0.f};

    float _modifierX{1.f};
    float _modifierY{1.f};
};

} // namespace wsp

#endif
