#ifndef WFROST_GENERATED_wsp_camera
#define WFROST_GENERATED_wsp_camera

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$Camera() friend struct frost::Meta<wsp::Camera>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::Camera> { static constexpr char const * name = "Camera"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::Camera;static constexpr auto fields = std::make_tuple(frost::Field<Type,float>{"Fov", &Type::_fov,eSlider,10.f,170.f,.01,"%.3f"},frost::Field<Type,float>{"Near Plane", &Type::_nearPlane,eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,float>{"Far Plane", &Type::_farPlane,eInput,0.f,0.f,.01,"%.3f"});static constexpr bool hasRefresh = true; static constexpr auto refreshFunc = &Type::Refresh;};

#endif
