#ifndef WFROST_GENERATED_wsp_environment
#define WFROST_GENERATED_wsp_environment

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$Environment() friend struct frost::Meta<wsp::Environment>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::Environment> { static constexpr char const * name = "Environment"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::Environment;static constexpr auto fields = std::make_tuple(frost::Field<Type,float>{"Shadow Map Radius", &Type::_shadowMapRadius,eSlider,1.f,100.f,.01,"%.3f"},frost::Field<Type,glm::vec3>{"Sun Color", &Type::_sunColor,eColor,0.f,0.f,.01,"%.3f"},frost::Field<Type,float>{"Sun Intensity", &Type::_sunIntensity,eSlider,0.f,10.f,.01,"%.3f"},frost::Field<Type,float>{"Rotation", &Type::_rotation,eSlider,0.f,360.f,1.f,"%.3f"});static constexpr bool hasRefresh = true; static constexpr auto refreshFunc = &Type::Refresh;};

#endif
