#ifndef WFROST_GENERATED_wsp_transform
#define WFROST_GENERATED_wsp_transform

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$Transform() friend struct frost::Meta<wsp::Transform>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::Transform> { static constexpr char const * name = "Transform"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::Transform;static constexpr auto fields = std::make_tuple(frost::Field<Type,glm::vec3>{"Position", &Type::_position,eDrag,0.f,0.f,.01,"%.3f"},frost::Field<Type,glm::vec3>{"Scale", &Type::_scale,eDrag,0.f,0.f,.01,"%.3f"});static constexpr bool hasRefresh = true; static constexpr auto refreshFunc = &Type::Refresh;};

#endif
