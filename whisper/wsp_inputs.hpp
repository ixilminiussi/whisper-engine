#ifndef WSP_INPUTS
#define WSP_INPUTS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <spdlog/spdlog.h>
#include <string>

namespace wsp
{

enum InputSource
{
    GAMEPAD,
    MOUSE,
    KEYBOARD
};

inline char const *toString(InputSource inputSource)
{
    switch (inputSource)
    {
    case InputSource::GAMEPAD:
        return "GAMEPAD";
        break;
    case InputSource::MOUSE:
        return "MOUSE";
        break;
    case InputSource::KEYBOARD:
        return "KEYBOARD";
        break;
    }
    return "";
}

inline InputSource toInputSource(char const *sourceString)
{
    if (strcmp(sourceString, "GAMEPAD") == 0)
    {
        return InputSource::GAMEPAD;
    }
    if (strcmp(sourceString, "MOUSE") == 0)
    {
        return InputSource::MOUSE;
    }

    return InputSource::KEYBOARD;
}

struct Button
{
    int code{};
    InputSource source{};

    short unsigned int id = 0; // used to set apart inputs of same code but different use

    int status{0};

    void editor(std::string const &label);
};

inline bool operator==(Button const &a, Button const &b)
{
    return (a.code == b.code && a.source == b.source && a.id == b.id);
}

struct Axis
{
    int code{};
    InputSource source{};

    short unsigned int id = 0; // used to set apart inputs of same code but different use

    float value{0.f};
    float absValue{0.f};

    void editor(std::string const &label);
};

inline bool operator==(Axis const &a, Axis const &b)
{
    return (a.code == b.code && a.source == b.source && a.id == b.id);
}

inline Button CMX_BUTTON_VOID{-1, InputSource::KEYBOARD};
inline Axis CMX_AXIS_VOID{-1, InputSource::KEYBOARD};
inline Axis CMX_MOUSE_AXIS_X_ABSOLUTE{0, InputSource::MOUSE};
inline Axis CMX_MOUSE_AXIS_Y_ABSOLUTE{1, InputSource::MOUSE};
inline Axis CMX_MOUSE_AXIS_X_RELATIVE{0, InputSource::MOUSE, 1};
inline Axis CMX_MOUSE_AXIS_Y_RELATIVE{1, InputSource::MOUSE, 1};
inline Axis CMX_MOUSE_SCROLL_X_RELATIVE{2, InputSource::MOUSE, 0};
inline Axis CMX_MOUSE_SCROLL_Y_RELATIVE{2, InputSource::MOUSE, 1};

inline Button CMX_KEY_SPACE{GLFW_KEY_SPACE, InputSource::KEYBOARD};
inline Button CMX_KEY_APOSTROPHE{GLFW_KEY_APOSTROPHE, InputSource::KEYBOARD};
inline Button CMX_KEY_COMMA{GLFW_KEY_COMMA, InputSource::KEYBOARD};
inline Button CMX_KEY_MINUS{GLFW_KEY_MINUS, InputSource::KEYBOARD};
inline Button CMX_KEY_PERIOD{GLFW_KEY_PERIOD, InputSource::KEYBOARD};
inline Button CMX_KEY_SLASH{GLFW_KEY_SLASH, InputSource::KEYBOARD};
inline Button CMX_KEY_0{GLFW_KEY_0, InputSource::KEYBOARD};
inline Button CMX_KEY_1{GLFW_KEY_1, InputSource::KEYBOARD};
inline Button CMX_KEY_2{GLFW_KEY_2, InputSource::KEYBOARD};
inline Button CMX_KEY_3{GLFW_KEY_3, InputSource::KEYBOARD};
inline Button CMX_KEY_4{GLFW_KEY_4, InputSource::KEYBOARD};
inline Button CMX_KEY_5{GLFW_KEY_5, InputSource::KEYBOARD};
inline Button CMX_KEY_6{GLFW_KEY_6, InputSource::KEYBOARD};
inline Button CMX_KEY_7{GLFW_KEY_7, InputSource::KEYBOARD};
inline Button CMX_KEY_8{GLFW_KEY_8, InputSource::KEYBOARD};
inline Button CMX_KEY_9{GLFW_KEY_9, InputSource::KEYBOARD};
inline Button CMX_KEY_SEMICOLON{GLFW_KEY_SEMICOLON, InputSource::KEYBOARD};
inline Button CMX_KEY_EQUAL{GLFW_KEY_EQUAL, InputSource::KEYBOARD};
inline Button CMX_KEY_A{GLFW_KEY_A, InputSource::KEYBOARD};
inline Button CMX_KEY_B{GLFW_KEY_B, InputSource::KEYBOARD};
inline Button CMX_KEY_C{GLFW_KEY_C, InputSource::KEYBOARD};
inline Button CMX_KEY_D{GLFW_KEY_D, InputSource::KEYBOARD};
inline Button CMX_KEY_E{GLFW_KEY_E, InputSource::KEYBOARD};
inline Button CMX_KEY_F{GLFW_KEY_F, InputSource::KEYBOARD};
inline Button CMX_KEY_H{GLFW_KEY_H, InputSource::KEYBOARD};
inline Button CMX_KEY_I{GLFW_KEY_I, InputSource::KEYBOARD};
inline Button CMX_KEY_J{GLFW_KEY_J, InputSource::KEYBOARD};
inline Button CMX_KEY_K{GLFW_KEY_K, InputSource::KEYBOARD};
inline Button CMX_KEY_L{GLFW_KEY_L, InputSource::KEYBOARD};
inline Button CMX_KEY_M{GLFW_KEY_M, InputSource::KEYBOARD};
inline Button CMX_KEY_N{GLFW_KEY_N, InputSource::KEYBOARD};
inline Button CMX_KEY_O{GLFW_KEY_O, InputSource::KEYBOARD};
inline Button CMX_KEY_P{GLFW_KEY_P, InputSource::KEYBOARD};
inline Button CMX_KEY_Q{GLFW_KEY_Q, InputSource::KEYBOARD};
inline Button CMX_KEY_R{GLFW_KEY_R, InputSource::KEYBOARD};
inline Button CMX_KEY_S{GLFW_KEY_S, InputSource::KEYBOARD};
inline Button CMX_KEY_T{GLFW_KEY_T, InputSource::KEYBOARD};
inline Button CMX_KEY_U{GLFW_KEY_U, InputSource::KEYBOARD};
inline Button CMX_KEY_V{GLFW_KEY_V, InputSource::KEYBOARD};
inline Button CMX_KEY_W{GLFW_KEY_W, InputSource::KEYBOARD};
inline Button CMX_KEY_X{GLFW_KEY_X, InputSource::KEYBOARD};
inline Button CMX_KEY_Y{GLFW_KEY_Y, InputSource::KEYBOARD};
inline Button CMX_KEY_Z{GLFW_KEY_Z, InputSource::KEYBOARD};
inline Button CMX_KEY_LEFT_BRACKET{GLFW_KEY_LEFT_BRACKET, InputSource::KEYBOARD};
inline Button CMX_KEY_BACKSLASH{GLFW_KEY_BACKSLASH, InputSource::KEYBOARD};
inline Button CMX_KEY_RIGHT_BRACKET{GLFW_KEY_RIGHT_BRACKET, InputSource::KEYBOARD};
inline Button CMX_KEY_GRAVE_ACCENT{GLFW_KEY_GRAVE_ACCENT, InputSource::KEYBOARD};
inline Button CMX_KEY_WORLD_1{GLFW_KEY_WORLD_1, InputSource::KEYBOARD};
inline Button CMX_KEY_WORLD_2{GLFW_KEY_WORLD_2, InputSource::KEYBOARD};
inline Button CMX_KEY_ESCAPE{GLFW_KEY_ESCAPE, InputSource::KEYBOARD};
inline Button CMX_KEY_ENTER{GLFW_KEY_ENTER, InputSource::KEYBOARD};
inline Button CMX_KEY_TAB{GLFW_KEY_TAB, InputSource::KEYBOARD};
inline Button CMX_KEY_BACKSPACE{GLFW_KEY_BACKSPACE, InputSource::KEYBOARD};
inline Button CMX_KEY_INSERT{GLFW_KEY_INSERT, InputSource::KEYBOARD};
inline Button CMX_KEY_DELETE{GLFW_KEY_DELETE, InputSource::KEYBOARD};
inline Button CMX_KEY_RIGHT{GLFW_KEY_RIGHT, InputSource::KEYBOARD};
inline Button CMX_KEY_LEFT{GLFW_KEY_LEFT, InputSource::KEYBOARD};
inline Button CMX_KEY_DOWN{GLFW_KEY_DOWN, InputSource::KEYBOARD};
inline Button CMX_KEY_UP{GLFW_KEY_UP, InputSource::KEYBOARD};
inline Button CMX_KEY_PAGE_UP{GLFW_KEY_PAGE_UP, InputSource::KEYBOARD};
inline Button CMX_KEY_PAGE_DOWN{GLFW_KEY_PAGE_DOWN, InputSource::KEYBOARD};
inline Button CMX_KEY_HOME{GLFW_KEY_HOME, InputSource::KEYBOARD};
inline Button CMX_KEY_END{GLFW_KEY_END, InputSource::KEYBOARD};
inline Button CMX_KEY_CAPS_LOCK{GLFW_KEY_CAPS_LOCK, InputSource::KEYBOARD};
inline Button CMX_KEY_SCROLL_LOCK{GLFW_KEY_SCROLL_LOCK, InputSource::KEYBOARD};
inline Button CMX_KEY_NUM_LOCK{GLFW_KEY_NUM_LOCK, InputSource::KEYBOARD};
inline Button CMX_KEY_PRINT_SCREEN{GLFW_KEY_PRINT_SCREEN, InputSource::KEYBOARD};
inline Button CMX_KEY_PAUSE{GLFW_KEY_PAUSE, InputSource::KEYBOARD};
inline Button CMX_KEY_F1{GLFW_KEY_F1, InputSource::KEYBOARD};
inline Button CMX_KEY_F2{GLFW_KEY_F2, InputSource::KEYBOARD};
inline Button CMX_KEY_F3{GLFW_KEY_F3, InputSource::KEYBOARD};
inline Button CMX_KEY_F4{GLFW_KEY_F4, InputSource::KEYBOARD};
inline Button CMX_KEY_F5{GLFW_KEY_F5, InputSource::KEYBOARD};
inline Button CMX_KEY_F6{GLFW_KEY_F6, InputSource::KEYBOARD};
inline Button CMX_KEY_F7{GLFW_KEY_F7, InputSource::KEYBOARD};
inline Button CMX_KEY_F8{GLFW_KEY_F8, InputSource::KEYBOARD};
inline Button CMX_KEY_F9{GLFW_KEY_F9, InputSource::KEYBOARD};
inline Button CMX_KEY_F10{GLFW_KEY_F10, InputSource::KEYBOARD};
inline Button CMX_KEY_F11{GLFW_KEY_F11, InputSource::KEYBOARD};
inline Button CMX_KEY_F12{GLFW_KEY_F12, InputSource::KEYBOARD};
inline Button CMX_KEY_F13{GLFW_KEY_F13, InputSource::KEYBOARD};
inline Button CMX_KEY_F14{GLFW_KEY_F14, InputSource::KEYBOARD};
inline Button CMX_KEY_F15{GLFW_KEY_F15, InputSource::KEYBOARD};
inline Button CMX_KEY_F16{GLFW_KEY_F16, InputSource::KEYBOARD};
inline Button CMX_KEY_F17{GLFW_KEY_F17, InputSource::KEYBOARD};
inline Button CMX_KEY_F18{GLFW_KEY_F18, InputSource::KEYBOARD};
inline Button CMX_KEY_F19{GLFW_KEY_F19, InputSource::KEYBOARD};
inline Button CMX_KEY_F20{GLFW_KEY_F20, InputSource::KEYBOARD};
inline Button CMX_KEY_F21{GLFW_KEY_F21, InputSource::KEYBOARD};
inline Button CMX_KEY_F22{GLFW_KEY_F22, InputSource::KEYBOARD};
inline Button CMX_KEY_F23{GLFW_KEY_F23, InputSource::KEYBOARD};
inline Button CMX_KEY_F24{GLFW_KEY_F24, InputSource::KEYBOARD};
inline Button CMX_KEY_F25{GLFW_KEY_F25, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_0{GLFW_KEY_KP_0, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_1{GLFW_KEY_KP_1, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_2{GLFW_KEY_KP_2, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_3{GLFW_KEY_KP_3, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_4{GLFW_KEY_KP_4, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_5{GLFW_KEY_KP_5, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_6{GLFW_KEY_KP_6, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_7{GLFW_KEY_KP_7, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_8{GLFW_KEY_KP_8, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_9{GLFW_KEY_KP_9, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_DECIMAL{GLFW_KEY_KP_DECIMAL, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_DIVIDE{GLFW_KEY_KP_DIVIDE, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_MULTIPLY{GLFW_KEY_KP_MULTIPLY, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_SUBTRACT{GLFW_KEY_KP_SUBTRACT, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_ADD{GLFW_KEY_KP_ADD, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_ENTER{GLFW_KEY_KP_ENTER, InputSource::KEYBOARD};
inline Button CMX_KEY_KP_EQUAL{GLFW_KEY_KP_EQUAL, InputSource::KEYBOARD};
inline Button CMX_KEY_LEFT_SHIFT{GLFW_KEY_LEFT_SHIFT, InputSource::KEYBOARD};
inline Button CMX_KEY_LEFT_CONTROL{GLFW_KEY_LEFT_CONTROL, InputSource::KEYBOARD};
inline Button CMX_KEY_LEFT_ALT{GLFW_KEY_KP_3, InputSource::KEYBOARD};
inline Button CMX_KEY_LEFT_SUPER{GLFW_KEY_LEFT_SUPER, InputSource::KEYBOARD};
inline Button CMX_KEY_RIGHT_SHIFT{GLFW_KEY_RIGHT_SHIFT, InputSource::KEYBOARD};
inline Button CMX_KEY_RIGHT_CONTROL{GLFW_KEY_RIGHT_CONTROL, InputSource::KEYBOARD};
inline Button CMX_KEY_RIGHT_ALT{GLFW_KEY_RIGHT_ALT, InputSource::KEYBOARD};
inline Button CMX_KEY_RIGHT_SUPER{GLFW_KEY_RIGHT_SUPER, InputSource::KEYBOARD};
inline Button CMX_KEY_MENU{GLFW_KEY_MENU, InputSource::KEYBOARD};

inline Button CMX_MOUSE_BUTTON_1{GLFW_MOUSE_BUTTON_1, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_2{GLFW_MOUSE_BUTTON_2, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_3{GLFW_MOUSE_BUTTON_3, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_4{GLFW_MOUSE_BUTTON_4, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_5{GLFW_MOUSE_BUTTON_5, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_6{GLFW_MOUSE_BUTTON_6, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_7{GLFW_MOUSE_BUTTON_7, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_8{GLFW_MOUSE_BUTTON_8, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_LAST{GLFW_MOUSE_BUTTON_LAST, InputSource::MOUSE};
inline Button CMX_MOUSE_BUTTON_LEFT{GLFW_MOUSE_BUTTON_LEFT, InputSource::MOUSE, 1};
inline Button CMX_MOUSE_BUTTON_RIGHT{GLFW_MOUSE_BUTTON_RIGHT, InputSource::MOUSE, 1};
inline Button CMX_MOUSE_BUTTON_MIDDLE{GLFW_MOUSE_BUTTON_MIDDLE, InputSource::MOUSE};

inline Button CMX_GAMEPAD_BUTTON_A{GLFW_GAMEPAD_BUTTON_A, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_B{GLFW_GAMEPAD_BUTTON_B, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_X{GLFW_GAMEPAD_BUTTON_X, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_Y{GLFW_GAMEPAD_BUTTON_Y, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_CROSS{GLFW_GAMEPAD_BUTTON_CROSS, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_CIRCLE{GLFW_GAMEPAD_BUTTON_CIRCLE, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_SQUARE{GLFW_GAMEPAD_BUTTON_SQUARE, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_TRIANGLE{GLFW_GAMEPAD_BUTTON_TRIANGLE, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_LEFT_BUMPER{GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_RIGHT_BUMPER{GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_BACK{GLFW_GAMEPAD_BUTTON_BACK, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_START{GLFW_GAMEPAD_BUTTON_START, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_GUIDE{GLFW_GAMEPAD_BUTTON_GUIDE, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_LEFT_THUMB{GLFW_GAMEPAD_BUTTON_LEFT_THUMB, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_RIGHT_THUMB{GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_DPAD_UP{GLFW_GAMEPAD_BUTTON_DPAD_UP, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_DPAD_RIGHT{GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_DPAD_DOWN{GLFW_GAMEPAD_BUTTON_DPAD_DOWN, InputSource::GAMEPAD};
inline Button CMX_GAMEPAD_BUTTON_DPAD_LEFT{GLFW_GAMEPAD_BUTTON_DPAD_LEFT, InputSource::GAMEPAD};

inline static std::pair<char const *, Axis *> const CMX_AXIS_DICTIONARY[] = {
    {"AXIS_VOID", &CMX_AXIS_VOID},
    {"MOUSE_AXIS_X_ABSOLUTE", &CMX_MOUSE_AXIS_X_ABSOLUTE},
    {"MOUSE_AXIS_Y_ABSOLUTE", &CMX_MOUSE_AXIS_Y_ABSOLUTE},
    {"MOUSE_AXIS_X_RELATIVE", &CMX_MOUSE_AXIS_X_RELATIVE},
    {"MOUSE_AXIS_Y_RELATIVE", &CMX_MOUSE_AXIS_Y_RELATIVE},
    {"MOUSE_MOUSE_SCROLL_X_RELATIVE", &CMX_MOUSE_SCROLL_X_RELATIVE},
    {"MOUSE_MOUSE_SCROLL_Y_RELATIVE", &CMX_MOUSE_SCROLL_Y_RELATIVE},
};

inline static std::pair<char const *, Button *> const CMX_BUTTON_DICTIONARY[] = {
    {"BUTTON_VOID", &CMX_BUTTON_VOID},
    {"KEY_SPACE", &CMX_KEY_SPACE},
    {"KEY_APOSTROPHE", &CMX_KEY_APOSTROPHE},
    {"KEY_COMMA", &CMX_KEY_COMMA},
    {"KEY_MINUS", &CMX_KEY_MINUS},
    {"KEY_PERIOD", &CMX_KEY_PERIOD},
    {"KEY_SLASH", &CMX_KEY_SLASH},
    {"KEY_0", &CMX_KEY_0},
    {"KEY_1", &CMX_KEY_1},
    {"KEY_2", &CMX_KEY_2},
    {"KEY_3", &CMX_KEY_3},
    {"KEY_4", &CMX_KEY_4},
    {"KEY_5", &CMX_KEY_5},
    {"KEY_6", &CMX_KEY_6},
    {"KEY_7", &CMX_KEY_7},
    {"KEY_8", &CMX_KEY_8},
    {"KEY_9", &CMX_KEY_9},
    {"KEY_SEMICOLON", &CMX_KEY_SEMICOLON},
    {"KEY_EQUAL", &CMX_KEY_EQUAL},
    {"KEY_A", &CMX_KEY_A},
    {"KEY_B", &CMX_KEY_B},
    {"KEY_C", &CMX_KEY_C},
    {"KEY_D", &CMX_KEY_D},
    {"KEY_E", &CMX_KEY_E},
    {"KEY_F", &CMX_KEY_F},
    {"KEY_H", &CMX_KEY_H},
    {"KEY_I", &CMX_KEY_I},
    {"KEY_J", &CMX_KEY_J},
    {"KEY_K", &CMX_KEY_K},
    {"KEY_L", &CMX_KEY_L},
    {"KEY_M", &CMX_KEY_M},
    {"KEY_N", &CMX_KEY_N},
    {"KEY_O", &CMX_KEY_O},
    {"KEY_P", &CMX_KEY_P},
    {"KEY_Q", &CMX_KEY_Q},
    {"KEY_R", &CMX_KEY_R},
    {"KEY_S", &CMX_KEY_S},
    {"KEY_T", &CMX_KEY_T},
    {"KEY_U", &CMX_KEY_U},
    {"KEY_V", &CMX_KEY_V},
    {"KEY_W", &CMX_KEY_W},
    {"KEY_X", &CMX_KEY_X},
    {"KEY_Y", &CMX_KEY_Y},
    {"KEY_Z", &CMX_KEY_Z},
    {"KEY_LEFT_BRACKET", &CMX_KEY_LEFT_BRACKET},
    {"KEY_BACKSLASH", &CMX_KEY_BACKSLASH},
    {"KEY_RIGHT_BRACKET", &CMX_KEY_RIGHT_BRACKET},
    {"KEY_GRAVE_ACCENT", &CMX_KEY_GRAVE_ACCENT},
    {"KEY_WORLD_1", &CMX_KEY_WORLD_1},
    {"KEY_WORLD_2", &CMX_KEY_WORLD_2},
    {"KEY_ESCAPE", &CMX_KEY_ESCAPE},
    {"KEY_ENTER", &CMX_KEY_ENTER},
    {"KEY_TAB", &CMX_KEY_TAB},
    {"KEY_BACKSPACE", &CMX_KEY_BACKSPACE},
    {"KEY_INSERT", &CMX_KEY_INSERT},
    {"KEY_DELETE", &CMX_KEY_DELETE},
    {"KEY_RIGHT", &CMX_KEY_RIGHT},
    {"KEY_LEFT", &CMX_KEY_LEFT},
    {"KEY_DOWN", &CMX_KEY_DOWN},
    {"KEY_UP", &CMX_KEY_UP},
    {"KEY_PAGE_UP", &CMX_KEY_PAGE_UP},
    {"KEY_PAGE_DOWN", &CMX_KEY_PAGE_DOWN},
    {"KEY_HOME", &CMX_KEY_HOME},
    {"KEY_END", &CMX_KEY_END},
    {"KEY_CAPS_LOCK", &CMX_KEY_CAPS_LOCK},
    {"KEY_SCROLL_LOCK", &CMX_KEY_SCROLL_LOCK},
    {"KEY_NUM_LOCK", &CMX_KEY_NUM_LOCK},
    {"KEY_PRINT_SCREEN", &CMX_KEY_PRINT_SCREEN},
    {"KEY_PAUSE", &CMX_KEY_PAUSE},
    {"KEY_F1", &CMX_KEY_F1},
    {"KEY_F2", &CMX_KEY_F2},
    {"KEY_F3", &CMX_KEY_F3},
    {"KEY_F4", &CMX_KEY_F4},
    {"KEY_F5", &CMX_KEY_F5},
    {"KEY_F6", &CMX_KEY_F6},
    {"KEY_F7", &CMX_KEY_F7},
    {"KEY_F8", &CMX_KEY_F8},
    {"KEY_F9", &CMX_KEY_F9},
    {"KEY_F10", &CMX_KEY_F10},
    {"KEY_F11", &CMX_KEY_F11},
    {"KEY_F12", &CMX_KEY_F12},
    {"KEY_F13", &CMX_KEY_F13},
    {"KEY_F14", &CMX_KEY_F14},
    {"KEY_F15", &CMX_KEY_F15},
    {"KEY_F16", &CMX_KEY_F16},
    {"KEY_F17", &CMX_KEY_F17},
    {"KEY_F18", &CMX_KEY_F18},
    {"KEY_F19", &CMX_KEY_F19},
    {"KEY_F20", &CMX_KEY_F20},
    {"KEY_F21", &CMX_KEY_F21},
    {"KEY_F22", &CMX_KEY_F22},
    {"KEY_F23", &CMX_KEY_F23},
    {"KEY_F24", &CMX_KEY_F24},
    {"KEY_F25", &CMX_KEY_F25},
    {"KEY_KP_0", &CMX_KEY_KP_0},
    {"KEY_KP_1", &CMX_KEY_KP_1},
    {"KEY_KP_2", &CMX_KEY_KP_2},
    {"KEY_KP_3", &CMX_KEY_KP_3},
    {"KEY_KP_4", &CMX_KEY_KP_4},
    {"KEY_KP_5", &CMX_KEY_KP_5},
    {"KEY_KP_6", &CMX_KEY_KP_6},
    {"KEY_KP_7", &CMX_KEY_KP_7},
    {"KEY_KP_8", &CMX_KEY_KP_8},
    {"KEY_KP_9", &CMX_KEY_KP_9},
    {"KEY_KP_DECIMAL", &CMX_KEY_KP_DECIMAL},
    {"KEY_KP_DIVIDE", &CMX_KEY_KP_DIVIDE},
    {"KEY_KP_MULTIPLY", &CMX_KEY_KP_MULTIPLY},
    {"KEY_KP_SUBTRACT", &CMX_KEY_KP_SUBTRACT},
    {"KEY_KP_ADD", &CMX_KEY_KP_ADD},
    {"KEY_KP_ENTER", &CMX_KEY_KP_ENTER},
    {"KEY_KP_EQUAL", &CMX_KEY_KP_EQUAL},
    {"KEY_LEFT_SHIFT", &CMX_KEY_LEFT_SHIFT},
    {"KEY_LEFT_CONTROL", &CMX_KEY_LEFT_CONTROL},
    {"KEY_LEFT_ALT", &CMX_KEY_LEFT_ALT},
    {"KEY_LEFT_SUPER", &CMX_KEY_LEFT_SUPER},
    {"KEY_RIGHT_SHIFT", &CMX_KEY_RIGHT_SHIFT},
    {"KEY_RIGHT_CONTROL", &CMX_KEY_RIGHT_CONTROL},
    {"KEY_RIGHT_ALT", &CMX_KEY_RIGHT_ALT},
    {"KEY_RIGHT_SUPER", &CMX_KEY_RIGHT_SUPER},
    {"KEY_MENU", &CMX_KEY_MENU},
    {"MOUSE_BUTTON_1", &CMX_MOUSE_BUTTON_1},
    {"MOUSE_BUTTON_2", &CMX_MOUSE_BUTTON_2},
    {"MOUSE_BUTTON_3", &CMX_MOUSE_BUTTON_3},
    {"MOUSE_BUTTON_4", &CMX_MOUSE_BUTTON_4},
    {"MOUSE_BUTTON_5", &CMX_MOUSE_BUTTON_5},
    {"MOUSE_BUTTON_6", &CMX_MOUSE_BUTTON_6},
    {"MOUSE_BUTTON_7", &CMX_MOUSE_BUTTON_7},
    {"MOUSE_BUTTON_8", &CMX_MOUSE_BUTTON_8},
    {"MOUSE_BUTTON_LAST", &CMX_MOUSE_BUTTON_LAST},
    {"MOUSE_BUTTON_LEFT", &CMX_MOUSE_BUTTON_LEFT},
    {"MOUSE_BUTTON_RIGHT", &CMX_MOUSE_BUTTON_RIGHT},
    {"MOUSE_BUTTON_MIDDLE", &CMX_MOUSE_BUTTON_MIDDLE},
    {"GAMEPAD_BUTTON_A", &CMX_GAMEPAD_BUTTON_A},
    {"GAMEPAD_BUTTON_B", &CMX_GAMEPAD_BUTTON_B},
    {"GAMEPAD_BUTTON_X", &CMX_GAMEPAD_BUTTON_X},
    {"GAMEPAD_BUTTON_Y", &CMX_GAMEPAD_BUTTON_Y},
    {"GAMEPAD_BUTTON_CROSS", &CMX_GAMEPAD_BUTTON_CROSS},
    {"GAMEPAD_BUTTON_CIRCLE", &CMX_GAMEPAD_BUTTON_CIRCLE},
    {"GAMEPAD_BUTTON_SQUARE", &CMX_GAMEPAD_BUTTON_SQUARE},
    {"GAMEPAD_BUTTON_TRIANGLE", &CMX_GAMEPAD_BUTTON_TRIANGLE},
    {"GAMEPAD_BUTTON_LEFT_BUMPER", &CMX_GAMEPAD_BUTTON_LEFT_BUMPER},
    {"GAMEPAD_BUTTON_RIGHT_BUMPER", &CMX_GAMEPAD_BUTTON_RIGHT_BUMPER},
    {"GAMEPAD_BUTTON_BACK", &CMX_GAMEPAD_BUTTON_BACK},
    {"GAMEPAD_BUTTON_START", &CMX_GAMEPAD_BUTTON_START},
    {"GAMEPAD_BUTTON_GUIDE", &CMX_GAMEPAD_BUTTON_GUIDE},
    {"GAMEPAD_BUTTON_LEFT_THUMB", &CMX_GAMEPAD_BUTTON_LEFT_THUMB},
    {"GAMEPAD_BUTTON_RIGHT_THUMB", &CMX_GAMEPAD_BUTTON_RIGHT_THUMB},
    {"GAMEPAD_BUTTON_DPAD_UP", &CMX_GAMEPAD_BUTTON_DPAD_UP},
    {"GAMEPAD_BUTTON_DPAD_RIGHT", &CMX_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"GAMEPAD_BUTTON_DPAD_DOWN", &CMX_GAMEPAD_BUTTON_DPAD_DOWN},
    {"GAMEPAD_BUTTON_DPAD_LEFT", &CMX_GAMEPAD_BUTTON_DPAD_LEFT}};

} // namespace wsp

#endif
