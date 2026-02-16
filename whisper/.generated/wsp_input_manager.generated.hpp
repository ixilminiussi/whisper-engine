#ifndef WFROST_GENERATED_wsp_input_manager
#define WFROST_GENERATED_wsp_input_manager

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$InputManager() friend struct frost::Meta<wsp::InputManager>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::InputManager> { static constexpr char const * name = "Input Manager"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::InputManager;static constexpr auto fields = std::make_tuple(frost::Field<Type,dictionary<std::string, class ButtonAction>>{"Button Dictionary", &Type::_buttonDictionary,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,dictionary<std::string, class AxisAction>>{"Axis Dictionary", &Type::_axisDictionary,Edit::eInput,0.f,0.f,.01,"%.3f"});static constexpr bool hasRefresh = false;};

#endif
