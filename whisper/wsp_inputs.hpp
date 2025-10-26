#ifndef WSP_INPUTS
#define WSP_INPUTS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <wsp_devkit.hpp>

#include <cstring>
#include <spdlog/spdlog.h>
#include <string>

namespace wsp
{

enum InputSource
{
    eGamepad,
    eMouse,
    eKeyboard
};

inline char const *ToString(InputSource inputSource)
{
    switch (inputSource)
    {
    case InputSource::eGamepad:
        return "GAMEPAD";
        break;
    case InputSource::eMouse:
        return "MOUSE";
        break;
    case InputSource::eKeyboard:
        return "KEYBOARD";
        break;
    }
    return "";
}

inline InputSource ToInputSource(char const *sourceString)
{
    if (strcmp(sourceString, "GAMEPAD") == 0)
    {
        return InputSource::eGamepad;
    }
    if (strcmp(sourceString, "MOUSE") == 0)
    {
        return InputSource::eMouse;
    }

    return InputSource::eKeyboard;
}

WCLASS()
struct Button
{
    int code{};
    InputSource source{};

    short unsigned int id = 0; // used to set apart inputs of same code but different use

    int status{0};
};

inline bool operator==(Button const &a, Button const &b)
{
    return (a.code == b.code && a.source == b.source && a.id == b.id);
}

WCLASS()
struct Axis
{
    int code{};
    InputSource source{};

    short unsigned int id = 0; // used to set apart inputs of same code but different use

    float value{0.f};
    float absValue{0.f};
};

inline bool operator==(Axis const &a, Axis const &b)
{
    return (a.code == b.code && a.source == b.source && a.id == b.id);
}

inline Button WSP_BUTTON_VOID{-1, InputSource::eKeyboard};
inline Axis WSP_AXIS_VOID{-1, InputSource::eKeyboard};
inline Axis WSP_MOUSE_AXIS_X_ABSOLUTE{0, InputSource::eMouse};
inline Axis WSP_MOUSE_AXIS_Y_ABSOLUTE{1, InputSource::eMouse};
inline Axis WSP_MOUSE_AXIS_X_RELATIVE{0, InputSource::eMouse, 1};
inline Axis WSP_MOUSE_AXIS_Y_RELATIVE{1, InputSource::eMouse, 1};
inline Axis WSP_MOUSE_SCROLL_X_RELATIVE{2, InputSource::eMouse, 0};
inline Axis WSP_MOUSE_SCROLL_Y_RELATIVE{2, InputSource::eMouse, 1};

inline Button WSP_KEY_SPACE{GLFW_KEY_SPACE, InputSource::eKeyboard};
inline Button WSP_KEY_APOSTROPHE{GLFW_KEY_APOSTROPHE, InputSource::eKeyboard};
inline Button WSP_KEY_COMMA{GLFW_KEY_COMMA, InputSource::eKeyboard};
inline Button WSP_KEY_MINUS{GLFW_KEY_MINUS, InputSource::eKeyboard};
inline Button WSP_KEY_PERIOD{GLFW_KEY_PERIOD, InputSource::eKeyboard};
inline Button WSP_KEY_SLASH{GLFW_KEY_SLASH, InputSource::eKeyboard};
inline Button WSP_KEY_0{GLFW_KEY_0, InputSource::eKeyboard};
inline Button WSP_KEY_1{GLFW_KEY_1, InputSource::eKeyboard};
inline Button WSP_KEY_2{GLFW_KEY_2, InputSource::eKeyboard};
inline Button WSP_KEY_3{GLFW_KEY_3, InputSource::eKeyboard};
inline Button WSP_KEY_4{GLFW_KEY_4, InputSource::eKeyboard};
inline Button WSP_KEY_5{GLFW_KEY_5, InputSource::eKeyboard};
inline Button WSP_KEY_6{GLFW_KEY_6, InputSource::eKeyboard};
inline Button WSP_KEY_7{GLFW_KEY_7, InputSource::eKeyboard};
inline Button WSP_KEY_8{GLFW_KEY_8, InputSource::eKeyboard};
inline Button WSP_KEY_9{GLFW_KEY_9, InputSource::eKeyboard};
inline Button WSP_KEY_SEMICOLON{GLFW_KEY_SEMICOLON, InputSource::eKeyboard};
inline Button WSP_KEY_EQUAL{GLFW_KEY_EQUAL, InputSource::eKeyboard};
inline Button WSP_KEY_A{GLFW_KEY_A, InputSource::eKeyboard};
inline Button WSP_KEY_B{GLFW_KEY_B, InputSource::eKeyboard};
inline Button WSP_KEY_C{GLFW_KEY_C, InputSource::eKeyboard};
inline Button WSP_KEY_D{GLFW_KEY_D, InputSource::eKeyboard};
inline Button WSP_KEY_E{GLFW_KEY_E, InputSource::eKeyboard};
inline Button WSP_KEY_F{GLFW_KEY_F, InputSource::eKeyboard};
inline Button WSP_KEY_H{GLFW_KEY_H, InputSource::eKeyboard};
inline Button WSP_KEY_I{GLFW_KEY_I, InputSource::eKeyboard};
inline Button WSP_KEY_J{GLFW_KEY_J, InputSource::eKeyboard};
inline Button WSP_KEY_K{GLFW_KEY_K, InputSource::eKeyboard};
inline Button WSP_KEY_L{GLFW_KEY_L, InputSource::eKeyboard};
inline Button WSP_KEY_M{GLFW_KEY_M, InputSource::eKeyboard};
inline Button WSP_KEY_N{GLFW_KEY_N, InputSource::eKeyboard};
inline Button WSP_KEY_O{GLFW_KEY_O, InputSource::eKeyboard};
inline Button WSP_KEY_P{GLFW_KEY_P, InputSource::eKeyboard};
inline Button WSP_KEY_Q{GLFW_KEY_Q, InputSource::eKeyboard};
inline Button WSP_KEY_R{GLFW_KEY_R, InputSource::eKeyboard};
inline Button WSP_KEY_S{GLFW_KEY_S, InputSource::eKeyboard};
inline Button WSP_KEY_T{GLFW_KEY_T, InputSource::eKeyboard};
inline Button WSP_KEY_U{GLFW_KEY_U, InputSource::eKeyboard};
inline Button WSP_KEY_V{GLFW_KEY_V, InputSource::eKeyboard};
inline Button WSP_KEY_W{GLFW_KEY_W, InputSource::eKeyboard};
inline Button WSP_KEY_X{GLFW_KEY_X, InputSource::eKeyboard};
inline Button WSP_KEY_Y{GLFW_KEY_Y, InputSource::eKeyboard};
inline Button WSP_KEY_Z{GLFW_KEY_Z, InputSource::eKeyboard};
inline Button WSP_KEY_LEFT_BRACKET{GLFW_KEY_LEFT_BRACKET, InputSource::eKeyboard};
inline Button WSP_KEY_BACKSLASH{GLFW_KEY_BACKSLASH, InputSource::eKeyboard};
inline Button WSP_KEY_RIGHT_BRACKET{GLFW_KEY_RIGHT_BRACKET, InputSource::eKeyboard};
inline Button WSP_KEY_GRAVE_ACCENT{GLFW_KEY_GRAVE_ACCENT, InputSource::eKeyboard};
inline Button WSP_KEY_WORLD_1{GLFW_KEY_WORLD_1, InputSource::eKeyboard};
inline Button WSP_KEY_WORLD_2{GLFW_KEY_WORLD_2, InputSource::eKeyboard};
inline Button WSP_KEY_ESCAPE{GLFW_KEY_ESCAPE, InputSource::eKeyboard};
inline Button WSP_KEY_ENTER{GLFW_KEY_ENTER, InputSource::eKeyboard};
inline Button WSP_KEY_TAB{GLFW_KEY_TAB, InputSource::eKeyboard};
inline Button WSP_KEY_BACKSPACE{GLFW_KEY_BACKSPACE, InputSource::eKeyboard};
inline Button WSP_KEY_INSERT{GLFW_KEY_INSERT, InputSource::eKeyboard};
inline Button WSP_KEY_DELETE{GLFW_KEY_DELETE, InputSource::eKeyboard};
inline Button WSP_KEY_RIGHT{GLFW_KEY_RIGHT, InputSource::eKeyboard};
inline Button WSP_KEY_LEFT{GLFW_KEY_LEFT, InputSource::eKeyboard};
inline Button WSP_KEY_DOWN{GLFW_KEY_DOWN, InputSource::eKeyboard};
inline Button WSP_KEY_UP{GLFW_KEY_UP, InputSource::eKeyboard};
inline Button WSP_KEY_PAGE_UP{GLFW_KEY_PAGE_UP, InputSource::eKeyboard};
inline Button WSP_KEY_PAGE_DOWN{GLFW_KEY_PAGE_DOWN, InputSource::eKeyboard};
inline Button WSP_KEY_HOME{GLFW_KEY_HOME, InputSource::eKeyboard};
inline Button WSP_KEY_END{GLFW_KEY_END, InputSource::eKeyboard};
inline Button WSP_KEY_CAPS_LOCK{GLFW_KEY_CAPS_LOCK, InputSource::eKeyboard};
inline Button WSP_KEY_SCROLL_LOCK{GLFW_KEY_SCROLL_LOCK, InputSource::eKeyboard};
inline Button WSP_KEY_NUM_LOCK{GLFW_KEY_NUM_LOCK, InputSource::eKeyboard};
inline Button WSP_KEY_PRINT_SCREEN{GLFW_KEY_PRINT_SCREEN, InputSource::eKeyboard};
inline Button WSP_KEY_PAUSE{GLFW_KEY_PAUSE, InputSource::eKeyboard};
inline Button WSP_KEY_F1{GLFW_KEY_F1, InputSource::eKeyboard};
inline Button WSP_KEY_F2{GLFW_KEY_F2, InputSource::eKeyboard};
inline Button WSP_KEY_F3{GLFW_KEY_F3, InputSource::eKeyboard};
inline Button WSP_KEY_F4{GLFW_KEY_F4, InputSource::eKeyboard};
inline Button WSP_KEY_F5{GLFW_KEY_F5, InputSource::eKeyboard};
inline Button WSP_KEY_F6{GLFW_KEY_F6, InputSource::eKeyboard};
inline Button WSP_KEY_F7{GLFW_KEY_F7, InputSource::eKeyboard};
inline Button WSP_KEY_F8{GLFW_KEY_F8, InputSource::eKeyboard};
inline Button WSP_KEY_F9{GLFW_KEY_F9, InputSource::eKeyboard};
inline Button WSP_KEY_F10{GLFW_KEY_F10, InputSource::eKeyboard};
inline Button WSP_KEY_F11{GLFW_KEY_F11, InputSource::eKeyboard};
inline Button WSP_KEY_F12{GLFW_KEY_F12, InputSource::eKeyboard};
inline Button WSP_KEY_F13{GLFW_KEY_F13, InputSource::eKeyboard};
inline Button WSP_KEY_F14{GLFW_KEY_F14, InputSource::eKeyboard};
inline Button WSP_KEY_F15{GLFW_KEY_F15, InputSource::eKeyboard};
inline Button WSP_KEY_F16{GLFW_KEY_F16, InputSource::eKeyboard};
inline Button WSP_KEY_F17{GLFW_KEY_F17, InputSource::eKeyboard};
inline Button WSP_KEY_F18{GLFW_KEY_F18, InputSource::eKeyboard};
inline Button WSP_KEY_F19{GLFW_KEY_F19, InputSource::eKeyboard};
inline Button WSP_KEY_F20{GLFW_KEY_F20, InputSource::eKeyboard};
inline Button WSP_KEY_F21{GLFW_KEY_F21, InputSource::eKeyboard};
inline Button WSP_KEY_F22{GLFW_KEY_F22, InputSource::eKeyboard};
inline Button WSP_KEY_F23{GLFW_KEY_F23, InputSource::eKeyboard};
inline Button WSP_KEY_F24{GLFW_KEY_F24, InputSource::eKeyboard};
inline Button WSP_KEY_F25{GLFW_KEY_F25, InputSource::eKeyboard};
inline Button WSP_KEY_KP_0{GLFW_KEY_KP_0, InputSource::eKeyboard};
inline Button WSP_KEY_KP_1{GLFW_KEY_KP_1, InputSource::eKeyboard};
inline Button WSP_KEY_KP_2{GLFW_KEY_KP_2, InputSource::eKeyboard};
inline Button WSP_KEY_KP_3{GLFW_KEY_KP_3, InputSource::eKeyboard};
inline Button WSP_KEY_KP_4{GLFW_KEY_KP_4, InputSource::eKeyboard};
inline Button WSP_KEY_KP_5{GLFW_KEY_KP_5, InputSource::eKeyboard};
inline Button WSP_KEY_KP_6{GLFW_KEY_KP_6, InputSource::eKeyboard};
inline Button WSP_KEY_KP_7{GLFW_KEY_KP_7, InputSource::eKeyboard};
inline Button WSP_KEY_KP_8{GLFW_KEY_KP_8, InputSource::eKeyboard};
inline Button WSP_KEY_KP_9{GLFW_KEY_KP_9, InputSource::eKeyboard};
inline Button WSP_KEY_KP_DECIMAL{GLFW_KEY_KP_DECIMAL, InputSource::eKeyboard};
inline Button WSP_KEY_KP_DIVIDE{GLFW_KEY_KP_DIVIDE, InputSource::eKeyboard};
inline Button WSP_KEY_KP_MULTIPLY{GLFW_KEY_KP_MULTIPLY, InputSource::eKeyboard};
inline Button WSP_KEY_KP_SUBTRACT{GLFW_KEY_KP_SUBTRACT, InputSource::eKeyboard};
inline Button WSP_KEY_KP_ADD{GLFW_KEY_KP_ADD, InputSource::eKeyboard};
inline Button WSP_KEY_KP_ENTER{GLFW_KEY_KP_ENTER, InputSource::eKeyboard};
inline Button WSP_KEY_KP_EQUAL{GLFW_KEY_KP_EQUAL, InputSource::eKeyboard};
inline Button WSP_KEY_LEFT_SHIFT{GLFW_KEY_LEFT_SHIFT, InputSource::eKeyboard};
inline Button WSP_KEY_LEFT_CONTROL{GLFW_KEY_LEFT_CONTROL, InputSource::eKeyboard};
inline Button WSP_KEY_LEFT_ALT{GLFW_KEY_KP_3, InputSource::eKeyboard};
inline Button WSP_KEY_LEFT_SUPER{GLFW_KEY_LEFT_SUPER, InputSource::eKeyboard};
inline Button WSP_KEY_RIGHT_SHIFT{GLFW_KEY_RIGHT_SHIFT, InputSource::eKeyboard};
inline Button WSP_KEY_RIGHT_CONTROL{GLFW_KEY_RIGHT_CONTROL, InputSource::eKeyboard};
inline Button WSP_KEY_RIGHT_ALT{GLFW_KEY_RIGHT_ALT, InputSource::eKeyboard};
inline Button WSP_KEY_RIGHT_SUPER{GLFW_KEY_RIGHT_SUPER, InputSource::eKeyboard};
inline Button WSP_KEY_MENU{GLFW_KEY_MENU, InputSource::eKeyboard};

inline Button WSP_MOUSE_BUTTON_1{GLFW_MOUSE_BUTTON_1, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_2{GLFW_MOUSE_BUTTON_2, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_3{GLFW_MOUSE_BUTTON_3, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_4{GLFW_MOUSE_BUTTON_4, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_5{GLFW_MOUSE_BUTTON_5, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_6{GLFW_MOUSE_BUTTON_6, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_7{GLFW_MOUSE_BUTTON_7, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_8{GLFW_MOUSE_BUTTON_8, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_LAST{GLFW_MOUSE_BUTTON_LAST, InputSource::eMouse};
inline Button WSP_MOUSE_BUTTON_LEFT{GLFW_MOUSE_BUTTON_LEFT, InputSource::eMouse, 1};
inline Button WSP_MOUSE_BUTTON_RIGHT{GLFW_MOUSE_BUTTON_RIGHT, InputSource::eMouse, 1};
inline Button WSP_MOUSE_BUTTON_MIDDLE{GLFW_MOUSE_BUTTON_MIDDLE, InputSource::eMouse};

inline Button WSP_GAMEPAD_BUTTON_A{GLFW_GAMEPAD_BUTTON_A, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_B{GLFW_GAMEPAD_BUTTON_B, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_X{GLFW_GAMEPAD_BUTTON_X, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_Y{GLFW_GAMEPAD_BUTTON_Y, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_CROSS{GLFW_GAMEPAD_BUTTON_CROSS, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_CIRCLE{GLFW_GAMEPAD_BUTTON_CIRCLE, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_SQUARE{GLFW_GAMEPAD_BUTTON_SQUARE, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_TRIANGLE{GLFW_GAMEPAD_BUTTON_TRIANGLE, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_LEFT_BUMPER{GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_RIGHT_BUMPER{GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_BACK{GLFW_GAMEPAD_BUTTON_BACK, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_START{GLFW_GAMEPAD_BUTTON_START, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_GUIDE{GLFW_GAMEPAD_BUTTON_GUIDE, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_LEFT_THUMB{GLFW_GAMEPAD_BUTTON_LEFT_THUMB, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_RIGHT_THUMB{GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_DPAD_UP{GLFW_GAMEPAD_BUTTON_DPAD_UP, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_DPAD_RIGHT{GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_DPAD_DOWN{GLFW_GAMEPAD_BUTTON_DPAD_DOWN, InputSource::eGamepad};
inline Button WSP_GAMEPAD_BUTTON_DPAD_LEFT{GLFW_GAMEPAD_BUTTON_DPAD_LEFT, InputSource::eGamepad};

inline static std::pair<char const *, Axis *> const WSP_AXIS_DICTIONARY[] = {
    {"AXIS_VOID", &WSP_AXIS_VOID},
    {"MOUSE_AXIS_X_ABSOLUTE", &WSP_MOUSE_AXIS_X_ABSOLUTE},
    {"MOUSE_AXIS_Y_ABSOLUTE", &WSP_MOUSE_AXIS_Y_ABSOLUTE},
    {"MOUSE_AXIS_X_RELATIVE", &WSP_MOUSE_AXIS_X_RELATIVE},
    {"MOUSE_AXIS_Y_RELATIVE", &WSP_MOUSE_AXIS_Y_RELATIVE},
    {"MOUSE_MOUSE_SCROLL_X_RELATIVE", &WSP_MOUSE_SCROLL_X_RELATIVE},
    {"MOUSE_MOUSE_SCROLL_Y_RELATIVE", &WSP_MOUSE_SCROLL_Y_RELATIVE},
};

inline static std::pair<char const *, Button *> const WSP_BUTTON_DICTIONARY[] = {
    {"BUTTON_VOID", &WSP_BUTTON_VOID},
    {"KEY_SPACE", &WSP_KEY_SPACE},
    {"KEY_APOSTROPHE", &WSP_KEY_APOSTROPHE},
    {"KEY_COMMA", &WSP_KEY_COMMA},
    {"KEY_MINUS", &WSP_KEY_MINUS},
    {"KEY_PERIOD", &WSP_KEY_PERIOD},
    {"KEY_SLASH", &WSP_KEY_SLASH},
    {"KEY_0", &WSP_KEY_0},
    {"KEY_1", &WSP_KEY_1},
    {"KEY_2", &WSP_KEY_2},
    {"KEY_3", &WSP_KEY_3},
    {"KEY_4", &WSP_KEY_4},
    {"KEY_5", &WSP_KEY_5},
    {"KEY_6", &WSP_KEY_6},
    {"KEY_7", &WSP_KEY_7},
    {"KEY_8", &WSP_KEY_8},
    {"KEY_9", &WSP_KEY_9},
    {"KEY_SEMICOLON", &WSP_KEY_SEMICOLON},
    {"KEY_EQUAL", &WSP_KEY_EQUAL},
    {"KEY_A", &WSP_KEY_A},
    {"KEY_B", &WSP_KEY_B},
    {"KEY_C", &WSP_KEY_C},
    {"KEY_D", &WSP_KEY_D},
    {"KEY_E", &WSP_KEY_E},
    {"KEY_F", &WSP_KEY_F},
    {"KEY_H", &WSP_KEY_H},
    {"KEY_I", &WSP_KEY_I},
    {"KEY_J", &WSP_KEY_J},
    {"KEY_K", &WSP_KEY_K},
    {"KEY_L", &WSP_KEY_L},
    {"KEY_M", &WSP_KEY_M},
    {"KEY_N", &WSP_KEY_N},
    {"KEY_O", &WSP_KEY_O},
    {"KEY_P", &WSP_KEY_P},
    {"KEY_Q", &WSP_KEY_Q},
    {"KEY_R", &WSP_KEY_R},
    {"KEY_S", &WSP_KEY_S},
    {"KEY_T", &WSP_KEY_T},
    {"KEY_U", &WSP_KEY_U},
    {"KEY_V", &WSP_KEY_V},
    {"KEY_W", &WSP_KEY_W},
    {"KEY_X", &WSP_KEY_X},
    {"KEY_Y", &WSP_KEY_Y},
    {"KEY_Z", &WSP_KEY_Z},
    {"KEY_LEFT_BRACKET", &WSP_KEY_LEFT_BRACKET},
    {"KEY_BACKSLASH", &WSP_KEY_BACKSLASH},
    {"KEY_RIGHT_BRACKET", &WSP_KEY_RIGHT_BRACKET},
    {"KEY_GRAVE_ACCENT", &WSP_KEY_GRAVE_ACCENT},
    {"KEY_WORLD_1", &WSP_KEY_WORLD_1},
    {"KEY_WORLD_2", &WSP_KEY_WORLD_2},
    {"KEY_ESCAPE", &WSP_KEY_ESCAPE},
    {"KEY_ENTER", &WSP_KEY_ENTER},
    {"KEY_TAB", &WSP_KEY_TAB},
    {"KEY_BACKSPACE", &WSP_KEY_BACKSPACE},
    {"KEY_INSERT", &WSP_KEY_INSERT},
    {"KEY_DELETE", &WSP_KEY_DELETE},
    {"KEY_RIGHT", &WSP_KEY_RIGHT},
    {"KEY_LEFT", &WSP_KEY_LEFT},
    {"KEY_DOWN", &WSP_KEY_DOWN},
    {"KEY_UP", &WSP_KEY_UP},
    {"KEY_PAGE_UP", &WSP_KEY_PAGE_UP},
    {"KEY_PAGE_DOWN", &WSP_KEY_PAGE_DOWN},
    {"KEY_HOME", &WSP_KEY_HOME},
    {"KEY_END", &WSP_KEY_END},
    {"KEY_CAPS_LOCK", &WSP_KEY_CAPS_LOCK},
    {"KEY_SCROLL_LOCK", &WSP_KEY_SCROLL_LOCK},
    {"KEY_NUM_LOCK", &WSP_KEY_NUM_LOCK},
    {"KEY_PRINT_SCREEN", &WSP_KEY_PRINT_SCREEN},
    {"KEY_PAUSE", &WSP_KEY_PAUSE},
    {"KEY_F1", &WSP_KEY_F1},
    {"KEY_F2", &WSP_KEY_F2},
    {"KEY_F3", &WSP_KEY_F3},
    {"KEY_F4", &WSP_KEY_F4},
    {"KEY_F5", &WSP_KEY_F5},
    {"KEY_F6", &WSP_KEY_F6},
    {"KEY_F7", &WSP_KEY_F7},
    {"KEY_F8", &WSP_KEY_F8},
    {"KEY_F9", &WSP_KEY_F9},
    {"KEY_F10", &WSP_KEY_F10},
    {"KEY_F11", &WSP_KEY_F11},
    {"KEY_F12", &WSP_KEY_F12},
    {"KEY_F13", &WSP_KEY_F13},
    {"KEY_F14", &WSP_KEY_F14},
    {"KEY_F15", &WSP_KEY_F15},
    {"KEY_F16", &WSP_KEY_F16},
    {"KEY_F17", &WSP_KEY_F17},
    {"KEY_F18", &WSP_KEY_F18},
    {"KEY_F19", &WSP_KEY_F19},
    {"KEY_F20", &WSP_KEY_F20},
    {"KEY_F21", &WSP_KEY_F21},
    {"KEY_F22", &WSP_KEY_F22},
    {"KEY_F23", &WSP_KEY_F23},
    {"KEY_F24", &WSP_KEY_F24},
    {"KEY_F25", &WSP_KEY_F25},
    {"KEY_KP_0", &WSP_KEY_KP_0},
    {"KEY_KP_1", &WSP_KEY_KP_1},
    {"KEY_KP_2", &WSP_KEY_KP_2},
    {"KEY_KP_3", &WSP_KEY_KP_3},
    {"KEY_KP_4", &WSP_KEY_KP_4},
    {"KEY_KP_5", &WSP_KEY_KP_5},
    {"KEY_KP_6", &WSP_KEY_KP_6},
    {"KEY_KP_7", &WSP_KEY_KP_7},
    {"KEY_KP_8", &WSP_KEY_KP_8},
    {"KEY_KP_9", &WSP_KEY_KP_9},
    {"KEY_KP_DECIMAL", &WSP_KEY_KP_DECIMAL},
    {"KEY_KP_DIVIDE", &WSP_KEY_KP_DIVIDE},
    {"KEY_KP_MULTIPLY", &WSP_KEY_KP_MULTIPLY},
    {"KEY_KP_SUBTRACT", &WSP_KEY_KP_SUBTRACT},
    {"KEY_KP_ADD", &WSP_KEY_KP_ADD},
    {"KEY_KP_ENTER", &WSP_KEY_KP_ENTER},
    {"KEY_KP_EQUAL", &WSP_KEY_KP_EQUAL},
    {"KEY_LEFT_SHIFT", &WSP_KEY_LEFT_SHIFT},
    {"KEY_LEFT_CONTROL", &WSP_KEY_LEFT_CONTROL},
    {"KEY_LEFT_ALT", &WSP_KEY_LEFT_ALT},
    {"KEY_LEFT_SUPER", &WSP_KEY_LEFT_SUPER},
    {"KEY_RIGHT_SHIFT", &WSP_KEY_RIGHT_SHIFT},
    {"KEY_RIGHT_CONTROL", &WSP_KEY_RIGHT_CONTROL},
    {"KEY_RIGHT_ALT", &WSP_KEY_RIGHT_ALT},
    {"KEY_RIGHT_SUPER", &WSP_KEY_RIGHT_SUPER},
    {"KEY_MENU", &WSP_KEY_MENU},
    {"MOUSE_BUTTON_1", &WSP_MOUSE_BUTTON_1},
    {"MOUSE_BUTTON_2", &WSP_MOUSE_BUTTON_2},
    {"MOUSE_BUTTON_3", &WSP_MOUSE_BUTTON_3},
    {"MOUSE_BUTTON_4", &WSP_MOUSE_BUTTON_4},
    {"MOUSE_BUTTON_5", &WSP_MOUSE_BUTTON_5},
    {"MOUSE_BUTTON_6", &WSP_MOUSE_BUTTON_6},
    {"MOUSE_BUTTON_7", &WSP_MOUSE_BUTTON_7},
    {"MOUSE_BUTTON_8", &WSP_MOUSE_BUTTON_8},
    {"MOUSE_BUTTON_LAST", &WSP_MOUSE_BUTTON_LAST},
    {"MOUSE_BUTTON_LEFT", &WSP_MOUSE_BUTTON_LEFT},
    {"MOUSE_BUTTON_RIGHT", &WSP_MOUSE_BUTTON_RIGHT},
    {"MOUSE_BUTTON_MIDDLE", &WSP_MOUSE_BUTTON_MIDDLE},
    {"GAMEPAD_BUTTON_A", &WSP_GAMEPAD_BUTTON_A},
    {"GAMEPAD_BUTTON_B", &WSP_GAMEPAD_BUTTON_B},
    {"GAMEPAD_BUTTON_X", &WSP_GAMEPAD_BUTTON_X},
    {"GAMEPAD_BUTTON_Y", &WSP_GAMEPAD_BUTTON_Y},
    {"GAMEPAD_BUTTON_CROSS", &WSP_GAMEPAD_BUTTON_CROSS},
    {"GAMEPAD_BUTTON_CIRCLE", &WSP_GAMEPAD_BUTTON_CIRCLE},
    {"GAMEPAD_BUTTON_SQUARE", &WSP_GAMEPAD_BUTTON_SQUARE},
    {"GAMEPAD_BUTTON_TRIANGLE", &WSP_GAMEPAD_BUTTON_TRIANGLE},
    {"GAMEPAD_BUTTON_LEFT_BUMPER", &WSP_GAMEPAD_BUTTON_LEFT_BUMPER},
    {"GAMEPAD_BUTTON_RIGHT_BUMPER", &WSP_GAMEPAD_BUTTON_RIGHT_BUMPER},
    {"GAMEPAD_BUTTON_BACK", &WSP_GAMEPAD_BUTTON_BACK},
    {"GAMEPAD_BUTTON_START", &WSP_GAMEPAD_BUTTON_START},
    {"GAMEPAD_BUTTON_GUIDE", &WSP_GAMEPAD_BUTTON_GUIDE},
    {"GAMEPAD_BUTTON_LEFT_THUMB", &WSP_GAMEPAD_BUTTON_LEFT_THUMB},
    {"GAMEPAD_BUTTON_RIGHT_THUMB", &WSP_GAMEPAD_BUTTON_RIGHT_THUMB},
    {"GAMEPAD_BUTTON_DPAD_UP", &WSP_GAMEPAD_BUTTON_DPAD_UP},
    {"GAMEPAD_BUTTON_DPAD_RIGHT", &WSP_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"GAMEPAD_BUTTON_DPAD_DOWN", &WSP_GAMEPAD_BUTTON_DPAD_DOWN},
    {"GAMEPAD_BUTTON_DPAD_LEFT", &WSP_GAMEPAD_BUTTON_DPAD_LEFT}};

} // namespace wsp

#endif
