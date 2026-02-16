#ifndef WFROST_GENERATED_wsp_input_action
#define WFROST_GENERATED_wsp_input_action

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$ButtonAction() friend struct frost::Meta<wsp::ButtonAction>; 
#define WCLASS_BODY$AxisAction() friend struct frost::Meta<wsp::AxisAction>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::ButtonAction> { static constexpr char const * name = "Button Action"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::ButtonAction;static constexpr auto fields = std::make_tuple(frost::Field<Type,wsp::ButtonAction::Usage>{"Button Usage", &Type::_buttonUsage,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,std::vector<Button>>{"Buttons", &Type::_buttons,Edit::eInput,0.f,0.f,.01,"%.3f"});static constexpr bool hasRefresh = false;};template <> struct frost::Meta<wsp::ButtonAction::Usage> { static constexpr char const * name = ""; static constexpr frost::WhispUsage usage =frost::WhispUsage::eEnum;};template <> inline const wsp::dictionary<std::string,wsp::ButtonAction::Usage>& frost::EnumDictionary() { static const wsp::dictionary<std::string,wsp::ButtonAction::Usage> dict = { {{ "Pressed",wsp::ButtonAction::Usage::ePressed},{ "Held",wsp::ButtonAction::Usage::eHeld},{ "Released",wsp::ButtonAction::Usage::eReleased},{ "Shortcut",wsp::ButtonAction::Usage::eShortcut},{ "Toggle",wsp::ButtonAction::Usage::eToggle}} }; return dict; }template <> struct frost::Meta<wsp::AxisAction> { static constexpr char const * name = "Axis Action"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::AxisAction;static constexpr auto fields = std::make_tuple(frost::Field<Type,wsp::AxisAction::Usage>{"Axis Usage", &Type::_axisUsage,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,std::array<Button, 4>>{"Buttons", &Type::_buttons,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,std::array<Axis, 2>>{"Axes", &Type::_axes,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,float>{"Modifier X", &Type::_modifierX,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,float>{"Modifier Y", &Type::_modifierY,Edit::eInput,0.f,0.f,.01,"%.3f"});static constexpr bool hasRefresh = false;};template <> struct frost::Meta<wsp::AxisAction::Usage> { static constexpr char const * name = ""; static constexpr frost::WhispUsage usage =frost::WhispUsage::eEnum;};template <> inline const wsp::dictionary<std::string,wsp::AxisAction::Usage>& frost::EnumDictionary() { static const wsp::dictionary<std::string,wsp::AxisAction::Usage> dict = { {{ "Buttons",wsp::AxisAction::Usage::eButtons},{ "Axes",wsp::AxisAction::Usage::eAxes}} }; return dict; }

#endif
