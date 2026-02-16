#ifndef WFROST_GENERATED_wsp_viewport_camera
#define WFROST_GENERATED_wsp_viewport_camera

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$ViewportCamera() friend struct frost::Meta<wsp::ViewportCamera>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::ViewportCamera> { static constexpr char const * name = "Viewport Camera"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::ViewportCamera;static constexpr auto fields = std::make_tuple(frost::Field<Type,Camera>{"Camera", &Type::_camera,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,float>{"Orbit Distance", &Type::_orbitDistance,Edit::eDrag,0.01f,1000000.f,0.01f,"%.3f"},frost::Field<Type,float>{"View Distance", &Type::_viewDistance,Edit::eDrag,50.0f,1000000.f,50.0f,"%.3f"},frost::Field<Type,glm::vec3>{"Position", &Type::_position,Edit::eDrag,0.f,0.f,.01,"%.3f"},frost::Field<Type,glm::vec2>{"Rotation", &Type::_rotation,Edit::eDrag,0.f,0.f,1.f,"%.3f"},frost::Field<Type,float>{"Movement Speed", &Type::_movementSpeed,Edit::eSlider,1.f,10000.f,0.01f,"%.3f"},frost::Field<Type,glm::vec2>{"Mouse Sensitivity", &Type::_mouseSensitivity,Edit::eSlider,0.1f,4.f,0.01f,"%.3f"});static constexpr bool hasRefresh = true; static constexpr auto refreshFunc = &Type::Refresh;};

#endif
